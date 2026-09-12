// SPDX-License-Identifier: GPL-3.0-or-later
// Higher fidelity presentation pass for Backyard Racer.
//
// The original scene renderer remains the tested gameplay/UI implementation.
// This translation unit exposes its internals to a presentation subclass, then
// replaces the generic car silhouette and simple workshop on the major screens
// with model-specific 1960s/1970s profiles and a more believable garage scene.

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <infiltratr/core.h>
#include <infiltratr/timing.h>
#include "gameplay.h"
#include "race_session.h"
#include "generated_assets.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

// main_scene_pass.cpp deliberately keeps its state private.  This wrapper is a
// presentation-only adapter in the same spirit as the earlier art pass: expose
// that state only inside this translation unit so gameplay/race logic remains a
// single implementation.
#define private protected
#define App LegacySceneApp
#define main backyard_racer_legacy_entry
#include "main_scene_pass.cpp"
#undef main
#undef App
#undef private

namespace backyard_racer {

class RealisticApp final : public LegacySceneApp {
public:
    using LegacySceneApp::LegacySceneApp;

    int run() {
        draw_realistic();
        while (running_) {
            if (screen_ == Screen::Race) {
                bool redraw = false;
                while (XPending(dpy_) > 0) {
                    XEvent ev{};
                    XNextEvent(dpy_, &ev);
                    process_event(ev);
                    redraw = true;
                }
                redraw = tick_race() || redraw;
                if (redraw) draw_realistic();
                if (screen_ == Screen::Race)
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } else {
                XEvent ev{};
                XNextEvent(dpy_, &ev);
                process_event(ev);
                draw_realistic();
            }
        }
        return 0;
    }

private:
    struct CarProfile {
        std::array<std::pair<int,int>, 12> body;
        int rear_wheel_x;
        int front_wheel_x;
        int wheel_pct;
        int cabin_front_x;
        int cabin_rear_x;
        int roof_front_x;
        int roof_rear_x;
        int roof_y;
        int belt_y;
        int grille_height;
        int headlamps;
        bool fastback;
        bool hood_stripe;
    };

    static CarProfile profile_for(const CarSpec& car) {
        // Normalised side profiles are intentionally distinct and model-led:
        // boxy Falcon/Nova, pony-car Mustang/Camaro, fastback Charger, muscular
        // GTO/Road Runner, and low/wide Challenger.  Values are percentages of
        // the rendered width/height and are based on each real car's recognisable
        // proportions rather than one shared cartoon body.
        if (car.id.find("falcon") != std::string::npos) {
            return {{{{2,61},{7,49},{18,45},{34,44},{38,18},{67,18},{72,42},{89,45},{97,53},{98,68},{91,73},{7,73}}},
                    23,76,7,38,69,42,65,18,43,9,2,false,false};
        }
        if (car.id.find("mustang") != std::string::npos || car.id.find("boss") != std::string::npos) {
            return {{{{2,62},{7,50},{23,44},{38,41},{45,17},{67,17},{75,39},{92,46},{98,55},{97,69},{90,73},{6,73}}},
                    24,77,7,43,70,47,66,17,42,10,2,false,car.id.find("boss") != std::string::npos};
        }
        if (car.id.find("nova") != std::string::npos) {
            return {{{{2,61},{8,49},{21,45},{35,43},{40,20},{66,20},{72,41},{90,45},{97,54},{98,69},{91,73},{6,73}}},
                    24,76,7,39,69,43,65,20,43,9,2,false,false};
        }
        if (car.id.find("camaro") != std::string::npos) {
            return {{{{1,63},{6,51},{22,46},{39,42},{46,19},{68,19},{76,42},{93,48},{99,57},{97,70},{91,74},{5,74}}},
                    24,78,7,44,71,48,67,19,43,11,2,false,true};
        }
        if (car.id.find("charger") != std::string::npos) {
            return {{{{1,61},{6,49},{20,44},{37,41},{44,17},{64,17},{79,42},{94,48},{99,57},{97,70},{90,74},{5,74}}},
                    23,78,7,43,77,47,62,17,43,12,0,true,false};
        }
        if (car.id.find("gto") != std::string::npos) {
            return {{{{1,62},{7,49},{22,44},{38,42},{45,19},{67,19},{75,41},{92,47},{99,56},{97,70},{90,74},{5,74}}},
                    24,77,7,43,70,47,66,19,43,11,4,false,false};
        }
        if (car.id.find("roadrunner") != std::string::npos) {
            return {{{{1,61},{7,48},{21,44},{37,42},{42,19},{68,19},{75,42},{92,46},{99,55},{97,70},{90,74},{5,74}}},
                    23,77,7,41,71,45,67,19,43,10,2,false,true};
        }
        if (car.id.find("challenger") != std::string::npos) {
            return {{{{1,64},{5,52},{22,46},{40,42},{47,20},{68,20},{77,43},{94,49},{99,58},{97,71},{91,75},{4,75}}},
                    24,79,7,45,71,49,67,20,44,12,4,false,true};
        }
        if (car.id.find("cougar") != std::string::npos) {
            return {{{{1,62},{6,50},{22,44},{39,41},{46,18},{67,18},{75,41},{93,47},{99,56},{97,70},{90,74},{5,74}}},
                    24,78,7,44,70,48,66,18,42,11,0,false,false};
        }
        return {{{{2,62},{7,50},{22,45},{38,42},{45,19},{67,19},{75,42},{92,47},{98,56},{97,70},{90,74},{6,74}}},
                24,77,7,43,70,47,66,19,43,10,2,false,false};
    }

    static std::array<unsigned,3> paint_for(const CarSpec& car) {
        if (car.id.find("falcon") != std::string::npos) return {204,198,164};
        if (car.id.find("mustang") != std::string::npos || car.id.find("boss") != std::string::npos) return {177,36,32};
        if (car.id.find("nova") != std::string::npos) return {48,91,130};
        if (car.id.find("camaro") != std::string::npos) return {210,88,29};
        if (car.id.find("charger") != std::string::npos) return {46,79,58};
        if (car.id.find("gto") != std::string::npos) return {53,95,148};
        if (car.id.find("roadrunner") != std::string::npos) return {214,171,38};
        if (car.id.find("challenger") != std::string::npos) return {104,54,119};
        if (car.id.find("cougar") != std::string::npos) return {97,44,34};
        return {145,42,38};
    }

    void ellipse(int x, int y, int w, int h, unsigned r, unsigned g, unsigned b) {
        color(r,g,b);
        XFillArc(dpy_, canvas(), gc_, x, y, std::max(1,w), std::max(1,h), 0, 360 * 64);
    }

    void real_wheel(int cx, int cy, int radius) {
        ellipse(cx-radius-2, cy-radius-2, radius*2+4, radius*2+4, 13,13,13);
        ellipse(cx-radius+3, cy-radius+3, radius*2-6, radius*2-6, 46,46,45);
        ellipse(cx-radius*3/5, cy-radius*3/5, radius*6/5, radius*6/5, 190,190,184);
        ellipse(cx-radius*2/5, cy-radius*2/5, radius*4/5, radius*4/5, 84,86,84);
        ellipse(cx-radius/7, cy-radius/7, radius*2/7, radius*2/7, 210,210,202);
        for (int a = 0; a < 4; ++a) {
            const int dx = (a < 2 ? 1 : -1) * radius * (a % 2 ? 2 : 1) / 5;
            const int dy = (a < 2 ? -1 : 1) * radius * (a % 2 ? 1 : 2) / 5;
            line(cx,cy,cx+dx,cy+dy,160,160,154,2);
        }
    }

    void draw_model_car(const CarSpec& car, int cx, int base_y, int w, bool small = false) {
        const CarProfile p = profile_for(car);
        const auto paint = paint_for(car);
        const unsigned br = paint[0], bg = paint[1], bb = paint[2];
        const int h = std::max(small ? 54 : 105, w * 29 / 100);
        const int x = cx - w/2;
        const int top = base_y - h;
        const int wr = std::max(small ? 11 : 22, w * p.wheel_pct / 100);
        const int wy = base_y - wr;
        const int rx = x + w * p.rear_wheel_x / 100;
        const int fx = x + w * p.front_wheel_x / 100;

        // Soft ground shadow gives the vehicle weight in the scene.
        ellipse(x + w*5/100, base_y-wr/2, w*90/100, std::max(6,wr/2), 30,29,27);

        XPoint body[12]{};
        for (std::size_t i=0;i<p.body.size();++i) {
            body[i].x = static_cast<short>(x + w * p.body[i].first / 100);
            body[i].y = static_cast<short>(top + h * p.body[i].second / 100);
        }
        color(br,bg,bb);
        XFillPolygon(dpy_,canvas(),gc_,body,12,Complex,CoordModeOrigin);

        // Dark lower sill, body highlight and chrome rocker trim.
        fill({x+w*6/100, top+h*59/100, w*88/100, std::max(3,h*4/100)},
             std::max(0U,br*3/4),std::max(0U,bg*3/4),std::max(0U,bb*3/4));
        line(x+w*8/100,top+h*47/100,x+w*91/100,top+h*47/100,
             std::min(255U,br+55),std::min(255U,bg+55),std::min(255U,bb+55), small?1:2);
        fill({x+w*8/100, top+h*68/100, w*83/100, std::max(2,h*3/100)}, 187,187,178);

        // Cabin glass follows the real model's roof and beltline proportions.
        XPoint glass[6] = {
            {static_cast<short>(x+w*p.cabin_front_x/100), static_cast<short>(top+h*p.belt_y/100)},
            {static_cast<short>(x+w*p.roof_front_x/100),  static_cast<short>(top+h*(p.roof_y+3)/100)},
            {static_cast<short>(x+w*p.roof_rear_x/100),   static_cast<short>(top+h*(p.roof_y+3)/100)},
            {static_cast<short>(x+w*p.cabin_rear_x/100),  static_cast<short>(top+h*p.belt_y/100)},
            {static_cast<short>(x+w*(p.cabin_rear_x-2)/100),static_cast<short>(top+h*(p.belt_y+2)/100)},
            {static_cast<short>(x+w*(p.cabin_front_x+2)/100),static_cast<short>(top+h*(p.belt_y+2)/100)}
        };
        color(31,48,56); XFillPolygon(dpy_,canvas(),gc_,glass,6,Complex,CoordModeOrigin);
        line(x+w*(p.cabin_front_x+8)/100,top+h*(p.roof_y+4)/100,
             x+w*(p.cabin_front_x+8)/100,top+h*(p.belt_y+1)/100,126,145,148,small?1:2);
        if (!p.fastback)
            line(x+w*(p.cabin_rear_x-10)/100,top+h*(p.roof_y+4)/100,
                 x+w*(p.cabin_rear_x-10)/100,top+h*(p.belt_y+1)/100,126,145,148,small?1:2);

        // Door shut lines and handle.
        const int doorx = x+w*56/100;
        line(doorx,top+h*43/100,doorx,top+h*68/100,42,37,34,small?1:2);
        line(x+w*64/100,top+h*50/100,x+w*68/100,top+h*50/100,214,209,193,small?1:2);

        // Model cues at the nose: hidden-grille cars stay dark; GTO/Challenger
        // get quad lamps, others receive their correct pair.
        fill({x+w*93/100,top+h*48/100,w*5/100,std::max(5,h*p.grille_height/100)}, 27,28,27);
        if (p.headlamps > 0) {
            const int count = p.headlamps;
            for (int i=0;i<count;++i) {
                const int lampx = x+w*94/100 + (i%2)*(small?4:7);
                const int lampy = top+h*51/100 + (i/2)*(small?5:8);
                ellipse(lampx,lampy,std::max(3,w/90),std::max(3,w/90),232,220,171);
            }
        }
        fill({x+w*97/100,top+h*68/100,std::max(3,w*2/100),std::max(3,h*4/100)}, 196,196,186);
        fill({x+w*1/100,top+h*67/100,w*5/100,std::max(3,h*4/100)}, 190,190,181);

        if (p.hood_stripe && !small)
            fill({x+w*72/100,top+h*42/100,w*16/100,std::max(4,h*4/100)}, 28,27,26);

        real_wheel(rx,wy,wr); real_wheel(fx,wy,wr);

        // Wheel-arch lips and bumper lines.
        color(205,205,194);
        XDrawArc(dpy_,canvas(),gc_,rx-wr-3,wy-wr-3,(wr+3)*2,(wr+3)*2,0,180*64);
        XDrawArc(dpy_,canvas(),gc_,fx-wr-3,wy-wr-3,(wr+3)*2,(wr+3)*2,0,180*64);
        line(x+w*2/100,base_y-4,x+w*10/100,base_y-4,205,205,195,small?1:2);
        line(x+w*89/100,base_y-4,x+w*98/100,base_y-4,205,205,195,small?1:2);
    }

    void draw_real_workshop() {
        const int floor_y = height_ * 69 / 100;
        fill({0,0,width_,height_},21,22,23);

        // Ceiling / steel beams.
        fill({0,64,width_,42},38,39,39);
        for (int x=0;x<width_;x+=180) fill({x,64,12,42},27,28,28);

        // Painted masonry wall with subdued block joints.
        fill({0,106,width_,floor_y-106},82,82,78);
        for (int y=106;y<floor_y;y+=34) {
            line(0,y,width_,y,60,60,57);
            const int off=((y-106)/34)%2?55:0;
            for (int x=off;x<width_;x+=110) line(x,y,x,std::min(y+34,floor_y),64,64,60);
        }
        fill({0,floor_y-95,width_,95},67,68,66);

        // Two fluorescent fixtures with small halos.
        for (int cx : {width_/3, width_*2/3}) {
            fill({cx-170,78,340,20},80,80,75);
            fill({cx-155,82,310,12},224,221,192);
            fill({cx-130,84,260,8},249,247,220);
        }

        // Roller door behind the car gives the room believable scale.
        const int door_x=width_/2-310, door_w=620;
        fill({door_x,126,door_w,floor_y-126},74,77,76);
        outline({door_x,126,door_w,floor_y-126},44,45,45,5);
        for(int y=142;y<floor_y;y+=30) line(door_x+3,y,door_x+door_w-4,y,52,54,53,2);
        fill({door_x+12,136,22,floor_y-150},60,62,61);
        fill({door_x+door_w-34,136,22,floor_y-150},60,62,61);

        // Left-side tall red tool chest and air compressor.
        fill({34,floor_y-238,170,218},115,30,27); outline({34,floor_y-238,170,218},42,38,35,3);
        fill({45,floor_y-224,148,32},156,42,35);
        for(int y=floor_y-180;y<floor_y-35;y+=34){
            fill({48,y,142,27},96,27,25); line(101,y+13,137,y+13,205,202,188,2);
        }
        ellipse(220,floor_y-145,58,128,58,77,84); outline({214,floor_y-142,70,128},44,49,51,2);
        fill({233,floor_y-166,18,24},39,40,40); line(251,floor_y-154,283,floor_y-154,126,128,123,3);

        // Right-side bench, pegboard and real-ish tool silhouettes.
        const int bx=width_-330;
        fill({bx,174,275,150},103,77,51); fill({bx+16,194,243,112},57,52,46);
        for(int yy=211;yy<298;yy+=22) for(int xx=bx+30;xx<bx+250;xx+=28) ellipse(xx,yy,2,2,104,95,80);
        outline({bx,174,275,150},145,118,85,3);
        // Hanging combination wrenches, hammer and pliers.
        for(int i=0;i<4;++i){
            const int tx=bx+42+i*48; line(tx,205,tx,274,187,184,173,4);
            ellipse(tx-6,198,12,12,187,184,173); ellipse(tx-3,201,6,6,57,52,46);
        }
        line(bx+218,207,bx+218,275,184,181,170,5); line(bx+198,220,bx+238,220,184,181,170,5);
        fill({bx-18,324,310,24},91,62,39); fill({bx,348,25,110},67,48,34); fill({bx+249,348,25,110},67,48,34);
        fill({bx+40,360,165,56},72,72,68); outline({bx+40,360,165,56},42,42,40,2);
        for(int i=0;i<4;++i) line(bx+58,bx?374+i*11:374,bx+187,374+i*11,129,126,116,1);

        // Floor with perspective joints, drain and stains.
        fill({0,floor_y,width_,height_-floor_y},55,55,53);
        for(int y=floor_y+34;y<height_;y+=42) line(0,y,width_,y,46,46,44);
        for(int x=-width_/2;x<width_*3/2;x+=130) line(width_/2,floor_y,x,height_,48,48,46);
        ellipse(width_/2-95,floor_y+92,190,35,43,42,40);
        fill({width_/2-22,floor_y+135,44,14},35,36,35);
        for(int i=0;i<5;++i) line(width_/2-20+i*10,floor_y+138,width_/2-20+i*10,floor_y+146,83,84,80);

        // Tyres and small shop clutter.
        for(int i=0;i<3;++i) real_wheel(width_-85-i*8,floor_y-18-i*32,25);
        fill({245,floor_y-55,70,35},101,87,62); outline({245,floor_y-55,70,35},56,50,40,2);
        fill({319,floor_y-42,43,22},124,48,34);
    }

    const CarSpec* classified_car_at(int cx, int base_y) const {
        const auto& cars = game_.classifieds();
        if (cars.empty()) return nullptr;
        const int left=48, top=112, gap=14, card_w=(width_-110)/2, card_h=112;
        int col = cx > width_/2 ? 1 : 0;
        int row = (base_y - top) / (card_h + 10);
        row = std::clamp(row,0,3);
        const std::size_t index=static_cast<std::size_t>(row*2+col);
        return index<cars.size()?&cars[index]:&cars.front();
    }

    void draw_menu_realistic() {
        draw_real_workshop();
        CarSpec hero{"hero_mustang",1967,"FORD","MUSTANG FASTBACK",0,289,3000,0.93,4};
        draw_model_car(hero,width_*72/100,height_*78/100,std::min(720,width_*55/100));
        fill({34,96,455,500},15,15,15); outline({34,96,455,500},179,166,137,3);
        big_text(66,140,"BACKYARD RACER",245,222,182);
        text(68,175,"BUILD IT  -  RACE IT  -  RISK IT",214,204,183);
        text(68,207,"1960s STREET MACHINES. YOUR GARAGE. YOUR MONEY.",171,164,151);
        button(68,257,330,48,"NEW GAME");
        button(68,319,330,48,"CONTINUE",game_.started());
        button(68,381,330,48,"SETTINGS");
        button(68,443,330,48,"QUIT");
        text(68,560,message_,191,181,163);
    }

    void draw_classifieds_realistic() {
        fill({0,0,width_,height_},203,194,165);
        fill({20,20,width_-40,height_-40},235,228,201); outline({20,20,width_-40,height_-40},46,42,35,3);
        big_text(48,56,"THE BACKYARD GAZETTE",28,26,23);
        line(48,69,width_-48,69,38,35,30,3);
        text(48,92,"USED CARS  -  REAL MODELS  -  CLICK AN AD TO BUY",48,44,38);
        const auto& cars=game_.classifieds();
        const int gap=14,left=48,top=112,card_w=(width_-110)/2,card_h=112;
        for(std::size_t i=0;i<cars.size();++i){
            const int col=static_cast<int>(i%2),row=static_cast<int>(i/2);
            Rect card{left+col*(card_w+gap),top+row*(card_h+10),card_w,card_h};
            const bool hover=card.contains(mouse_x_,mouse_y_);
            if(hover) fill(card,248,241,214);
            outline(card,61,55,45,hover?3:1);
            big_text(card.x+12,card.y+24,car_display_name(cars[i]),35,32,27);
            text(card.x+12,card.y+49,"$"+std::to_string(cars[i].price)+"   "+std::to_string(cars[i].horsepower)+" HP   "+std::to_string(cars[i].weight_lb)+" LB",55,50,43);
            text(card.x+12,card.y+76,"OWNER SAYS: RUNS STRONG - COME SEE IT",76,68,57);
            draw_model_car(cars[i],card.x+card.w-100,card.y+card.h-7,178,true);
        }
        button(width_-205,height_-62,160,36,"GARAGE");
        text(48,height_-38,message_,70,62,50);
    }

    void draw_garage_realistic() {
        draw_real_workshop(); status_bar("BACKYARD GARAGE");
        const OwnedCar* car=game_.active_car();
        if(car){
            // Information panel is kept above the car instead of sitting over it.
            fill({26,82,430,122},18,18,17); outline({26,82,430,122},158,143,119,2);
            big_text(44,108,car_display_name(car->base),241,222,187);
            text(44,133,"HP "+std::to_string(car->horsepower())+"   WEIGHT "+std::to_string(car->base.weight_lb)+" LB   CONDITION "+std::to_string(car->condition)+"%",217,207,187);
            text(44,157,"SELL $"+std::to_string(car->resale_value())+"   REPAIR $"+std::to_string(car->repair_cost())+"   SPARES "+std::to_string(game_.spare_parts().size()),203,192,172);
            button(44,166,138,34,"REPAIR",car->repair_cost()>0); button(196,166,138,34,"SELL CAR");

            draw_model_car(car->base,width_/2,height_*78/100,std::min(790,width_*61/100));

            fill({width_-342,338,302,128},20,20,19); outline({width_-342,338,302,128},158,143,119,2);
            text(width_-322,363,"INSTALLED HARDWARE",236,218,183);
            int y=387;
            if(car->installed_parts.empty()) text(width_-322,y,"STOCK",196,187,170);
            else for(const auto& p:car->installed_parts){text(width_-322,y,part_type_name(p.type)+": "+p.name,196,187,170);y+=19;if(y>452)break;}
        } else {
            fill({30,88,410,92},18,18,17); outline({30,88,410,92},158,143,119,2);
            big_text(48,120,"EMPTY GARAGE",242,220,184); text(48,150,"GRAB THE PAPER AND BUY YOUR FIRST CAR.",215,203,181);
        }
        button(30,height_-57,174,38,"CLASSIFIEDS");
        button(216,height_-57,154,38,"PARTS / TUNE",car!=nullptr);
        button(382,height_-57,148,38,"DRIVE-IN",car!=nullptr);
        button(542,height_-57,150,38,"NEXT CAR",game_.garage().size()>1);
        button(width_-180,height_-57,150,38,"MAIN MENU");
        text(32,height_-76,message_,226,215,194);
    }

    void draw_diner_realistic() {
        // Keep the existing authored drive-in, but replace its generic cars with
        // the actual player's car and actual opponent model.
        fill({0,0,width_,height_},17,25,39);
        fill({0,height_*54/100,width_,height_*46/100},29,30,32);
        for(int i=0;i<30;++i) ellipse((i*83)%width_,78+(i*47)%185,2,2,220,220,190);
        fill({width_*16/100,135,width_*68/100,260},68,52,44);
        fill({width_*18/100,169,width_*64/100,191},214,204,177);
        for(int i=0;i<6;++i) fill({width_*20/100+i*(width_*10/100),211,width_*8/100,105},28,68,83);
        fill({width_*14/100,102,width_*72/100,78},19,17,20); outline({width_*14/100,102,width_*72/100,78},226,55,82,4);
        big_text(width_*27/100,149,"BACKYARD DRIVE-IN",245,84,117);
        fill({0,400,width_,12},151,140,112); line(0,480,width_,480,229,218,175,3);
        status_bar("DRIVE-IN - FIND A RACE");
        const Opponent opp=game_.current_opponent();
        if(const OwnedCar* mine=game_.active_car()) draw_model_car(mine->base,width_*28/100,height_*78/100,std::min(480,width_*38/100));
        draw_model_car(opp.car.base,width_*72/100,height_*78/100,std::min(480,width_*38/100));
        fill({45,84,410,88},17,16,17); outline({45,84,410,88},197,164,116,2);
        big_text(65,112,opp.name+" WANTS TO RACE",243,220,179);
        text(65,138,car_display_name(opp.car.base)+"   "+std::to_string(opp.car.horsepower())+" HP",210,198,176);
        text(65,160,"YOUR REP "+std::to_string(game_.reputation())+"   RECORD "+std::to_string(game_.wins())+"-"+std::to_string(game_.losses()),194,183,165);
        button(70,height_-61,175,40,"DRAG - $100"); button(260,height_-61,175,40,"DRAG - $250"); button(450,height_-61,190,40,"RACE FOR PINKS"); button(width_-180,height_-61,150,40,"GARAGE");
    }

    void draw_realistic() {
        ensure_back_buffer();
        switch(screen_){
            case Screen::Menu: draw_menu_realistic(); break;
            case Screen::Classifieds: draw_classifieds_realistic(); break;
            case Screen::Garage: draw_garage_realistic(); break;
            case Screen::Parts: LegacySceneApp::draw_parts(); break;
            case Screen::Diner: draw_diner_realistic(); break;
            case Screen::Race: LegacySceneApp::draw_race(); break;
            case Screen::Result: LegacySceneApp::draw_result(); break;
            case Screen::Settings: LegacySceneApp::draw_settings(); break;
        }
        XCopyArea(dpy_,back_buffer_,win_,gc_,0,0,static_cast<unsigned>(std::max(1,width_)),static_cast<unsigned>(std::max(1,height_)),0,0);
        XFlush(dpy_);
    }
};

} // namespace backyard_racer

int main(int argc, char** argv) {
    const InfiltratrProjectInfo& info = backyard_racer::project_info();
    if (!infiltratr_project_info_is_valid(&info)) {
        std::cerr << "Backyard Racer project metadata is invalid\n";
        return 1;
    }
    if (argc == 2 && infiltratr_string_equal(argv[1], "--version")) {
        std::cout << info.program_name << ' ' << info.version << '\n';
        return 0;
    }
    if (argc == 2 && infiltratr_string_equal(argv[1], "--project-info"))
        return infiltratr_project_info_print(stdout, &info) == 0 ? 0 : 1;
    try {
        backyard_racer::RealisticApp app(1280, 720);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << info.program_name << " failed to start: " << e.what() << '\n';
        return 1;
    }
}
