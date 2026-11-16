#pragma once

#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace bible {

struct RemoteSwordBible {
    QString id;
    QString description;
    QString lang;
};

inline std::vector<RemoteSwordBible> parse_sword_repo_dir(const QString &mods_dir)
{
    std::vector<RemoteSwordBible> out;
    QDir dir(mods_dir);
    if (!dir.exists())
        return out;

    const auto files = dir.entryInfoList({"*.conf"}, QDir::Files, QDir::Name);
    for (const auto &fi : files) {
        std::ifstream ifs(fi.absoluteFilePath().toStdString());
        if (!ifs.is_open())
            continue;
        std::string line, id, desc, lang, drv;
        while (std::getline(ifs, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
                line.pop_back();
            if (line.empty() || line[0] == '#')
                continue;
            if (line.front() == '[' && line.back() == ']' && id.empty()) {
                id = line.substr(1, line.size() - 2);
                continue;
            }
            auto eq = line.find('=');
            if (eq == std::string::npos)
                continue;
            std::string k = line.substr(0, eq);
            std::string v = line.substr(eq + 1);
            if (k == "Description")
                desc = v;
            else if (k == "Lang")
                lang = v;
            else if (k == "ModDrv")
                drv = v;
        }
        // Only Biblical Texts (zText, zText4, RawText, RawText4)
        if (!id.empty() && (drv == "zText" || drv == "zText4" || drv == "RawText" || drv == "RawText4")) {
            out.push_back({
                QString::fromUtf8(id.c_str()),
                QString::fromUtf8(desc.c_str()),
                lang.empty() ? QString("en") : QString::fromUtf8(lang.c_str()),
            });
        }
    }
    return out;
}

class SwordFetchThread : public QThread {
    Q_OBJECT
public:
    explicit SwordFetchThread(QObject *parent = nullptr) : QThread(parent) {}
    std::vector<RemoteSwordBible> results;
    QString error_msg;

protected:
    void run() override
    {
        QString tmp = QDir::tempPath() + "/obs_bible_sword_repo";
        QDir(tmp).removeRecursively();
        QDir().mkpath(tmp);

        QString tgz = tmp + "/mods.d.tar.gz";
        QString cmd = QString("curl -fsSL -L 'https://crosswire.org/ftpmirror/pub/sword/raw/mods.d.tar.gz' -o '%1' && tar -xzf '%1' -C '%2'")
                          .arg(tgz, tmp);
        int rc = std::system(cmd.toUtf8().constData());
        if (rc != 0) {
            error_msg = "Impossible de télécharger le catalogue CrossWire SWORD (vérifiez votre connexion Internet).";
            return;
        }
        results = parse_sword_repo_dir(tmp + "/mods.d");
    }
};

class SwordInstallThread : public QThread {
    Q_OBJECT
public:
    SwordInstallThread(QString mod_id, QObject *parent = nullptr)
        : QThread(parent), mod_id_(std::move(mod_id))
    {
    }
    bool ok = false;
    QString mod_id_;
    QString error_msg;

protected:
    void run() override
    {
        QString sword_dir = QDir::homePath() + "/.sword";
        QDir().mkpath(sword_dir);
        QString zip_path = QDir::tempPath() + "/" + mod_id_ + ".zip";
        QString url = QString("https://crosswire.org/ftpmirror/pub/sword/packages/rawzip/%1.zip").arg(mod_id_);
        QString cmd = QString("curl -fsSL -L '%1' -o '%2' && unzip -qo '%2' -d '%3' && rm -f '%2'")
                          .arg(url, zip_path, sword_dir);
        int rc = std::system(cmd.toUtf8().constData());
        if (rc != 0) {
            error_msg = QString("Échec du téléchargement de %1 (vérifiez que 'curl' et 'unzip' sont installés).").arg(mod_id_);
            ok = false;
            return;
        }
        ok = true;
    }
};

class SwordDownloadDialog : public QDialog {
    Q_OBJECT
public:
    explicit SwordDownloadDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Télécharger des Bibles SWORD (CrossWire)");
        resize(560, 480);
        setStyleSheet("QWidget { font-size: 13px; }");

        auto *lay = new QVBoxLayout(this);

        auto *top = new QHBoxLayout();
        lang_combo_ = new QComboBox();
        lang_combo_->addItem("Toutes les langues", "");
        filter_edit_ = new QLineEdit();
        filter_edit_->setPlaceholderText("Filtrer par nom (ex: Segond, Reina, KJV)…");
        top->addWidget(new QLabel("Langue :"));
        top->addWidget(lang_combo_);
        top->addWidget(filter_edit_, 1);
        lay->addLayout(top);

        list_ = new QListWidget();
        list_->setStyleSheet("QListWidget { font-size: 13px; }");
        lay->addWidget(list_, 1);

        progress_ = new QProgressBar();
        progress_->setRange(0, 0);
        lay->addWidget(progress_);

        auto *bot = new QHBoxLayout();
        status_lbl_ = new QLabel("Chargement du catalogue CrossWire…");
        install_btn_ = new QPushButton("Télécharger & Installer");
        install_btn_->setEnabled(false);
        install_btn_->setStyleSheet("background-color: #00a884; color: #081210; font-weight: bold; padding: 6px 14px;");
        auto *close_btn = new QPushButton("Fermer");
        bot->addWidget(status_lbl_, 1);
        bot->addWidget(install_btn_);
        bot->addWidget(close_btn);
        lay->addLayout(bot);

        connect(lang_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { refresh_list(); });
        connect(filter_edit_, &QLineEdit::textChanged, this, [this]() { refresh_list(); });
        connect(install_btn_, &QPushButton::clicked, this, [this]() { install_selected(); });
        connect(close_btn, &QPushButton::clicked, this, &QDialog::accept);

        auto *fetcher = new SwordFetchThread(this);
        connect(fetcher, &QThread::finished, this, [this, fetcher]() {
            progress_->setRange(0, 1);
            progress_->setValue(1);
            if (!fetcher->error_msg.isEmpty()) {
                status_lbl_->setText(fetcher->error_msg);
            } else {
                all_bibles_ = std::move(fetcher->results);
                populate_languages();
                refresh_list();
                install_btn_->setEnabled(true);
                status_lbl_->setText(QString("%1 Bibles disponibles.").arg(all_bibles_.size()));
            }
            fetcher->deleteLater();
        });
        fetcher->start();
    }

    bool installed_any = false;

private:
    void populate_languages()
    {
        std::set<QString> langs;
        for (const auto &b : all_bibles_)
            langs.insert(b.lang);
        lang_combo_->blockSignals(true);
        lang_combo_->clear();
        lang_combo_->addItem("Toutes les langues", "");
        for (const auto &l : langs)
            lang_combo_->addItem(l, l);
        lang_combo_->blockSignals(false);
    }

    void refresh_list()
    {
        QString lang = lang_combo_->currentData().toString();
        QString q = filter_edit_->text().trimmed();
        list_->clear();
        for (const auto &b : all_bibles_) {
            if (!lang.isEmpty() && b.lang != lang)
                continue;
            if (!q.isEmpty() && !b.id.contains(q, Qt::CaseInsensitive) &&
                !b.description.contains(q, Qt::CaseInsensitive))
                continue;
            auto *item = new QListWidgetItem(QString("[%1] %2 — %3").arg(b.lang, b.id, b.description));
            item->setData(Qt::UserRole, b.id);
            list_->addItem(item);
        }
    }

    void install_selected()
    {
        auto *item = list_->currentItem();
        if (!item)
            return;
        QString mod_id = item->data(Qt::UserRole).toString();
        install_btn_->setEnabled(false);
        progress_->setRange(0, 0);
        status_lbl_->setText("Téléchargement de " + mod_id + "…");

        auto *worker = new SwordInstallThread(mod_id, this);
        connect(worker, &QThread::finished, this, [this, worker]() {
            progress_->setRange(0, 1);
            progress_->setValue(1);
            install_btn_->setEnabled(true);
            if (worker->ok) {
                installed_any = true;
                status_lbl_->setText("Installé dans ~/.sword : " + worker->mod_id_);
            } else {
                status_lbl_->setText(worker->error_msg);
            }
            worker->deleteLater();
        });
        worker->start();
    }

    std::vector<RemoteSwordBible> all_bibles_;
    QComboBox *lang_combo_ = nullptr;
    QLineEdit *filter_edit_ = nullptr;
    QListWidget *list_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QLabel *status_lbl_ = nullptr;
    QPushButton *install_btn_ = nullptr;
};

} // namespace bible
