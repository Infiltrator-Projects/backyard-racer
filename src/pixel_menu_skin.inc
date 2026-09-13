    double menu_ui_scale() const {
        return std::clamp(std::min(width / 1280.0, height / 720.0), 0.50, 1.50);
    }

    Rect menu_new() const {
        const double s = menu_ui_scale();
        return {static_cast<int>(58 * s), static_cast<int>(286 * s),
                static_cast<int>(340 * s), static_cast<int>(58 * s)};
    }

    Rect menu_continue() const {
        const double s = menu_ui_scale();
        return {static_cast<int>(58 * s), static_cast<int>(352 * s),
                static_cast<int>(340 * s), static_cast<int>(58 * s)};
    }

    Rect menu_settings() const {
        const double s = menu_ui_scale();
        return {static_cast<int>(58 * s), static_cast<int>(418 * s),
                static_cast<int>(340 * s), static_cast<int>(58 * s)};
    }

    Rect menu_quit() const {
        const double s = menu_ui_scale();
        return {static_cast<int>(58 * s), static_cast<int>(484 * s),
                static_cast<int>(340 * s), static_cast<int>(58 * s)};
    }

    const SurfaceOwner& authored_menu_chrome() {
        static SurfaceOwner chrome = load_indexed_asset(scene_assets::menu_chrome);
        return chrome;
    }

    void authored_menu_button(Rect r, const std::string& label, bool enabled = true) {
        const bool hover = enabled && r.contains(mouse_x, mouse_y);
        const int source_y = !enabled ? 348 : (hover ? 284 : 220);
        const SurfaceOwner& chrome = authored_menu_chrome();
        ::backyard_surface_blit_region_scaled_smooth(
            &frame.get(), &chrome.get(), 0, source_y, 340, 58,
            r.x, r.y, r.w, r.h);

        const int text_scale = std::max(1, static_cast<int>(std::lround(2.0 * menu_ui_scale())));
        const GlyphAtlas::Tone tone = !enabled ? GlyphAtlas::Tone::Dark :
            (hover ? GlyphAtlas::Tone::Gold : GlyphAtlas::Tone::Pale);
        const int tx = r.x + static_cast<int>(42 * menu_ui_scale());
        const int th = 10 * text_scale;
        text(tx, r.y + (r.h - th) / 2, label, tone, text_scale);
    }

    void draw_menu() {
        if (!garage_scaled.empty()) infiltratr_surface_copy(&frame.get(), &garage_scaled.get());
        else frame.clear(rgba(37, 34, 31));

        // The garage remains the hero image.  The left side is only darkened
        // enough to carry the authored logo and menu chrome; there is no
        // rectangular DOS-style menu panel any more.
        frame.blend(0, 0, width, height, rgba(0, 0, 0, 12));
        const int fade_width = std::max(260, static_cast<int>(560 * menu_ui_scale()));
        const int strips = 14;
        for (int i = 0; i < strips; ++i) {
            const int x0 = i * fade_width / strips;
            const int x1 = (i + 1) * fade_width / strips;
            const double t = static_cast<double>(i) / (strips - 1);
            const int alpha = static_cast<int>(188.0 * std::pow(1.0 - t, 1.55));
            frame.blend(x0, 0, x1 - x0 + 1, height,
                        rgba(4, 5, 6, static_cast<std::uint8_t>(std::clamp(alpha, 0, 255))));
        }

        const double s = menu_ui_scale();
        const SurfaceOwner& chrome = authored_menu_chrome();
        ::backyard_surface_blit_region_scaled_smooth(
            &frame.get(), &chrome.get(), 0, 0, 440, 210,
            static_cast<int>(38 * s), static_cast<int>(42 * s),
            static_cast<int>(440 * s), static_cast<int>(210 * s));

        authored_menu_button(menu_new(), "New Game");
        authored_menu_button(menu_continue(), "Continue", game.started());
        authored_menu_button(menu_settings(), "Settings");
        authored_menu_button(menu_quit(), "Quit");

        const int footer_scale = std::max(1, static_cast<int>(std::lround(menu_ui_scale())));
        const int footer_x = static_cast<int>(60 * s);
        const int footer_y = static_cast<int>(654 * s);
        text(footer_x, footer_y,
             game.started() ? "Continue where you left off." : "Buy a car. Build it. Race it.",
             GlyphAtlas::Tone::Pale, footer_scale);
    }
