// SPDX-License-Identifier: GPL-3.0-or-later
#include "pixel_app.hpp"

#include "gameplay.h"
#include "race_session.h"
#include "scene_embedded_assets.h"

#include <infiltratr/format.h>
#include <zlib.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace backyard_racer {
namespace {

using Color = InfiltratrColor;

constexpr Color rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) {
    return Color{r, g, b, a};
}

std::string fixed_ascii(double value, unsigned decimal_places) {
    char buffer[64]{};
    if (!infiltratr_format_fixed_ascii(value, decimal_places, buffer, sizeof(buffer))) {
        return "N/A";
    }
    return buffer;
}

class SurfaceOwner {
public:
    SurfaceOwner() = default;
    SurfaceOwner(int width, int height) { resize(width, height); }
    ~SurfaceOwner() { infiltratr_surface_release(&surface_); }

    SurfaceOwner(const SurfaceOwner&) = delete;
    SurfaceOwner& operator=(const SurfaceOwner&) = delete;

    SurfaceOwner(SurfaceOwner&& other) noexcept : surface_(other.surface_) {
        other.surface_ = {};
    }
    SurfaceOwner& operator=(SurfaceOwner&& other) noexcept {
        if (this != &other) {
            infiltratr_surface_release(&surface_);
            surface_ = other.surface_;
            other.surface_ = {};
        }
        return *this;
    }

    void resize(int width, int height) {
        if (width <= 0 || height <= 0 ||
            !infiltratr_surface_resize(&surface_, static_cast<std::size_t>(width),
                                      static_cast<std::size_t>(height))) {
            throw std::bad_alloc();
        }
    }

    bool empty() const { return !surface_.pixels || surface_.width == 0 || surface_.height == 0; }
    int width() const { return static_cast<int>(surface_.width); }
    int height() const { return static_cast<int>(surface_.height); }
    InfiltratrSurface& get() { return surface_; }
    const InfiltratrSurface& get() const { return surface_; }

    void clear(Color c) { infiltratr_surface_clear(&surface_, c); }
    void fill(int x, int y, int width, int height, Color c) {
        infiltratr_surface_fill_rect(&surface_, x, y, width, height, c);
    }
    void blend(int x, int y, int width, int height, Color c) {
        infiltratr_surface_blend_rect(&surface_, x, y, width, height, c);
    }
    void blit(const SurfaceOwner& source, int x, int y) {
        infiltratr_surface_blit(&surface_, &source.surface_, x, y);
    }
    void blit_scaled(const SurfaceOwner& source, int x, int y, int width, int height) {
        infiltratr_surface_blit_scaled_bilinear(&surface_, &source.surface_, x, y, width, height);
    }

private:
    InfiltratrSurface surface_{};
};

void outline(InfiltratrSurface& surface, int x, int y, int width, int height,
             Color color, int thickness = 1) {
    for (int i = 0; i < thickness; ++i) {
        infiltratr_surface_fill_rect(&surface, x + i, y + i, width - 2 * i, 1, color);
        infiltratr_surface_fill_rect(&surface, x + i, y + height - 1 - i,
                                     width - 2 * i, 1, color);
        infiltratr_surface_fill_rect(&surface, x + i, y + i, 1, height - 2 * i, color);
        infiltratr_surface_fill_rect(&surface, x + width - 1 - i, y + i,
                                     1, height - 2 * i, color);
    }
}

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    bool contains(int px, int py) const {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};

std::vector<std::uint8_t> decode_base64(std::string_view text) {
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::array<int, 256> map{};
    map.fill(-1);
    for (int i = 0; i < 64; ++i) map[static_cast<unsigned char>(alphabet[i])] = i;
    std::vector<std::uint8_t> out;
    out.reserve(text.size() * 3 / 4);
    unsigned value = 0;
    int bits = -8;
    for (char ch : text) {
        if (ch == '=' || ch == '\\') break;
        const int v = map[static_cast<unsigned char>(ch)];
        if (v < 0) continue;
        value = (value << 6U) | static_cast<unsigned>(v);
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<std::uint8_t>((value >> bits) & 255U));
            bits -= 8;
        }
    }
    return out;
}

SurfaceOwner load_indexed_asset(const scene_assets::IndexedAsset& asset) {
    const auto compressed = decode_base64(asset.zlib_b64);
    const std::size_t pixel_count = static_cast<std::size_t>(asset.w) * asset.h;
    const std::size_t rgb_expected = static_cast<std::size_t>(asset.colors) * 3U + pixel_count;
    const std::size_t rgba_expected = static_cast<std::size_t>(asset.colors) * 4U + pixel_count;
    std::vector<std::uint8_t> raw(std::max(rgb_expected, rgba_expected));
    uLongf raw_size = static_cast<uLongf>(raw.size());
    int result = uncompress(raw.data(), &raw_size, compressed.data(),
                            static_cast<uLong>(compressed.size()));
    while (result == Z_BUF_ERROR && raw.size() < (64U * 1024U * 1024U)) {
        raw.resize(raw.size() * 2U);
        raw_size = static_cast<uLongf>(raw.size());
        result = uncompress(raw.data(), &raw_size, compressed.data(),
                            static_cast<uLong>(compressed.size()));
    }
    if (result != Z_OK) {
        throw std::runtime_error("Embedded scene asset zlib decode failed");
    }
    raw.resize(static_cast<std::size_t>(raw_size));
    const bool rgba_palette = raw.size() == rgba_expected;
    if (!rgba_palette && raw.size() != rgb_expected) {
        throw std::runtime_error("Embedded scene asset has unexpected decoded size");
    }

    const std::size_t palette_stride = rgba_palette ? 4U : 3U;
    std::vector<Color> palette(static_cast<std::size_t>(asset.colors));
    for (int i = 0; i < asset.colors; ++i) {
        const std::size_t offset = static_cast<std::size_t>(i) * palette_stride;
        palette[static_cast<std::size_t>(i)] =
            rgba(raw[offset], raw[offset + 1], raw[offset + 2],
                 rgba_palette ? raw[offset + 3] : 255U);
    }

    SurfaceOwner image(asset.w, asset.h);
    image.clear(rgba(0, 0, 0, 0));
    const std::size_t pixel_offset = static_cast<std::size_t>(asset.colors) * palette_stride;
    for (int y = 0; y < asset.h; ++y) {
        for (int x = 0; x < asset.w; ++x) {
            const unsigned index = raw[pixel_offset + static_cast<std::size_t>(y) * asset.w + x];
            if (index == 255U) continue;
            infiltratr_surface_set_pixel(&image.get(), x, y, palette.at(index));
        }
    }
    return image;
}

SurfaceOwner scaled_copy(const SurfaceOwner& source, int width, int height) {
    SurfaceOwner result(width, height);
    result.clear(rgba(0, 0, 0, 0));
    infiltratr_surface_blit_scaled_bilinear(&result.get(), &source.get(), 0, 0, width, height);
    return result;
}

std::array<std::uint8_t, 7> glyph_rows(char c) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
#define G(a,b,c,d,e,f,g) return {a,b,c,d,e,f,g}
    switch (c) {
        case 'A': G(0x0e,0x11,0x11,0x1f,0x11,0x11,0x11);
        case 'B': G(0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e);
        case 'C': G(0x0e,0x11,0x10,0x10,0x10,0x11,0x0e);
        case 'D': G(0x1e,0x11,0x11,0x11,0x11,0x11,0x1e);
        case 'E': G(0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f);
        case 'F': G(0x1f,0x10,0x10,0x1e,0x10,0x10,0x10);
        case 'G': G(0x0e,0x11,0x10,0x17,0x11,0x11,0x0f);
        case 'H': G(0x11,0x11,0x11,0x1f,0x11,0x11,0x11);
        case 'I': G(0x0e,0x04,0x04,0x04,0x04,0x04,0x0e);
        case 'J': G(0x07,0x02,0x02,0x02,0x12,0x12,0x0c);
        case 'K': G(0x11,0x12,0x14,0x18,0x14,0x12,0x11);
        case 'L': G(0x10,0x10,0x10,0x10,0x10,0x10,0x1f);
        case 'M': G(0x11,0x1b,0x15,0x15,0x11,0x11,0x11);
        case 'N': G(0x11,0x19,0x15,0x13,0x11,0x11,0x11);
        case 'O': G(0x0e,0x11,0x11,0x11,0x11,0x11,0x0e);
        case 'P': G(0x1e,0x11,0x11,0x1e,0x10,0x10,0x10);
        case 'Q': G(0x0e,0x11,0x11,0x11,0x15,0x12,0x0d);
        case 'R': G(0x1e,0x11,0x11,0x1e,0x14,0x12,0x11);
        case 'S': G(0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e);
        case 'T': G(0x1f,0x04,0x04,0x04,0x04,0x04,0x04);
        case 'U': G(0x11,0x11,0x11,0x11,0x11,0x11,0x0e);
        case 'V': G(0x11,0x11,0x11,0x11,0x11,0x0a,0x04);
        case 'W': G(0x11,0x11,0x11,0x15,0x15,0x15,0x0a);
        case 'X': G(0x11,0x11,0x0a,0x04,0x0a,0x11,0x11);
        case 'Y': G(0x11,0x11,0x0a,0x04,0x04,0x04,0x04);
        case 'Z': G(0x1f,0x01,0x02,0x04,0x08,0x10,0x1f);
        case '0': G(0x0e,0x11,0x13,0x15,0x19,0x11,0x0e);
        case '1': G(0x04,0x0c,0x04,0x04,0x04,0x04,0x0e);
        case '2': G(0x0e,0x11,0x01,0x02,0x04,0x08,0x1f);
        case '3': G(0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e);
        case '4': G(0x02,0x06,0x0a,0x12,0x1f,0x02,0x02);
        case '5': G(0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e);
        case '6': G(0x0e,0x10,0x10,0x1e,0x11,0x11,0x0e);
        case '7': G(0x1f,0x01,0x02,0x04,0x08,0x08,0x08);
        case '8': G(0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e);
        case '9': G(0x0e,0x11,0x11,0x0f,0x01,0x01,0x0e);
        case '$': G(0x04,0x0f,0x14,0x0e,0x05,0x1e,0x04);
        case '-': G(0,0,0,0x1f,0,0,0);
        case '/': G(0x01,0x02,0x02,0x04,0x08,0x08,0x10);
        case ':': G(0,0x04,0x04,0,0x04,0x04,0);
        case '.': G(0,0,0,0,0,0x06,0x06);
        case '%': G(0x19,0x1a,0x02,0x04,0x08,0x0b,0x13);
        case '+': G(0,0x04,0x04,0x1f,0x04,0x04,0);
        case '?': G(0x0e,0x11,0x01,0x02,0x04,0,0x04);
        case '!': G(0x04,0x04,0x04,0x04,0x04,0,0x04);
        case ',': G(0,0,0,0,0,0x04,0x08);
        case '(': G(0x02,0x04,0x08,0x08,0x08,0x04,0x02);
        case ')': G(0x08,0x04,0x02,0x02,0x02,0x04,0x08);
        case ' ': default: G(0,0,0,0,0,0,0);
    }
#undef G
}

class GlyphAtlas {
public:
    GlyphAtlas() {
        base_.resize(static_cast<int>(characters_.size()) * cell_w_, cell_h_);
        base_.clear(rgba(0, 0, 0, 0));
        for (std::size_t i = 0; i < characters_.size(); ++i) {
            const auto rows = glyph_rows(characters_[i]);
            for (int y = 0; y < 7; ++y) {
                for (int x = 0; x < 5; ++x) {
                    if (rows[static_cast<std::size_t>(y)] & (1U << (4 - x))) {
                        infiltratr_surface_set_pixel(&base_.get(),
                            static_cast<int>(i) * cell_w_ + x, y, rgba(255, 255, 255));
                    }
                }