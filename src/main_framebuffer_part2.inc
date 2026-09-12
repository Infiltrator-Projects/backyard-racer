            pixel(xx,yy,{static_cast<std::uint8_t>((s.r*a+d.r*ia)/255),static_cast<std::uint8_t>((s.g*a+d.g*ia)/255),static_cast<std::uint8_t>((s.b*a+d.b*ia)/255),255});
        }
    }
private:
    int w_=1,h_=1;
    std::vector<std::uint32_t> px_{1,0xff000000U};
};

static std::array<std::uint8_t,7> glyph(char c) {
    if(c>='a'&&c<='z') c=static_cast<char>(c-'a'+'A');
#define G(a,b,c,d,e,f,g) return {a,b,c,d,e,f,g}
    switch(c){
        case 'A': G(0x0e,0x11,0x11,0x1f,0x11,0x11,0x11); case 'B': G(0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e);
        case 'C': G(0x0e,0x11,0x10,0x10,0x10,0x11,0x0e); case 'D': G(0x1e,0x11,0x11,0x11,0x11,0x11,0x1e);
        case 'E': G(0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f); case 'F': G(0x1f,0x10,0x10,0x1e,0x10,0x10,0x10);
        case 'G': G(0x0e,0x11,0x10,0x17,0x11,0x11,0x0f); case 'H': G(0x11,0x11,0x11,0x1f,0x11,0x11,0x11);
        case 'I': G(0x0e,0x04,0x04,0x04,0x04,0x04,0x0e); case 'J': G(0x07,0x02,0x02,0x02,0x12,0x12,0x0c);
        case 'K': G(0x11,0x12,0x14,0x18,0x14,0x12,0x11); case 'L': G(0x10,0x10,0x10,0x10,0x10,0x10,0x1f);
        case 'M': G(0x11,0x1b,0x15,0x15,0x11,0x11,0x11); case 'N': G(0x11,0x19,0x15,0x13,0x11,0x11,0x11);
        case 'O': G(0x0e,0x11,0x11,0x11,0x11,0x11,0x0e); case 'P': G(0x1e,0x11,0x11,0x1e,0x10,0x10,0x10);
        case 'Q': G(0x0e,0x11,0x11,0x11,0x15,0x12,0x0d); case 'R': G(0x1e,0x11,0x11,0x1e,0x14,0x12,0x11);
        case 'S': G(0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e); case 'T': G(0x1f,0x04,0x04,0x04,0x04,0x04,0x04);
        case 'U': G(0x11,0x11,0x11,0x11,0x11,0x11,0x0e); case 'V': G(0x11,0x11,0x11,0x11,0x11,0x0a,0x04);
        case 'W': G(0x11,0x11,0x11,0x15,0x15,0x15,0x0a); case 'X': G(0x11,0x11,0x0a,0x04,0x0a,0x11,0x11);
        case 'Y': G(0x11,0x11,0x0a,0x04,0x04,0x04,0x04); case 'Z': G(0x1f,0x01,0x02,0x04,0x08,0x10,0x1f);
        case '0': G(0x0e,0x11,0x13,0x15,0x19,0x11,0x0e); case '1': G(0x04,0x0c,0x04,0x04,0x04,0x04,0x0e);
        case '2': G(0x0e,0x11,0x01,0x02,0x04,0x08,0x1f); case '3': G(0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e);
        case '4': G(0x02,0x06,0x0a,0x12,0x1f,0x02,0x02); case '5': G(0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e);
        case '6': G(0x0e,0x10,0x10,0x1e,0x11,0x11,0x0e); case '7': G(0x1f,0x01,0x02,0x04,0x08,0x08,0x08);
        case '8': G(0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e); case '9': G(0x0e,0x11,0x11,0x0f,0x01,0x01,0x0e);
        case '$': G(0x04,0x0f,0x14,0x0e,0x05,0x1e,0x04); case '-': G(0,0,0,0x1f,0,0,0);
        case '/': G(0x01,0x02,0x02,0x04,0x08,0x08,0x10); case ':': G(0,0x04,0x04,0,0x04,0x04,0);
        case '.': G(0,0,0,0,0,0x06,0x06); case '%': G(0x19,0x1a,0x02,0x04,0x08,0x0b,0x13);
        case '+': G(0,0x04,0x04,0x1f,0x04,0x04,0); case '?': G(0x0e,0x11,0x01,0x02,0x04,0,0x04);
        case '!': G(0x04,0x04,0x04,0x04,0x04,0,0x04); case ',': G(0,0,0,0,0,0x04,0x08);
        case '(': G(0x02,0x04,0x08,0x08,0x08,0x04,0x02); case ')': G(0x08,0x04,0x02,0x02,0x02,0x04,0x08);
        case ' ': default: G(0,0,0,0,0,0,0);
    }
#undef G
}

static void text(Surface& s,int x,int y,const std::string& str,Color c,int scale=2) {
    const int adv=6*scale;
    for(char ch:str){
        const auto rows=glyph(ch);
        for(int gy=0;gy<7;++gy)for(int gx=0;gx<5;++gx)if(rows[gy]&(1u<<(4-gx)))s.rect(x+gx*scale,y+gy*scale,scale,scale,c);
        x+=adv;
    }
}

} // namespace fb

class FramebufferApp final : public LegacySceneApp {
public:
    FramebufferApp(int w,int h):LegacySceneApp(w,h){
        load_assets();
        if (const OwnedCar* c=game_.active_car()) displayed_=*c;
        last_tick_=std::chrono::steady_clock::now();
    }

    ~FramebufferApp(){ if(ximg_) XDestroyImage(ximg_); }

    int run(){
        draw_current();
        while(running_){
            const bool realtime = motion_ != Motion::Idle || screen_ == Screen::Race;
            bool redraw=false;
            if(realtime){
                while(XPending(dpy_)>0){XEvent ev{};XNextEvent(dpy_,&ev);handle_event(ev);redraw=true;}
                if(screen_==Screen::Race) redraw=tick_race()||redraw;
                redraw=tick_motion()||redraw;
                if(redraw)draw_current();
                std::this_thread::sleep_for(std::chrono::milliseconds(8));
            }else{
                XEvent ev{};XNextEvent(dpy_,&ev);handle_event(ev);draw_current();
            }
        }
        return 0;
    }

private:
    enum class Motion { Idle, Entering, Exiting };

    struct CarArt {
        std::string id;
        fb::Image source;
        double rear_x=0.23, front_x=0.77, wheel_y=0.80, wheel_r=0.105;
        int paint_min=80, paint_max=245;
        fb::Color paint=fb::rgb(210,210,205);
    };

    fb::Surface frame_;
    fb::Image garage_src_, garage_scaled_;
    std::vector<CarArt> cars_;
    XImage* ximg_=nullptr;
    int ximg_w_=0,ximg_h_=0;
    std::optional<OwnedCar> displayed_;
    std::optional<OwnedCar> incoming_;
    Motion motion_=Motion::Idle;
    double car_x_=0.0;
    double wheel_angle_=0.0;
    std::chrono::steady_clock::time_point last_tick_{};

    void load_assets(){
        try {
            garage_src_=fb::load_indexed_image(scene_assets::garage);
            cars_.push_back({"falcon64",fb::load_indexed_image(scene_assets::falcon),.225,.775,.80,.105,120,248,fb::rgb(232,229,216)});
            cars_.push_back({"mustang65",fb::load_indexed_image(scene_assets::mustang),.245,.765,.82,.105,70,205,fb::rgb(44,83,128)});
        } catch(const std::exception& e) { std::cerr<<"Backyard Racer embedded asset warning: "<<e.what()<<'\n'; }
    }

    CarArt* art_for(const std::string& id){for(auto& a:cars_)if(a.id==id)return &a;return nullptr;}

    static std::optional<OwnedCar> copy_active(const GameState& g){
        if(const OwnedCar* c=g.active_car())return *c;
        return std::nullopt;
    }

    void handle_event(const XEvent& ev){
        const Screen before_screen=screen_;
        const auto before=copy_active(game_);
        const std::size_t before_index=game_.active_car_index();
        const std::size_t before_count=game_.garage().size();
        if(motion_!=Motion::Idle && screen_==Screen::Garage && ev.type==ButtonPress) return;
        process_event(ev);
        const auto after=copy_active(game_);
        if(screen_==Screen::Garage){
            const std::string bid=before?before->base.id:"";
            const std::string aid=after?after->base.id:"";
            const bool occupant_changed=(before.has_value()!=after.has_value()) || bid!=aid ||
                before_index!=game_.active_car_index() || before_count!=game_.garage().size();
            if(occupant_changed && (before_screen==Screen::Garage || before_screen==Screen::Classifieds)){
                begin_change(before,after);
            }else if(motion_==Motion::Idle && after){
                displayed_=after;
            }
        }
    }

    void begin_change(const std::optional<OwnedCar>& oldcar,const std::optional<OwnedCar>& newcar){
        incoming_=newcar;
        wheel_angle_=0.0;
        if(oldcar){
            displayed_=oldcar;
            car_x_=parked_x(*oldcar);
            motion_=Motion::Exiting;
        }else if(newcar){
            displayed_=newcar;
            car_x_=static_cast<double>(width_+40);
            motion_=Motion::Entering;
        }else{
            displayed_.reset(); incoming_.reset(); motion_=Motion::Idle;
        }
        last_tick_=std::chrono::steady_clock::now();
    }

    int car_width(const OwnedCar& c)const{
        if(c.base.id=="falcon64")return std::clamp(width_*62/100,600,850);
        if(c.base.id=="mustang65")return std::clamp(width_*58/100,560,800);
        return std::clamp(width_*58/100,560,800);
