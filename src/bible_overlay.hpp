#pragma once

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QString>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace bible {

struct OverlayState {
    bool visible = false;
    QString reference;
    QString verse_text;

    QString font_family = "Sans Serif";
    int font_size = 42;
    QColor text_color = QColor(255, 255, 255);
    QColor ref_color = QColor(0, 210, 168);
    QColor bg_color = QColor(12, 16, 24);
    int bg_opacity = 200; // 0..255
    int position = 0;     // 0=Bas, 1=Centre, 2=Haut
    int width = 1920;
    int height = 1080;
};

class SharedOverlay {
public:
    static SharedOverlay &instance()
    {
        static SharedOverlay inst;
        return inst;
    }

    void set_verse(const QString &ref, const QString &text)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            state_.reference = ref;
            state_.verse_text = text;
            state_.visible = !text.isEmpty();
            rev_.fetch_add(1, std::memory_order_release);
        }
        cv_.notify_all();
    }

    void clear()
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            state_.visible = false;
            rev_.fetch_add(1, std::memory_order_release);
        }
        cv_.notify_all();
    }

    void update_style(
        const QString &font_family,
        int font_size,
        const QColor &text_color,
        const QColor &ref_color,
        const QColor &bg_color,
        int bg_opacity,
        int position)
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            state_.font_family = font_family;
            state_.font_size = font_size;
            state_.text_color = text_color;
            state_.ref_color = ref_color;
            state_.bg_color = bg_color;
            state_.bg_opacity = bg_opacity;
            state_.position = position;
            rev_.fetch_add(1, std::memory_order_release);
        }
        cv_.notify_all();
    }

    uint64_t revision() const { return rev_.load(std::memory_order_acquire); }

    bool wait_for_change(uint64_t &last_rev, const std::atomic<bool> &running)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, std::chrono::milliseconds(100), [&]() {
            return !running.load(std::memory_order_acquire) ||
                   rev_.load(std::memory_order_acquire) != last_rev;
        });
        uint64_t cur = rev_.load(std::memory_order_acquire);
        if (cur != last_rev) {
            last_rev = cur;
            return true;
        }
        return false;
    }

    void wake() { cv_.notify_all(); }

    OverlayState snapshot() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return state_;
    }

private:
    SharedOverlay() = default;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    OverlayState state_;
    std::atomic<uint64_t> rev_{1};
};

inline void render_overlay_into(QImage &img, const OverlayState &st)
{
    if (img.width() != st.width || img.height() != st.height || img.format() != QImage::Format_ARGB32) {
        img = QImage(st.width, st.height, QImage::Format_ARGB32);
    }
    img.fill(Qt::transparent);

    if (!st.visible || st.verse_text.isEmpty())
        return;

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const int margin_x = static_cast<int>(st.width * 0.08);
    const int pad_x = 38;
    const int pad_y = 26;
    const int max_text_w = st.width - 2 * margin_x - 2 * pad_x;

    QFont text_font(st.font_family, st.font_size, QFont::Medium);
    QFont ref_font(st.font_family, std::max(18, static_cast<int>(st.font_size * 0.68)), QFont::Bold);

    QFontMetrics fm_text(text_font);
    QFontMetrics fm_ref(ref_font);

    QRect text_bound = fm_text.boundingRect(
        QRect(0, 0, max_text_w, st.height / 2),
        Qt::TextWordWrap | Qt::AlignLeft,
        st.verse_text);

    const int ref_h = st.reference.isEmpty() ? 0 : (fm_ref.height() + 12);
    const int box_w = std::min(st.width - 2 * margin_x, std::max(text_bound.width(), fm_ref.horizontalAdvance(st.reference)) + 2 * pad_x);
    const int box_h = text_bound.height() + ref_h + 2 * pad_y;

    const int box_x = (st.width - box_w) / 2;
    int box_y = st.height - box_h - 60;
    if (st.position == 1)
        box_y = (st.height - box_h) / 2;
    else if (st.position == 2)
        box_y = 60;

    if (st.bg_opacity > 0) {
        QColor bg = st.bg_color;
        bg.setAlpha(std::clamp(st.bg_opacity, 0, 255));
        QPainterPath path;
        path.addRoundedRect(QRectF(box_x, box_y, box_w, box_h), 18.0, 18.0);
        p.fillPath(path, bg);

        QColor border(255, 255, 255, std::min(60, st.bg_opacity / 3));
        p.setPen(QPen(border, 2.0));
        p.drawPath(path);
    }

    int cur_y = box_y + pad_y;
    if (!st.reference.isEmpty()) {
        p.setFont(ref_font);
        // Subtle drop shadow
        p.setPen(QColor(0, 0, 0, 180));
        p.drawText(QRect(box_x + pad_x + 2, cur_y + 2, box_w - 2 * pad_x, fm_ref.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, st.reference);
        p.setPen(st.ref_color);
        p.drawText(QRect(box_x + pad_x, cur_y, box_w - 2 * pad_x, fm_ref.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, st.reference);
        cur_y += ref_h;
    }

    p.setFont(text_font);
    QRect text_rect(box_x + pad_x, cur_y, box_w - 2 * pad_x, text_bound.height() + 10);
    p.setPen(QColor(0, 0, 0, 190));
    p.drawText(text_rect.translated(2, 2), Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, st.verse_text);
    p.setPen(st.text_color);
    p.drawText(text_rect, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, st.verse_text);
}

} // namespace bible
