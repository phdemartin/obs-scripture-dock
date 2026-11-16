#pragma once

#include <QAbstractItemView>
#include <QColorDialog>
#include <QComboBox>
#include <QFileDialog>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "bible_overlay.hpp"
#include "bible_store.hpp"
#include "sword_downloader.hpp"

namespace bible {

class BibleDockWidget : public QWidget {
public:
    explicit BibleDockWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Scripture Dock");
        setMinimumWidth(220);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        setStyleSheet(
            "QWidget { font-size: 13px; }"
            "QListWidget, QTreeWidget { font-size: 14px; }"
            "QListWidget::item, QTreeWidget::item { padding: 4px 2px; }"
            "QPushButton { padding: 5px 6px; }");

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(6, 6, 6, 6);
        root->setSpacing(5);

        // --- Row 1: Translation + Folder + Online SWORD Download ---
        auto *bib_row = new QHBoxLayout();
        bible_combo_ = new QComboBox();
        bible_combo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        bible_combo_->setMinimumContentsLength(8);
        bible_combo_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        auto *dir_btn = new QPushButton("📁");
        dir_btn->setFixedWidth(30);
        dir_btn->setToolTip("Choisir un dossier local de Bibles (.js / .json)");
        auto *dl_btn = new QPushButton("🌐");
        dl_btn->setFixedWidth(30);
        dl_btn->setToolTip("Télécharger des Bibles SWORD gratuites en ligne");
        bib_row->addWidget(bible_combo_, 1);
        bib_row->addWidget(dir_btn);
        bib_row->addWidget(dl_btn);
        root->addLayout(bib_row);

        // --- Tabs: Recherche / Liste (Prédication) / Style ---
        tabs_ = new QTabWidget();
        root->addWidget(tabs_, 1);

        // === Tab 1: Recherche & Navigation ===
        auto *search_tab = new QWidget();
        auto *st_lay = new QVBoxLayout(search_tab);
        st_lay->setContentsMargins(6, 6, 6, 6);
        st_lay->setSpacing(6);

        auto *search_row = new QHBoxLayout();
        search_edit_ = new QLineEdit();
        search_edit_->setPlaceholderText("Ex: Galates 3:1-6 ou mot-clé…");
        auto *search_btn = new QPushButton("🔍");
        search_btn->setFixedWidth(36);
        search_row->addWidget(search_edit_, 1);
        search_row->addWidget(search_btn);
        st_lay->addLayout(search_row);

        auto *nav_row = new QHBoxLayout();
        book_combo_ = new QComboBox();
        book_combo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        book_combo_->setMinimumContentsLength(6);
        book_combo_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        chapter_combo_ = new QComboBox();
        chapter_combo_->setFixedWidth(64);
        nav_row->addWidget(book_combo_, 1);
        nav_row->addWidget(chapter_combo_);
        st_lay->addLayout(nav_row);

        verse_list_ = new QListWidget();
        verse_list_->setWordWrap(true);
        verse_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        verse_list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        st_lay->addWidget(verse_list_, 1);

        auto *add_btn = new QPushButton("+ Ajouter à la liste");
        add_btn->setToolTip("Ajoute la sélection, ou tout le passage recherché (ex: Galates 3:1-6), dans l'onglet Liste");
        st_lay->addWidget(add_btn);

        tabs_->addTab(search_tab, "Recherche");

        // === Tab 2: Liste (Prédication groupée par passage) ===
        auto *list_tab = new QWidget();
        auto *lt_lay = new QVBoxLayout(list_tab);
        lt_lay->setContentsMargins(6, 6, 6, 6);
        lt_lay->setSpacing(6);

        playlist_tree_ = new QTreeWidget();
        playlist_tree_->setHeaderHidden(true);
        playlist_tree_->setWordWrap(true);
        playlist_tree_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        playlist_tree_->setIndentation(14);
        lt_lay->addWidget(playlist_tree_, 1);

        auto *pl_btns = new QHBoxLayout();
        auto *up_btn = new QPushButton("▲");
        auto *down_btn = new QPushButton("▼");
        auto *rem_btn = new QPushButton("Retirer");
        auto *clr_list_btn = new QPushButton("Vider");
        up_btn->setFixedWidth(36);
        down_btn->setFixedWidth(36);
        pl_btns->addWidget(up_btn);
        pl_btns->addWidget(down_btn);
        pl_btns->addStretch();
        pl_btns->addWidget(rem_btn);
        pl_btns->addWidget(clr_list_btn);
        lt_lay->addLayout(pl_btns);

        tabs_->addTab(list_tab, "Liste (0)");

        // === Tab 3: Style ===
        auto *style_tab = new QWidget();
        auto *sy_lay = new QVBoxLayout(style_tab);
        sy_lay->setContentsMargins(6, 8, 6, 6);
        sy_lay->setSpacing(8);

        auto *style_row1 = new QHBoxLayout();
        pos_combo_ = new QComboBox();
        pos_combo_->addItem("Bas", 0);
        pos_combo_->addItem("Centre", 1);
        pos_combo_->addItem("Haut", 2);
        size_spin_ = new QSpinBox();
        size_spin_->setRange(18, 96);
        size_spin_->setValue(42);
        size_spin_->setSuffix(" px");
        style_row1->addWidget(new QLabel("Position :"));
        style_row1->addWidget(pos_combo_, 1);
        style_row1->addWidget(new QLabel("Taille :"));
        style_row1->addWidget(size_spin_);
        sy_lay->addLayout(style_row1);

        auto *style_row2 = new QHBoxLayout();
        font_combo_ = new QFontComboBox();
        font_combo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        font_combo_->setMinimumContentsLength(6);
        font_combo_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        style_row2->addWidget(new QLabel("Police :"));
        style_row2->addWidget(font_combo_, 1);
        sy_lay->addLayout(style_row2);

        auto *style_row3 = new QHBoxLayout();
        opacity_slider_ = new QSlider(Qt::Horizontal);
        opacity_slider_->setRange(0, 255);
        opacity_slider_->setValue(200);
        style_row3->addWidget(new QLabel("Opacité fond :"));
        style_row3->addWidget(opacity_slider_, 1);
        sy_lay->addLayout(style_row3);

        auto *style_row4 = new QHBoxLayout();
        auto *txt_col_btn = new QPushButton("Couleur Texte");
        auto *bg_col_btn = new QPushButton("Couleur Fond");
        style_row4->addWidget(txt_col_btn);
        style_row4->addWidget(bg_col_btn);
        sy_lay->addLayout(style_row4);
        sy_lay->addStretch();

        tabs_->addTab(style_tab, "Style");

        // --- Live Action Buttons (3-button compact bar: ◀ | Afficher/Masquer | ▶) ---
        auto *act_row = new QHBoxLayout();
        act_row->setSpacing(4);
        auto *prev_btn = new QPushButton("◀");
        prev_btn->setFixedWidth(40);
        prev_btn->setToolTip("Verset précédent");
        toggle_btn_ = new QPushButton("Afficher");
        toggle_btn_->setStyleSheet("background-color: #00a884; color: #081210; font-weight: bold;");
        auto *next_btn = new QPushButton("▶");
        next_btn->setFixedWidth(40);
        next_btn->setToolTip("Verset suivant");
        act_row->addWidget(prev_btn);
        act_row->addWidget(toggle_btn_, 1);
        act_row->addWidget(next_btn);
        root->addLayout(act_row);

        status_lbl_ = new QLabel("Prêt");
        status_lbl_->setStyleSheet("color: #8b9bb4; font-size: 12px;");
        root->addWidget(status_lbl_);

        // --- Load settings & bibles ---
        load_settings();
        reload_bible_list();
        update_playlist_tab_title();

        // --- Connections ---
        connect(dir_btn, &QPushButton::clicked, this, [this]() { choose_bible_dir(); });
        connect(dl_btn, &QPushButton::clicked, this, [this]() { open_sword_downloader(); });
        connect(bible_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
            on_bible_changed(idx);
        });
        connect(book_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
            on_book_changed(idx);
        });
        connect(chapter_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
            on_chapter_changed(idx);
        });
        connect(search_btn, &QPushButton::clicked, this, [this]() { run_search(); });
        connect(search_edit_, &QLineEdit::returnPressed, this, [this]() { run_search(); });
        connect(verse_list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
            show_search_item(item, true);
        });
        connect(playlist_tree_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int) {
            show_tree_item(item, true);
        });
        connect(add_btn, &QPushButton::clicked, this, [this]() { add_to_playlist(); });
        connect(up_btn, &QPushButton::clicked, this, [this]() { move_group(-1); });
        connect(down_btn, &QPushButton::clicked, this, [this]() { move_group(1); });
        connect(rem_btn, &QPushButton::clicked, this, [this]() { remove_group(); });
        connect(clr_list_btn, &QPushButton::clicked, this, [this]() {
            playlist_tree_->clear();
            save_playlist();
            update_playlist_tab_title();
        });

        connect(toggle_btn_, &QPushButton::clicked, this, [this]() {
            if (SharedOverlay::instance().snapshot().visible) {
                SharedOverlay::instance().clear();
                status_lbl_->setText("Masqué");
                sync_toggle_button();
                return;
            }
            if (tabs_->currentIndex() == 1)
                show_tree_item(playlist_tree_->currentItem(), false);
            else
                show_search_item(verse_list_->currentItem(), false);
        });
        connect(prev_btn, &QPushButton::clicked, this, [this]() { step_active(-1); });
        connect(next_btn, &QPushButton::clicked, this, [this]() { step_active(1); });

        auto sync_style = [this]() { apply_style(); };
        connect(pos_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, sync_style);
        connect(size_spin_, QOverload<int>::of(&QSpinBox::valueChanged), this, sync_style);
        connect(opacity_slider_, &QSlider::valueChanged, this, sync_style);
        connect(font_combo_, &QFontComboBox::currentFontChanged, this, sync_style);
        connect(txt_col_btn, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(text_color_, this, "Couleur du texte");
            if (c.isValid()) {
                text_color_ = c;
                apply_style();
            }
        });
        connect(bg_col_btn, &QPushButton::clicked, this, [this]() {
            QColor c = QColorDialog::getColor(bg_color_, this, "Couleur du fond");
            if (c.isValid()) {
                bg_color_ = c;
                apply_style();
            }
        });

        apply_style();
    }

private:
    void open_sword_downloader()
    {
        SwordDownloadDialog dlg(this);
        dlg.exec();
        if (dlg.installed_any)
            reload_bible_list();
    }

    void add_group_node(const QString &title, const QList<QPair<QString, QString>> &verses, bool expand = false)
    {
        auto *parent = new QTreeWidgetItem(playlist_tree_);
        parent->setText(0, title);
        QFont f = parent->font(0);
        f.setBold(true);
        parent->setFont(0, f);
        parent->setData(0, Qt::UserRole + 7, title);

        for (const auto &vt : verses) {
            auto *child = new QTreeWidgetItem(parent);
            child->setText(0, QString("%1 — %2").arg(vt.first, vt.second));
            child->setData(0, Qt::UserRole + 6, vt.first);
            child->setData(0, Qt::UserRole + 5, vt.second);
        }
        parent->setExpanded(expand);
    }

    void load_settings()
    {
        QSettings s("OBSBiblePlugin", "obs-bible-plugin");
        custom_dir_ = s.value("custom_dir", "").toString();
        last_bible_ = s.value("last_bible", "").toString();
        size_spin_->setValue(s.value("font_size", 42).toInt());
        opacity_slider_->setValue(s.value("bg_opacity", 200).toInt());
        pos_combo_->setCurrentIndex(s.value("position", 0).toInt());
        text_color_ = QColor(s.value("text_color", "#ffffff").toString());
        bg_color_ = QColor(s.value("bg_color", "#0c1018").toString());
        QString ff = s.value("font_family", "").toString();
        if (!ff.isEmpty())
            font_combo_->setCurrentFont(QFont(ff));

        QByteArray pl_json = s.value("playlist_groups_json").toByteArray();
        if (!pl_json.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(pl_json);
            if (doc.isArray()) {
                for (const auto &gval : doc.array()) {
                    QJsonObject go = gval.toObject();
                    QString title = go.value("title").toString();
                    QList<QPair<QString, QString>> verses;
                    for (const auto &vval : go.value("verses").toArray()) {
                        QJsonObject vo = vval.toObject();
                        verses.append({vo.value("ref").toString(), vo.value("text").toString()});
                    }
                    if (!title.isEmpty() && !verses.isEmpty())
                        add_group_node(title, verses, false);
                }
            }
        }
    }

    void save_settings() const
    {
        QSettings s("OBSBiblePlugin", "obs-bible-plugin");
        s.setValue("custom_dir", custom_dir_);
        s.setValue("last_bible", bible_combo_->currentData().toString());
        s.setValue("font_size", size_spin_->value());
        s.setValue("bg_opacity", opacity_slider_->value());
        s.setValue("position", pos_combo_->currentIndex());
        s.setValue("text_color", text_color_.name());
        s.setValue("bg_color", bg_color_.name());
        s.setValue("font_family", font_combo_->currentFont().family());
    }

    void save_playlist() const
    {
        QJsonArray arr;
        for (int i = 0; i < playlist_tree_->topLevelItemCount(); ++i) {
            auto *g = playlist_tree_->topLevelItem(i);
            QJsonObject go;
            go["title"] = g->data(0, Qt::UserRole + 7).toString();
            QJsonArray varr;
            for (int j = 0; j < g->childCount(); ++j) {
                auto *c = g->child(j);
                QJsonObject vo;
                vo["ref"] = c->data(0, Qt::UserRole + 6).toString();
                vo["text"] = c->data(0, Qt::UserRole + 5).toString();
                varr.append(vo);
            }
            go["verses"] = varr;
            arr.append(go);
        }
        QSettings s("OBSBiblePlugin", "obs-bible-plugin");
        s.setValue("playlist_groups_json", QJsonDocument(arr).toJson(QJsonDocument::Compact));
    }

    void update_playlist_tab_title()
    {
        tabs_->setTabText(1, QString("Liste (%1)").arg(playlist_tree_->topLevelItemCount()));
    }

    void choose_bible_dir()
    {
        QString dir = QFileDialog::getExistingDirectory(this, "Dossier des Bibles (.js / .json)", custom_dir_);
        if (dir.isEmpty())
            return;
        custom_dir_ = dir;
        save_settings();
        reload_bible_list();
    }

    void reload_bible_list()
    {
        bible_combo_->blockSignals(true);
        bible_combo_->clear();
        int target_idx = 0;

        const QStringList files = discover_bible_files(custom_dir_);
        for (const auto &f : files) {
            QFileInfo fi(f);
            int idx = bible_combo_->count();
            bible_combo_->addItem(fi.baseName(), f);
            if (f == last_bible_)
                target_idx = idx;
        }

        const auto swords = discover_sword_bibles();
        for (const auto &sm : swords) {
            QString key = "sword://" + sm.name;
            int idx = bible_combo_->count();
            bible_combo_->addItem(QString("[SWORD] %1 — %2").arg(sm.name, sm.description), key);
            if (key == last_bible_)
                target_idx = idx;
        }

        bible_combo_->blockSignals(false);
        if (bible_combo_->count() > 0) {
            bible_combo_->setCurrentIndex(target_idx);
            on_bible_changed(target_idx);
        } else {
            status_lbl_->setText("Aucune Bible trouvée. Cliquez sur 🌐 pour en télécharger.");
        }
    }

    void on_bible_changed(int idx)
    {
        if (idx < 0)
            return;
        QString path = bible_combo_->itemData(idx).toString();
        bool ok = false;
        if (path.startsWith("sword://"))
            ok = load_sword_module(path.mid(8), current_bible_);
        else
            ok = load_bible_file(path, current_bible_);

        if (!ok) {
            status_lbl_->setText("Erreur de lecture : " + path);
            return;
        }
        save_settings();

        book_combo_->blockSignals(true);
        book_combo_->clear();
        for (const auto &b : current_bible_.books)
            book_combo_->addItem(b.name, b.id);
        book_combo_->blockSignals(false);
        if (book_combo_->count() > 0) {
            book_combo_->setCurrentIndex(0);
            on_book_changed(0);
        }
        status_lbl_->setText(QString("%1 (%2 livres)").arg(current_bible_.name).arg(current_bible_.books.size()));
    }

    void on_book_changed(int idx)
    {
        if (idx < 0)
            return;
        int book_id = book_combo_->itemData(idx).toInt();
        const Book *b = current_bible_.find_book(book_id);
        if (!b)
            return;
        chapter_combo_->blockSignals(true);
        chapter_combo_->clear();
        for (const auto &cp : b->chapters)
            chapter_combo_->addItem(QString::number(cp.first), cp.first);
        chapter_combo_->blockSignals(false);
        if (chapter_combo_->count() > 0) {
            chapter_combo_->setCurrentIndex(0);
            on_chapter_changed(0);
        }
    }

    void on_chapter_changed(int idx)
    {
        if (idx < 0)
            return;
        int book_id = book_combo_->itemData(book_combo_->currentIndex()).toInt();
        int ch = chapter_combo_->itemData(idx).toInt();
        const Book *b = current_bible_.find_book(book_id);
        if (!b)
            return;
        auto it = b->chapters.find(ch);
        if (it == b->chapters.end())
            return;

        verse_list_->clear();
        for (const auto &v : it->second) {
            auto *item = new QListWidgetItem(QString("%1  %2").arg(v.verse).arg(v.text));
            item->setData(Qt::UserRole + 1, v.book_id);
            item->setData(Qt::UserRole + 2, v.chapter);
            item->setData(Qt::UserRole + 3, v.verse);
            item->setData(Qt::UserRole + 4, b->name);
            item->setData(Qt::UserRole + 5, v.text);
            verse_list_->addItem(item);
        }
    }

    void run_search()
    {
        QString q = search_edit_->text().trimmed();
        if (q.isEmpty()) {
            on_chapter_changed(chapter_combo_->currentIndex());
            return;
        }
        auto hits = current_bible_.search(q);
        verse_list_->clear();
        for (const auto &h : hits) {
            auto *item = new QListWidgetItem(
                QString("%1 %2:%3 — %4").arg(h.book_name).arg(h.chapter).arg(h.verse).arg(h.text));
            item->setData(Qt::UserRole + 1, h.book_id);
            item->setData(Qt::UserRole + 2, h.chapter);
            item->setData(Qt::UserRole + 3, h.verse);
            item->setData(Qt::UserRole + 4, h.book_name);
            item->setData(Qt::UserRole + 5, h.text);
            verse_list_->addItem(item);
        }
        status_lbl_->setText(QString("%1 résultat(s)").arg(hits.size()));
    }

    void add_to_playlist()
    {
        QList<QListWidgetItem *> items = verse_list_->selectedItems();
        if (items.isEmpty()) {
            for (int i = 0; i < verse_list_->count(); ++i)
                items.append(verse_list_->item(i));
        }
        if (items.isEmpty())
            return;

        QString abbr = current_bible_.abbr.isEmpty() ? current_bible_.name : current_bible_.abbr;
        QList<QPair<QString, QString>> verses;
        for (auto *src : items) {
            QString bname = src->data(Qt::UserRole + 4).toString();
            int ch = src->data(Qt::UserRole + 2).toInt();
            int vs = src->data(Qt::UserRole + 3).toInt();
            QString vtext = src->data(Qt::UserRole + 5).toString();
            QString ref = QString("%1 %2:%3 (%4)").arg(bname).arg(ch).arg(vs).arg(abbr);
            verses.append({ref, vtext});
        }

        // Build compact group title: e.g. "Galates 3:1-6 (FreLSG)"
        auto *first = items.front();
        auto *last = items.back();
        QString b_first = first->data(Qt::UserRole + 4).toString();
        int c_first = first->data(Qt::UserRole + 2).toInt();
        int v_first = first->data(Qt::UserRole + 3).toInt();
        QString b_last = last->data(Qt::UserRole + 4).toString();
        int c_last = last->data(Qt::UserRole + 2).toInt();
        int v_last = last->data(Qt::UserRole + 3).toInt();

        QString title;
        if (items.size() == 1) {
            title = QString("%1 %2:%3 (%4)").arg(b_first).arg(c_first).arg(v_first).arg(abbr);
        } else if (b_first == b_last && c_first == c_last) {
            title = QString("%1 %2:%3-%4 (%5)").arg(b_first).arg(c_first).arg(v_first).arg(v_last).arg(abbr);
        } else {
            title = QString("%1 %2:%3 — %4 %5:%6 (%7)")
                        .arg(b_first)
                        .arg(c_first)
                        .arg(v_first)
                        .arg(b_last)
                        .arg(c_last)
                        .arg(v_last)
                        .arg(abbr);
        }

        add_group_node(title, verses, false);
        save_playlist();
        update_playlist_tab_title();
        status_lbl_->setText("Ajouté à la liste : " + title);
    }

    void move_group(int delta)
    {
        auto *cur = playlist_tree_->currentItem();
        if (!cur)
            return;
        auto *top = cur->parent() ? cur->parent() : cur;
        int idx = playlist_tree_->indexOfTopLevelItem(top);
        int next = idx + delta;
        if (idx < 0 || next < 0 || next >= playlist_tree_->topLevelItemCount())
            return;
        bool was_exp = top->isExpanded();
        auto *taken = playlist_tree_->takeTopLevelItem(idx);
        playlist_tree_->insertTopLevelItem(next, taken);
        taken->setExpanded(was_exp);
        playlist_tree_->setCurrentItem(taken);
        save_playlist();
    }

    void remove_group()
    {
        auto *cur = playlist_tree_->currentItem();
        if (!cur)
            return;
        auto *top = cur->parent() ? cur->parent() : cur;
        int idx = playlist_tree_->indexOfTopLevelItem(top);
        if (idx >= 0) {
            delete playlist_tree_->takeTopLevelItem(idx);
            save_playlist();
            update_playlist_tab_title();
        }
    }

    void sync_toggle_button()
    {
        bool vis = SharedOverlay::instance().snapshot().visible;
        if (vis) {
            toggle_btn_->setText("Masquer");
            toggle_btn_->setStyleSheet("background-color: #5c232c; color: #ffd4db; font-weight: bold;");
        } else {
            toggle_btn_->setText("Afficher");
            toggle_btn_->setStyleSheet("background-color: #00a884; color: #081210; font-weight: bold;");
        }
    }

    void display_or_toggle(const QString &ref, const QString &vtext, bool toggle_if_same)
    {
        if (toggle_if_same) {
            auto st = SharedOverlay::instance().snapshot();
            if (st.visible && st.reference == ref) {
                SharedOverlay::instance().clear();
                status_lbl_->setText("Masqué");
                sync_toggle_button();
                return;
            }
        }
        SharedOverlay::instance().set_verse(ref, vtext);
        status_lbl_->setText("Affiché : " + ref);
        sync_toggle_button();
    }

    void show_search_item(QListWidgetItem *item, bool toggle_if_same = false)
    {
        if (!item)
            return;
        active_book_id_ = item->data(Qt::UserRole + 1).toInt();
        active_chapter_ = item->data(Qt::UserRole + 2).toInt();
        active_verse_ = item->data(Qt::UserRole + 3).toInt();
        QString bname = item->data(Qt::UserRole + 4).toString();
        QString vtext = item->data(Qt::UserRole + 5).toString();

        QString abbr = current_bible_.abbr.isEmpty() ? current_bible_.name : current_bible_.abbr;
        QString ref = QString("%1 %2:%3 (%4)").arg(bname).arg(active_chapter_).arg(active_verse_).arg(abbr);
        display_or_toggle(ref, vtext, toggle_if_same);
    }

    void show_tree_item(QTreeWidgetItem *item, bool toggle_if_same = false)
    {
        if (!item)
            return;
        if (!item->parent()) {
            item->setExpanded(true);
            if (item->childCount() == 0)
                return;
            item = item->child(0);
            playlist_tree_->setCurrentItem(item);
        }
        QString ref = item->data(0, Qt::UserRole + 6).toString();
        QString vtext = item->data(0, Qt::UserRole + 5).toString();
        display_or_toggle(ref, vtext, toggle_if_same);
    }

    void step_active(int delta)
    {
        if (tabs_->currentIndex() == 1 && playlist_tree_->topLevelItemCount() > 0) {
            // Flatten all child verse items in order
            std::vector<QTreeWidgetItem *> all_verses;
            for (int i = 0; i < playlist_tree_->topLevelItemCount(); ++i) {
                auto *g = playlist_tree_->topLevelItem(i);
                for (int j = 0; j < g->childCount(); ++j)
                    all_verses.push_back(g->child(j));
            }
            if (all_verses.empty())
                return;

            auto *cur = playlist_tree_->currentItem();
            int cur_idx = -1;
            if (cur && cur->parent()) {
                for (int k = 0; k < static_cast<int>(all_verses.size()); ++k) {
                    if (all_verses[k] == cur) {
                        cur_idx = k;
                        break;
                    }
                }
            }
            int next_idx = (cur_idx < 0) ? 0 : std::clamp(cur_idx + delta, 0, static_cast<int>(all_verses.size()) - 1);
            auto *target = all_verses[next_idx];
            if (target->parent())
                target->parent()->setExpanded(true);
            playlist_tree_->setCurrentItem(target);
            show_tree_item(target);
            return;
        }

        if (active_book_id_ <= 0) {
            if (verse_list_->count() > 0) {
                verse_list_->setCurrentRow(0);
                show_search_item(verse_list_->currentItem());
            }
            return;
        }
        if (current_bible_.step_verse(active_book_id_, active_chapter_, active_verse_, delta)) {
            const Book *b = current_bible_.find_book(active_book_id_);
            const Verse *v = current_bible_.get_verse(active_book_id_, active_chapter_, active_verse_);
            if (b && v) {
                QString abbr = current_bible_.abbr.isEmpty() ? current_bible_.name : current_bible_.abbr;
                QString ref = QString("%1 %2:%3 (%4)").arg(b->name).arg(active_chapter_).arg(active_verse_).arg(abbr);
                SharedOverlay::instance().set_verse(ref, v->text);
                status_lbl_->setText("Affiché : " + ref);
                sync_toggle_button();
            }
        }
    }

    void apply_style()
    {
        SharedOverlay::instance().update_style(
            font_combo_->currentFont().family(),
            size_spin_->value(),
            text_color_,
            QColor(0, 210, 168),
            bg_color_,
            opacity_slider_->value(),
            pos_combo_->currentData().toInt());
        save_settings();
    }

    QString custom_dir_;
    QString last_bible_;
    BibleData current_bible_;

    QTabWidget *tabs_ = nullptr;
    QComboBox *bible_combo_ = nullptr;
    QLineEdit *search_edit_ = nullptr;
    QComboBox *book_combo_ = nullptr;
    QComboBox *chapter_combo_ = nullptr;
    QListWidget *verse_list_ = nullptr;
    QTreeWidget *playlist_tree_ = nullptr;
    QComboBox *pos_combo_ = nullptr;
    QSpinBox *size_spin_ = nullptr;
    QSlider *opacity_slider_ = nullptr;
    QFontComboBox *font_combo_ = nullptr;
    QPushButton *toggle_btn_ = nullptr;
    QLabel *status_lbl_ = nullptr;

    QColor text_color_ = Qt::white;
    QColor bg_color_ = QColor(12, 16, 24);
    int active_book_id_ = 0;
    int active_chapter_ = 0;
    int active_verse_ = 0;
};

} // namespace bible
