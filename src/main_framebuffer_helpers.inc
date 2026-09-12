// SPDX-License-Identifier: GPL-3.0-or-later
// Small framebuffer helpers kept separate while the presentation migration is split.

namespace fb {

static Image scale_nearest(const Image& src, int dw, int dh) {
    if (src.empty() || dw <= 0 || dh <= 0) return {};

    Image out;
    out.w = dw;
    out.h = dh;
    out.px.resize(static_cast<std::size_t>(dw) * static_cast<std::size_t>(dh));

    for (int y = 0; y < dh; ++y) {
        const int sy = std::clamp((y * src.h) / dh, 0, src.h - 1);
        for (int x = 0; x < dw; ++x) {
            const int sx = std::clamp((x * src.w) / dw, 0, src.w - 1);
            out.px[static_cast<std::size_t>(y) * static_cast<std::size_t>(dw) + static_cast<std::size_t>(x)] =
                src.px[static_cast<std::size_t>(sy) * static_cast<std::size_t>(src.w) + static_cast<std::size_t>(sx)];
        }
    }

    return out;
}

} // namespace fb
