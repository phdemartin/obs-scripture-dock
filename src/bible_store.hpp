#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#ifdef HAVE_SWORD
#include <swmgr.h>
#include <swmodule.h>
#include <versekey.h>
#endif

#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

namespace bible {

struct Verse {
    int book_id = 0;
    int chapter = 0;
    int verse = 0;
    QString text;
};

struct Book {
    int id = 0;
    QString name;
    std::map<int, std::vector<Verse>> chapters;
};

struct BibleData {
    QString name;
    QString abbr;
    QString path;
    std::vector<Book> books;
    std::map<int, size_t> book_index_by_id;

    const Book *find_book(int id) const
    {
        auto it = book_index_by_id.find(id);
        if (it == book_index_by_id.end())
            return nullptr;
        return &books[it->second];
    }

    const Verse *get_verse(int book_id, int chapter, int verse_num) const
    {
        const Book *b = find_book(book_id);
        if (!b)
            return nullptr;
        auto cit = b->chapters.find(chapter);
        if (cit == b->chapters.end())
            return nullptr;
        for (const auto &v : cit->second) {
            if (v.verse == verse_num)
                return &v;
        }
        return nullptr;
    }

    bool step_verse(int &book_id, int &chapter, int &verse_num, int delta) const
    {
        const Book *b = find_book(book_id);
        if (!b)
            return false;
        auto cit = b->chapters.find(chapter);
        if (cit == b->chapters.end())
            return false;
        const auto &vs = cit->second;
        int idx = -1;
        for (int i = 0; i < static_cast<int>(vs.size()); ++i) {
            if (vs[i].verse == verse_num) {
                idx = i;
                break;
            }
        }
        if (idx < 0)
            return false;
        int next_idx = idx + delta;
        if (next_idx >= 0 && next_idx < static_cast<int>(vs.size())) {
            verse_num = vs[next_idx].verse;
            return true;
        }
        // Step across chapters
        if (delta > 0) {
            auto next_cit = std::next(cit);
            if (next_cit != b->chapters.end() && !next_cit->second.empty()) {
                chapter = next_cit->first;
                verse_num = next_cit->second.front().verse;
                return true;
            }
        } else if (delta < 0 && cit != b->chapters.begin()) {
            auto prev_cit = std::prev(cit);
            if (!prev_cit->second.empty()) {
                chapter = prev_cit->first;
                verse_num = prev_cit->second.back().verse;
                return true;
            }
        }
        return false;
    }

    struct SearchHit {
        int book_id;
        QString book_name;
        int chapter;
        int verse;
        QString text;
    };

    static QString normalize_book_query(const QString &q)
    {
        QString s = q.trimmed().toLower();
        // Common French / Spanish -> English/French aliases for SWORD modules
        static const std::pair<const char *, const char *> aliases[] = {
            {"genese", "gen"}, {"genèse", "gen"}, {"genesis", "gen"}, {"génesis", "gen"},
            {"exode", "exod"}, {"exodo", "exod"}, {"éxodo", "exod"},
            {"levitique", "lev"}, {"lévitique", "lev"}, {"nombres", "num"}, {"numeros", "num"},
            {"deuteronome", "deut"}, {"deutéronome", "deut"}, {"josue", "josh"}, {"josué", "josh"},
            {"juges", "judg"}, {"jueces", "judg"}, {"psaumes", "ps"}, {"psaume", "ps"},
            {"salmos", "ps"}, {"salmo", "ps"}, {"proverbes", "prov"}, {"ecclesiaste", "eccl"},
            {"esaie", "isa"}, {"ésaïe", "isa"}, {"isaias", "isa"}, {"jeremie", "jer"}, {"jérémie", "jer"},
            {"ezechiel", "ezek"}, {"ézéchiel", "ezek"}, {"osee", "hos"}, {"osée", "hos"},
            {"matthieu", "matt"}, {"mateo", "matt"}, {"marc", "mark"}, {"marcos", "mark"},
            {"luc", "luke"}, {"lucas", "luke"}, {"jean", "john"}, {"juan", "john"}, {"jn", "john"},
            {"actes", "acts"}, {"hechos", "acts"}, {"romains", "rom"}, {"romanos", "rom"},
            {"corinthiens", "cor"}, {"corintios", "cor"}, {"galates", "gal"}, {"ephesiens", "eph"},
            {"éphésiens", "eph"}, {"philippiens", "phil"}, {"colossiens", "col"},
            {"thessaloniciens", "thess"}, {"timothee", "tim"}, {"timothée", "tim"},
            {"tite", "titus"}, {"hebreux", "heb"}, {"hébreux", "heb"}, {"jacques", "jas"},
            {"santiago", "jas"}, {"pierre", "pet"}, {"pedro", "pet"}, {"apocalypse", "rev"},
            {"apocalipsis", "rev"},
        };
        for (const auto &kv : aliases) {
            if (s == kv.first)
                return QString::fromUtf8(kv.second);
        }
        return s;
    }

    std::vector<SearchHit> search(const QString &query, int max_results = 80) const
    {
        std::vector<SearchHit> hits;
        QString q = query.trimmed();
        if (q.isEmpty())
            return hits;

        // Check reference pattern: e.g. "Jean 1:3", "Jean 1:3-6", "1 Cor 13:4-7", "Gen 1"
        int colon = q.lastIndexOf(':');
        int space_before = (colon > 0) ? q.lastIndexOf(' ', colon) : q.lastIndexOf(' ');
        if (space_before > 0) {
            QString book_raw = q.left(space_before).trimmed().toLower();
            QString book_norm = normalize_book_query(book_raw);
            QString num_part = q.mid(space_before + 1).trimmed();
            int ch = 0;
            int vs_start = 0;
            int vs_end = 0;
            if (colon > space_before) {
                ch = num_part.section(':', 0, 0).toInt();
                QString vrange = num_part.section(':', 1, 1).trimmed();
                if (vrange.contains('-')) {
                    vs_start = vrange.section('-', 0, 0).toInt();
                    vs_end = vrange.section('-', 1, 1).toInt();
                    if (vs_end < vs_start)
                        vs_end = vs_start;
                } else {
                    vs_start = vrange.toInt();
                    vs_end = vs_start;
                }
            } else {
                ch = num_part.toInt();
            }
            if (ch > 0) {
                for (const auto &b : books) {
                    QString bname = b.name.toLower();
                    if (bname.contains(book_raw) || bname.contains(book_norm)) {
                        auto cit = b.chapters.find(ch);
                        if (cit != b.chapters.end()) {
                            for (const auto &v : cit->second) {
                                if (vs_start == 0 || (v.verse >= vs_start && v.verse <= vs_end)) {
                                    hits.push_back({b.id, b.name, ch, v.verse, v.text});
                                    if (static_cast<int>(hits.size()) >= max_results)
                                        return hits;
                                }
                            }
                        }
                    }
                }
                if (!hits.empty())
                    return hits;
            }
        }

        // Fallback: keyword text search
        for (const auto &b : books) {
            for (const auto &ch_pair : b.chapters) {
                for (const auto &v : ch_pair.second) {
                    if (v.text.contains(q, Qt::CaseInsensitive) ||
                        b.name.contains(q, Qt::CaseInsensitive)) {
                        hits.push_back({b.id, b.name, ch_pair.first, v.verse, v.text});
                        if (static_cast<int>(hits.size()) >= max_results)
                            return hits;
                    }
                }
            }
        }
        return hits;
    }
};

inline QStringList discover_bible_files(const QString &custom_dir = {})
{
    QStringList dirs;
    if (!custom_dir.isEmpty())
        dirs << custom_dir;
    dirs << QDir::homePath() + "/obs-bible-plugin/obs-bible-plugin-dock/bibles"
         << QDir::homePath() + "/.config/obs-studio/bibles"
         << "/usr/share/obs-bible-plugin/bibles";

    QStringList files;
    QStringList seen_names;
    for (const auto &dpath : dirs) {
        QDir dir(dpath);
        if (!dir.exists())
            continue;
        const auto entries = dir.entryInfoList({"*.js", "*.json"}, QDir::Files, QDir::Name);
        for (const auto &fi : entries) {
            if (!seen_names.contains(fi.baseName())) {
                seen_names << fi.baseName();
                files << fi.absoluteFilePath();
            }
        }
    }
    return files;
}

inline bool load_bible_file(const QString &path, BibleData &out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QByteArray raw = f.readAll();
    int start = raw.indexOf('{');
    int end = raw.lastIndexOf('}');
    if (start < 0 || end <= start)
        return false;

    QJsonParseError err {};
    QJsonDocument doc = QJsonDocument::fromJson(raw.mid(start, end - start + 1), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    QJsonObject obj = doc.object();
    out = BibleData {};
    out.path = path;
    out.name = obj.value("name").toString(QFileInfo(path).baseName());
    out.abbr = obj.value("abbreviation").toString();

    const QJsonArray books_arr = obj.value("books").toArray();
    for (const auto &bv : books_arr) {
        QJsonObject bo = bv.toObject();
        Book b;
        b.id = bo.value("id").toInt();
        b.name = bo.value("name").toString();
        out.book_index_by_id[b.id] = out.books.size();
        out.books.push_back(std::move(b));
    }

    const QJsonArray sc_arr = obj.value("scriptures").toArray();
    for (const auto &sv : sc_arr) {
        QJsonObject so = sv.toObject();
        Verse v;
        v.book_id = so.value("bookId").toInt();
        v.chapter = so.value("chapter").toInt();
        v.verse = so.value("verse").toInt();
        v.text = so.value("text").toString();
        auto it = out.book_index_by_id.find(v.book_id);
        if (it != out.book_index_by_id.end()) {
            out.books[it->second].chapters[v.chapter].push_back(std::move(v));
        }
    }
    return !out.books.empty();
}

struct SwordModuleEntry {
    QString name;
    QString description;
};

inline std::vector<SwordModuleEntry> discover_sword_bibles()
{
    std::vector<SwordModuleEntry> list;
#ifdef HAVE_SWORD
#ifdef _WIN32
    QByteArray sword_path = (QDir::homePath() + "/.sword").toUtf8();
    sword::SWMgr mgr(sword_path.constData());
#else
    sword::SWMgr mgr;
#endif
    for (const auto &kv : mgr.Modules) {
        sword::SWModule *mod = kv.second;
        if (!mod || !mod->getType())
            continue;
        if (std::strcmp(mod->getType(), "Biblical Texts") != 0)
            continue;
        QString mname = QString::fromUtf8(mod->getName());
        QString desc = mod->getDescription() ? QString::fromUtf8(mod->getDescription()) : mname;
        list.push_back({mname, desc});
    }
    std::sort(list.begin(), list.end(), [](const SwordModuleEntry &a, const SwordModuleEntry &b) {
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
#endif
    return list;
}

inline QString localized_book_name(int testament, int book_num, const QString &lang, const char *fallback)
{
    // Canonical 1..39 (OT) + 1..27 (NT) -> index 0..65
    int idx = (testament == 1) ? (book_num - 1) : (testament == 2 ? (39 + book_num - 1) : -1);
    if (idx < 0 || idx >= 66)
        return fallback ? QString::fromUtf8(fallback) : QString();

    static const char *const FR_BOOKS[66] = {
        "Genèse", "Exode", "Lévitique", "Nombres", "Deutéronome", "Josué", "Juges", "Ruth",
        "1 Samuel", "2 Samuel", "1 Rois", "2 Rois", "1 Chroniques", "2 Chroniques", "Esdras",
        "Néhémie", "Esther", "Job", "Psaumes", "Proverbes", "Ecclésiaste", "Cantique des cantiques",
        "Ésaïe", "Jérémie", "Lamentations", "Ézéchiel", "Daniel", "Osée", "Joël", "Amos",
        "Abdias", "Jonas", "Michée", "Nahum", "Habacuc", "Sophonie", "Aggée", "Zacharie", "Malachie",
        "Matthieu", "Marc", "Luc", "Jean", "Actes", "Romains", "1 Corinthiens", "2 Corinthiens",
        "Galates", "Éphésiens", "Philippiens", "Colossiens", "1 Thessaloniciens", "2 Thessaloniciens",
        "1 Timothée", "2 Timothée", "Tite", "Philémon", "Hébreux", "Jacques", "1 Pierre", "2 Pierre",
        "1 Jean", "2 Jean", "3 Jean", "Jude", "Apocalypse"
    };
    static const char *const ES_BOOKS[66] = {
        "Génesis", "Éxodo", "Levítico", "Números", "Deuteronomio", "Josué", "Jueces", "Rut",
        "1 Samuel", "2 Samuel", "1 Reyes", "2 Reyes", "1 Crónicas", "2 Crónicas", "Esdras",
        "Nehemías", "Ester", "Job", "Salmos", "Proverbios", "Eclesiastés", "Cantares",
        "Isaías", "Jeremías", "Lamentaciones", "Ezequiel", "Daniel", "Oseas", "Joel", "Amós",
        "Abdías", "Jonás", "Miqueas", "Nahúm", "Habacuc", "Sofonías", "Hageo", "Zacarías", "Malaquías",
        "Mateo", "Marcos", "Lucas", "Juan", "Hechos", "Romanos", "1 Corintios", "2 Corintios",
        "Gálatas", "Efesios", "Filipenses", "Colosenses", "1 Tesalonicenses", "2 Tesalonicenses",
        "1 Timoteo", "2 Timoteo", "Tito", "Filemón", "Hebreos", "Santiago", "1 Pedro", "2 Pedro",
        "1 Juan", "2 Juan", "3 Juan", "Judas", "Apocalipsis"
    };

    QString l = lang.trimmed().toLower();
    if (l.startsWith("fr"))
        return QString::fromUtf8(FR_BOOKS[idx]);
    if (l.startsWith("es"))
        return QString::fromUtf8(ES_BOOKS[idx]);
    return fallback ? QString::fromUtf8(fallback) : QString();
}

inline bool load_sword_module(const QString &mod_name, BibleData &out)
{
#ifndef HAVE_SWORD
    (void)mod_name;
    (void)out;
    return false;
#else
#ifdef _WIN32
    QByteArray sword_path = (QDir::homePath() + "/.sword").toUtf8();
    sword::SWMgr mgr(sword_path.constData());
#else
    sword::SWMgr mgr;
#endif
    sword::SWModule *mod = mgr.getModule(mod_name.toUtf8().constData());
    if (!mod)
        return false;

    const char *enc_entry = mod->getConfigEntry("Encoding");
    const char *lang_entry = mod->getConfigEntry("Lang");
    QString mod_lang = lang_entry ? QString::fromLatin1(lang_entry) : QString("en");
    const bool is_utf8 = enc_entry && QString::fromLatin1(enc_entry).compare("UTF-8", Qt::CaseInsensitive) == 0;
    auto decode = [is_utf8](const char *raw) -> QString {
        if (!raw)
            return {};
        if (is_utf8)
            return QString::fromUtf8(raw);
        QString u = QString::fromUtf8(raw);
        if (!u.contains(QChar::ReplacementCharacter))
            return u;
        return QString::fromLatin1(raw);
    };

    out = BibleData {};
    out.path = "sword://" + mod_name;
    out.abbr = mod_name;
    out.name = mod->getDescription() ? decode(mod->getDescription()) : mod_name;

    mod->setSkipConsecutiveLinks(true);
    auto *vk = dynamic_cast<sword::VerseKey *>(mod->getKey());
    if (!vk)
        return false;

    for (*mod = sword::TOP; !mod->popError(); (*mod)++) {
        int book_id = vk->getTestament() * 100 + vk->getBook();
        int ch = vk->getChapter();
        int vs = vk->getVerse();
        if (ch <= 0 || vs <= 0)
            continue;

        QString text = decode(mod->stripText()).simplified();
        if (text.isEmpty())
            continue;

        auto it = out.book_index_by_id.find(book_id);
        if (it == out.book_index_by_id.end()) {
            Book b;
            b.id = book_id;
            b.name = localized_book_name(vk->getTestament(), vk->getBook(), mod_lang, vk->getBookName());
            out.book_index_by_id[book_id] = out.books.size();
            out.books.push_back(std::move(b));
            it = out.book_index_by_id.find(book_id);
        }
        out.books[it->second].chapters[ch].push_back({book_id, ch, vs, std::move(text)});
    }
    return !out.books.empty();
#endif
}

} // namespace bible
