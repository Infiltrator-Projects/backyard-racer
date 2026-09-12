// Bilinear scaler for photographic/static scene artwork.
// Included after pixel_app_part1.inc while SurfaceOwner/rgba are in scope.
SurfaceOwner scaled_copy_bilinear(const SurfaceOwner& source, int width, int height) {
    SurfaceOwner result(width, height);
    result.clear(rgba(0, 0, 0, 0));
    if (source.empty() || width <= 0 || height <= 0) return result;

    const int sw = source.width();
    const int sh = source.height();
    if (sw == 1 || sh == 1) {
        infiltratr_surface_blit_scaled_nearest(&result.get(), &source.get(), 0, 0, width, height);
        return result;
    }

    for (int y = 0; y < height; ++y) {
        const double sy = (height == 1) ? 0.0 : static_cast<double>(y) * (sh - 1) / (height - 1);
        const int y0 = static_cast<int>(sy);
        const int y1 = std::min(y0 + 1, sh - 1);
        const double fy = sy - y0;
        for (int x = 0; x < width; ++x) {
            const double sx = (width == 1) ? 0.0 : static_cast<double>(x) * (sw - 1) / (width - 1);
            const int x0 = static_cast<int>(sx);
            const int x1 = std::min(x0 + 1, sw - 1);
            const double fx = sx - x0;

            const Color c00 = infiltratr_surface_get_pixel(&source.get(), x0, y0);
            const Color c10 = infiltratr_surface_get_pixel(&source.get(), x1, y0);
            const Color c01 = infiltratr_surface_get_pixel(&source.get(), x0, y1);
            const Color c11 = infiltratr_surface_get_pixel(&source.get(), x1, y1);

            auto lerp2 = [fx, fy](std::uint8_t a00, std::uint8_t a10,
                                  std::uint8_t a01, std::uint8_t a11) -> std::uint8_t {
                const double top = a00 + (a10 - a00) * fx;
                const double bottom = a01 + (a11 - a01) * fx;
                const double value = top + (bottom - top) * fy;
                return static_cast<std::uint8_t>(std::clamp(std::lround(value), 0L, 255L));
            };

            infiltratr_surface_set_pixel(&result.get(), x, y,
                rgba(lerp2(c00.r,c10.r,c01.r,c11.r),
                     lerp2(c00.g,c10.g,c01.g,c11.g),
                     lerp2(c00.b,c10.b,c01.b,c11.b),
                     lerp2(c00.a,c10.a,c01.a,c11.a)));
        }
    }
    return result;
}
