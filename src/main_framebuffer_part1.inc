// SPDX-License-Identifier: GPL-3.0-or-later
// Backyard Racer framebuffer presentation cutover.
//
// X11 is now a window/input/present layer for the migrated screens.  Menu,
// Classifieds and Garage are composed completely in software pixel memory,
// including bitmap-glyph text, image assets and garage vehicle animation.

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <infiltratr/core.h>
#include <infiltratr/timing.h>

#include "gameplay.h"
#include "race_session.h"
#include "generated_assets.h"
#include "scene_embedded_assets.h"
#include <zlib.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

// Reuse the proven gameplay/input/race loop while replacing presentation.
#define private protected
#define App LegacySceneApp
#define main backyard_racer_legacy_entry
#include "main_scene_pass.cpp"
#undef main
#undef App
#undef private

namespace backyard_racer {

namespace fb {

struct Color {
    std::uint8_t r = 0, g = 0, b = 0, a = 255;
};

static constexpr Color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return {r,g,b,255};
}

struct Image {
    int w = 0;
    int h = 0;
    std::vector<std::uint32_t> px; // 0xAARRGGBB
    bool empty() const { return w <= 0 || h <= 0 || px.empty(); }
};

static std::uint32_t pack(Color c) {
    return (static_cast<std::uint32_t>(c.a) << 24) |
           (static_cast<std::uint32_t>(c.r) << 16) |
           (static_cast<std::uint32_t>(c.g) << 8) |
            static_cast<std::uint32_t>(c.b);
}

static Color unpack(std::uint32_t p) {
    return {static_cast<std::uint8_t>((p >> 16) & 0xff),
            static_cast<std::uint8_t>((p >> 8) & 0xff),
            static_cast<std::uint8_t>(p & 0xff),
            static_cast<std::uint8_t>((p >> 24) & 0xff)};
}

static std::vector<std::uint8_t> decode_base64(std::string_view text) {
    static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::array<int,256> map{}; map.fill(-1);
    for(int i=0;i<64;++i) map[static_cast<unsigned char>(alphabet[i])] = i;
    std::vector<std::uint8_t> out; out.reserve(text.size()*3/4);
    unsigned value=0; int bits=-8;
    for(char ch:text){ if(ch=='=')break; const int v=map[static_cast<unsigned char>(ch)]; if(v<0)continue; value=(value<<6)|static_cast<unsigned>(v); bits+=6; if(bits>=0){out.push_back(static_cast<std::uint8_t>((value>>bits)&255U));bits-=8;} }
    return out;
}

static Image load_indexed_image(const scene_assets::IndexedAsset& asset) {
    const auto compressed=decode_base64(asset.zlib_b64);
    const std::size_t expected=static_cast<std::size_t>(asset.colors)*3U + static_cast<std::size_t>(asset.w)*asset.h;
    std::vector<std::uint8_t> raw(expected);
    uLongf raw_size=static_cast<uLongf>(raw.size());
    const int zr=uncompress(raw.data(),&raw_size,compressed.data(),static_cast<uLong>(compressed.size()));
    if(zr!=Z_OK || raw_size!=raw.size()) throw std::runtime_error("Embedded scene asset decode failed");
    std::vector<Color> palette(static_cast<std::size_t>(asset.colors));
    for(int i=0;i<asset.colors;++i){
        const std::size_t j=static_cast<std::size_t>(i)*3U;
        palette[static_cast<std::size_t>(i)]={raw[j],raw[j+1],raw[j+2],255};
    }
    Image out;out.w=asset.w;out.h=asset.h;out.px.resize(static_cast<std::size_t>(asset.w)*asset.h);
    const std::size_t offset=static_cast<std::size_t>(asset.colors)*3U;
    for(std::size_t i=0;i<out.px.size();++i){const unsigned idx=raw[offset+i];out.px[i]=idx==255U?0U:pack(palette.at(idx));}
    return out;
}

static Image scale_nearest(const Image& src, int dw, int dh) {
    if(src.empty() || dw<=0 || dh<=0) return {};
    Image out; out.w=dw; out.h=dh;
    out.px.resize(static_cast<std::size_t>(dw)*static_cast<std::size_t>(dh));
    for(int y=0;y<dh;++y){
        const int sy=std::clamp((y*src.h)/dh,0,src.h-1);
        for(int x=0;x<dw;++x){
            const int sx=std::clamp((x*src.w)/dw,0,src.w-1);
            out.px[static_cast<std::size_t>(y)*dw+x]=src.px[static_cast<std::size_t>(sy)*src.w+sx];
        }
    }
    return out;
}

class Surface {
public:
    void resize(int w, int h) {
        w_ = std::max(1,w); h_ = std::max(1,h);
        px_.resize(static_cast<std::size_t>(w_)*static_cast<std::size_t>(h_));
    }
    int width() const { return w_; }
    int height() const { return h_; }
    std::vector<std::uint32_t>& pixels() { return px_; }
    const std::vector<std::uint32_t>& pixels() const { return px_; }
    void clear(Color c) { std::fill(px_.begin(),px_.end(),pack(c)); }
    void copy_from(const Image& im) {
        if (im.w != w_ || im.h != h_) return;
        px_ = im.px;
    }
    void pixel(int x,int y,Color c) {
        if (x<0||y<0||x>=w_||y>=h_) return;
        px_[static_cast<std::size_t>(y)*w_+x]=pack(c);
    }
    void rect(int x,int y,int w,int h,Color c) {
        const int x0=std::clamp(x,0,w_), y0=std::clamp(y,0,h_);
        const int x1=std::clamp(x+w,0,w_), y1=std::clamp(y+h,0,h_);
        const auto p=pack(c);
        for(int yy=y0;yy<y1;++yy)
            std::fill(px_.begin()+static_cast<std::ptrdiff_t>(yy*w_+x0),
                      px_.begin()+static_cast<std::ptrdiff_t>(yy*w_+x1),p);
    }
    void outline(int x,int y,int w,int h,Color c,int t=1) {
        for(int i=0;i<t;++i){rect(x+i,y+i,w-2*i,1,c);rect(x+i,y+h-1-i,w-2*i,1,c);rect(x+i,y+i,1,h-2*i,c);rect(x+w-1-i,y+i,1,h-2*i,c);}
    }
    void line(int x0,int y0,int x1,int y1,Color c) {
        int dx=std::abs(x1-x0), sx=x0<x1?1:-1;
        int dy=-std::abs(y1-y0), sy=y0<y1?1:-1;
        int err=dx+dy;
        for(;;){pixel(x0,y0,c);if(x0==x1&&y0==y1)break;const int e2=2*err;if(e2>=dy){err+=dy;x0+=sx;}if(e2<=dx){err+=dx;y0+=sy;}}
    }
    void blit(const Image& src,int dx,int dy) {
        for(int sy=0;sy<src.h;++sy){
            const int y=dy+sy;if(y<0||y>=h_)continue;
            for(int sx=0;sx<src.w;++sx){
                const int x=dx+sx;if(x<0||x>=w_)continue;
                const Color s=unpack(src.px[static_cast<std::size_t>(sy)*src.w+sx]);
                if(s.a==0)continue;
                if(s.a==255){px_[static_cast<std::size_t>(y)*w_+x]=pack(s);continue;}
                const Color d=unpack(px_[static_cast<std::size_t>(y)*w_+x]);
                const unsigned a=s.a, ia=255-a;
                pixel(x,y,{static_cast<std::uint8_t>((s.r*a+d.r*ia)/255),static_cast<std::uint8_t>((s.g*a+d.g*ia)/255),static_cast<std::uint8_t>((s.b*a+d.b*ia)/255),255});
            }
        }
    }
    void alpha_rect(int x,int y,int w,int h,Color s) {
        const int x0=std::clamp(x,0,w_), y0=std::clamp(y,0,h_);
        const int x1=std::clamp(x+w,0,w_), y1=std::clamp(y+h,0,h_);
        const unsigned a=s.a,ia=255-a;
        for(int yy=y0;yy<y1;++yy)for(int xx=x0;xx<x1;++xx){
            const Color d=unpack(px_[static_cast<std::size_t>(yy)*w_+xx]);
