#include <obs/obs-frontend-api.h>
#include <obs/obs-module.h>
#include <obs/obs.h>
#include <obs/util/platform.h>

#include <QMainWindow>
#include <QMetaObject>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

#include "bible_dock.hpp"
#include "bible_overlay.hpp"

OBS_DECLARE_MODULE()

MODULE_EXPORT const char *obs_module_description(void)
{
    return "OBS Scripture Dock — Native Qt6 Bible Dock & True-Alpha Verse Overlay (SWORD + JSON/JS)";
}

namespace {

struct BibleSourceContext {
    obs_source_t *source = nullptr;
    std::atomic<bool> running{true};
    std::thread worker;
    uint64_t last_rev = 0;
};

void push_frame(obs_source_t *source, const QImage &img)
{
    if (!source || img.isNull())
        return;

    struct obs_source_frame frame {};
    frame.width = static_cast<uint32_t>(img.width());
    frame.height = static_cast<uint32_t>(img.height());
    frame.format = VIDEO_FORMAT_BGRA;
    frame.full_range = true;
    frame.data[0] = const_cast<uint8_t *>(img.constBits());
    frame.linesize[0] = static_cast<uint32_t>(img.bytesPerLine());
    frame.timestamp = os_gettime_ns();
    obs_source_output_video(source, &frame);
}

const char *bible_source_get_name(void *)
{
    return "Bible Verse Overlay";
}

void *bible_source_create(obs_data_t *, obs_source_t *source)
{
    auto *ctx = new BibleSourceContext();
    ctx->source = source;
    ctx->worker = std::thread([ctx]() {
        QImage cached_img;
        // Push initial frame immediately
        auto st0 = bible::SharedOverlay::instance().snapshot();
        bible::render_overlay_into(cached_img, st0);
        push_frame(ctx->source, cached_img);
        ctx->last_rev = bible::SharedOverlay::instance().revision();

        while (ctx->running.load(std::memory_order_acquire)) {
            if (bible::SharedOverlay::instance().wait_for_change(ctx->last_rev, ctx->running)) {
                auto st = bible::SharedOverlay::instance().snapshot();
                bible::render_overlay_into(cached_img, st);
                push_frame(ctx->source, cached_img);
            }
        }
    });
    return ctx;
}

void bible_source_destroy(void *data)
{
    auto *ctx = static_cast<BibleSourceContext *>(data);
    ctx->running.store(false, std::memory_order_release);
    bible::SharedOverlay::instance().wake();
    if (ctx->worker.joinable())
        ctx->worker.join();
    delete ctx;
}

struct obs_source_info bible_source_info = {
    .id = "obs_bible_verse_source",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC,
    .get_name = bible_source_get_name,
    .create = bible_source_create,
    .destroy = bible_source_destroy,
};

} // namespace

bool obs_module_load(void)
{
    obs_register_source(&bible_source_info);
    auto *main_win = static_cast<QWidget *>(obs_frontend_get_main_window());
    if (main_win) {
        auto *dock = new bible::BibleDockWidget(main_win);
        obs_frontend_add_dock_by_id("obs_scripture_dock", "Scripture Dock", dock);
    }
    return true;
}

void obs_module_unload(void)
{
}
