    }
    double parked_x(const OwnedCar& c)const{return static_cast<double>((width_-car_width(c))/2+35);}

    bool tick_motion(){
        if(motion_==Motion::Idle)return false;
        const auto now=std::chrono::steady_clock::now();
        double dt=std::chrono::duration<double>(now-last_tick_).count();last_tick_=now;dt=std::clamp(dt,0.0,0.05);
        if(!displayed_){motion_=Motion::Idle;return true;}
        const int cw=car_width(*displayed_);
        const double radius=std::max(20.0,cw*0.105);
        if(motion_==Motion::Exiting){
            const double dx=std::min(760.0*dt,static_cast<double>(width_+cw)-car_x_);
            car_x_+=dx; wheel_angle_-=dx/radius;
            if(car_x_>=width_+10){
                if(incoming_){displayed_=incoming_;incoming_.reset();car_x_=width_+30;motion_=Motion::Entering;wheel_angle_=0.0;}
                else{displayed_.reset();motion_=Motion::Idle;}
            }
        }else{
            const double target=parked_x(*displayed_);
            const double remaining=car_x_-target;
            double speed=720.0;
            if(remaining<260.0)speed=std::max(170.0,remaining*2.4);
            const double dx=std::min(remaining,speed*dt);
            car_x_-=dx; wheel_angle_+=dx/radius;
            if(car_x_<=target+0.5){car_x_=target;motion_=Motion::Idle;wheel_angle_=0.0;if(const auto a=copy_active(game_))displayed_=a;}
        }
        return true;
    }

    void ensure_frame(){
        if(frame_.width()!=width_||frame_.height()!=height_){
            frame_.resize(width_,height_);
            if(!garage_src_.empty())garage_scaled_=fb::scale_nearest(garage_src_,width_,height_);
            if(ximg_){XDestroyImage(ximg_);ximg_=nullptr;}
        }
    }

    static unsigned shift_for(unsigned long mask){unsigned s=0;if(!mask)return 0;while(((mask>>s)&1UL)==0UL)++s;return s;}
    unsigned long native_pixel(fb::Color c)const{
        auto channel=[](unsigned v,unsigned long mask){if(!mask)return 0UL;const unsigned s=shift_for(mask);const unsigned long range=mask>>s;return (((static_cast<unsigned long>(v)*range+127UL)/255UL)<<s)&mask;};
        return channel(c.r,visual_->red_mask)|channel(c.g,visual_->green_mask)|channel(c.b,visual_->blue_mask);
    }

    void ensure_ximage(){
        if(ximg_&&ximg_w_==width_&&ximg_h_==height_)return;
        if(ximg_)XDestroyImage(ximg_);
        const int depth=DefaultDepth(dpy_,screen_no_);
        const std::size_t bytes=static_cast<std::size_t>(width_)*static_cast<std::size_t>(height_)*4U;
        char* data=static_cast<char*>(std::calloc(bytes,1));
        if(!data)throw std::bad_alloc();
        ximg_=XCreateImage(dpy_,visual_,static_cast<unsigned>(depth),ZPixmap,0,data,static_cast<unsigned>(width_),static_cast<unsigned>(height_),32,0);
        if(!ximg_){std::free(data);throw std::runtime_error("Unable to allocate XImage framebuffer");}
        ximg_w_=width_;ximg_h_=height_;
    }

    void present(){
        ensure_ximage();
        const auto& src=frame_.pixels();
        const unsigned rs=shift_for(visual_->red_mask), gs=shift_for(visual_->green_mask), bs=shift_for(visual_->blue_mask);
        const unsigned long rr=visual_->red_mask?visual_->red_mask>>rs:0, gr=visual_->green_mask?visual_->green_mask>>gs:0, br=visual_->blue_mask?visual_->blue_mask>>bs:0;
        auto fast_native=[&](std::uint32_t p){
            const unsigned r=(p>>16)&255U,g=(p>>8)&255U,b=p&255U;
            return static_cast<std::uint32_t>((((r*rr+127U)/255U)<<rs)&visual_->red_mask) |
                   static_cast<std::uint32_t>((((g*gr+127U)/255U)<<gs)&visual_->green_mask) |
                   static_cast<std::uint32_t>((((b*br+127U)/255U)<<bs)&visual_->blue_mask);
        };
        if(ximg_->bits_per_pixel==32){
            auto* dst=reinterpret_cast<std::uint32_t*>(ximg_->data);
            for(std::size_t i=0;i<src.size();++i)dst[i]=fast_native(src[i]);
        }else{
            for(int y=0;y<height_;++y)for(int x=0;x<width_;++x)XPutPixel(ximg_,x,y,native_pixel(fb::unpack(src[static_cast<std::size_t>(y)*width_+x])));
        }
        XPutImage(dpy_,win_,gc_,ximg_,0,0,0,0,static_cast<unsigned>(width_),static_cast<unsigned>(height_));
        XFlush(dpy_);
    }

    void panel(int x,int y,int w,int h){frame_.alpha_rect(x,y,w,h,{10,10,10,220});frame_.outline(x,y,w,h,fb::rgb(218,203,170),2);}
    void ftext(int x,int y,const std::string& s,fb::Color c=fb::rgb(239,231,207),int sc=2){fb::text(frame_,x,y,s,c,sc);}
    void fbutton(int x,int y,int w,int h,const std::string& label,bool enabled=true){
        const bool hover=enabled&&mouse_x_>=x&&mouse_x_<x+w&&mouse_y_>=y&&mouse_y_<y+h;
        frame_.alpha_rect(x,y,w,h,hover?fb::Color{116,34,30,235}:fb::Color{24,22,21,225});
        frame_.outline(x,y,w,h,enabled?fb::rgb(225,209,170):fb::rgb(90,90,86),2);
        ftext(x+12,y+h/2-7,label,enabled?fb::rgb(244,235,211):fb::rgb(110,110,106),2);
    }

    void status(const std::string& where){
        frame_.alpha_rect(0,0,width_,66,{9,9,10,235});frame_.rect(0,62,width_,4,fb::rgb(157,37,29));
        ftext(22,14,"BACKYARD RACER",fb::rgb(246,225,187),3);ftext(22,42,where,fb::rgb(219,205,177),2);
        const std::string rhs="CASH $"+std::to_string(game_.cash())+"  REP "+std::to_string(game_.reputation())+"  W "+std::to_string(game_.wins())+" L "+std::to_string(game_.losses());
        ftext(std::max(520,width_-500),22,rhs,fb::rgb(239,230,205),2);
    }

    fb::Image tinted_scaled(const CarArt& art,int dw,double angle){
        const int dh=std::max(1,art.source.h*dw/art.source.w);
        fb::Image out;out.w=dw;out.h=dh;out.px.resize(static_cast<std::size_t>(dw)*dh,0);
        const double ca=std::cos(angle),sa=std::sin(angle);
        for(int y=0;y<dh;++y){
            const double sy0=(static_cast<double>(y)+0.5)*art.source.h/dh-0.5;
            for(int x=0;x<dw;++x){
                double sx=(static_cast<double>(x)+0.5)*art.source.w/dw-0.5, sy=sy0;
                const double nx=static_cast<double>(x)/dw, ny=static_cast<double>(y)/dh;
                bool wheel=false; double wcx=0,wcy=0,wr=0;
                for(double cx:{art.rear_x,art.front_x}){
                    const double dx=nx-cx,dy=ny-art.wheel_y;
                    if(dx*dx+dy*dy<art.wheel_r*art.wheel_r){wheel=true;wcx=cx*dw;wcy=art.wheel_y*dh;wr=art.wheel_r*dw;break;}
                }
                if(wheel){
                    const double dx=x-wcx,dy=y-wcy;
                    const double rx=ca*dx+sa*dy, ry=-sa*dx+ca*dy;
                    const double px=wcx+rx,py=wcy+ry;
                    sx=(px+0.5)*art.source.w/dw-0.5;sy=(py+0.5)*art.source.h/dh-0.5;
                }
                const int isx=std::clamp(static_cast<int>(std::lround(sx)),0,art.source.w-1),isy=std::clamp(static_cast<int>(std::lround(sy)),0,art.source.h-1);
                fb::Color c=fb::unpack(art.source.px[static_cast<std::size_t>(isy)*art.source.w+isx]);
                if(c.a>0&&!wheel){
                    const int mx=std::max({c.r,c.g,c.b}),mn=std::min({c.r,c.g,c.b}),lum=(static_cast<int>(c.r)+c.g+c.b)/3;
                    if(mx-mn<20&&lum>=art.paint_min&&lum<=art.paint_max){
                        const double shade=std::clamp(lum/190.0,0.35,1.35);
                        c.r=static_cast<std::uint8_t>(std::clamp(static_cast<int>(art.paint.r*shade),0,255));
                        c.g=static_cast<std::uint8_t>(std::clamp(static_cast<int>(art.paint.g*shade),0,255));
                        c.b=static_cast<std::uint8_t>(std::clamp(static_cast<int>(art.paint.b*shade),0,255));
                    }
                }
                out.px[static_cast<std::size_t>(y)*dw+x]=fb::pack(c);
            }
        }
        return out;
    }

    void draw_car(const OwnedCar& car,int x,int base_y,double angle){
        CarArt* art=art_for(car.base.id);
        if(!art){panel(width_/2-220,base_y-130,440,90);ftext(width_/2-190,base_y-98,"MODEL ART PENDING",fb::rgb(240,210,160),2);return;}
        const int cw=car_width(car);const fb::Image im=tinted_scaled(*art,cw,angle);
        const int y=base_y-im.h;
        frame_.alpha_rect(x+cw/8,base_y-18,cw*3/4,22,{0,0,0,90});
        frame_.blit(im,x,y);
    }

    void draw_menu_fb(){
        if(!garage_scaled_.empty())frame_.copy_from(garage_scaled_);else frame_.clear(fb::rgb(37,34,31));
        frame_.alpha_rect(0,0,width_,height_,{0,0,0,80});
        panel(32,92,455,500);ftext(62,126,"BACKYARD RACER",fb::rgb(246,224,183),3);ftext(62,166,"BUILD IT - RACE IT - RISK IT",fb::rgb(220,207,181),2);
        fbutton(70,252,330,48,"NEW GAME");fbutton(70,314,330,48,"CONTINUE",game_.started());fbutton(70,376,330,48,"SETTINGS");fbutton(70,438,330,48,"QUIT");
        ftext(62,548,message_,fb::rgb(201,190,169),2);
        present();
    }

    void draw_classifieds_fb(){
        frame_.clear(fb::rgb(207,198,168));frame_.rect(20,20,width_-40,height_-40,fb::rgb(236,229,202));frame_.outline(20,20,width_-40,height_-40,fb::rgb(44,40,34),3);
        ftext(48,42,"THE BACKYARD GAZETTE",fb::rgb(35,32,28),3);frame_.rect(48,72,width_-96,3,fb::rgb(44,40,34));ftext(48,88,"USED CARS - CASH TALKS - CLICK AN AD TO BUY",fb::rgb(55,49,41),2);
        const auto& cars=game_.classifieds();const int gap=14,left=48,top=112,card_w=(width_-110)/2,card_h=112;
        for(std::size_t i=0;i<cars.size();++i){const int col=i%2,row=i/2;const int x=left+col*(card_w+gap),y=top+row*(card_h+10);const bool hover=mouse_x_>=x&&mouse_x_<x+card_w&&mouse_y_>=y&&mouse_y_<y+card_h;if(hover)frame_.rect(x,y,card_w,card_h,fb::rgb(248,241,214));frame_.outline(x,y,card_w,card_h,fb::rgb(68,61,50),hover?3:1);ftext(x+12,y+13,car_display_name(cars[i]),fb::rgb(40,36,30),2);ftext(x+12,y+41,"$"+std::to_string(cars[i].price)+"  "+std::to_string(cars[i].horsepower)+" HP  "+std::to_string(cars[i].weight_lb)+" LB",fb::rgb(67,59,48),2);if(CarArt* a=art_for(cars[i].id)){const int pw=165;const auto im=tinted_scaled(*a,pw,0.0);frame_.blit(im,x+card_w-pw-10,y+card_h-im.h-5);}else ftext(x+12,y+70,"MODEL ART PENDING",fb::rgb(100,82,63),1);}
        fbutton(width_-205,height_-62,160,36,"GARAGE");ftext(48,height_-42,message_,fb::rgb(67,59,48),2);present();
    }

    void draw_garage_fb(){
        if(!garage_scaled_.empty())frame_.copy_from(garage_scaled_);else frame_.clear(fb::rgb(45,42,38));
        status("BACKYARD GARAGE");
        const bool moving=motion_!=Motion::Idle;
        if(displayed_){
            const auto& c=*displayed_;
            panel(26,82,438,122);ftext(44,101,car_display_name(c.base),fb::rgb(246,225,188),2);ftext(44,128,"HP "+std::to_string(c.horsepower())+"  WEIGHT "+std::to_string(c.base.weight_lb)+" LB  CONDITION "+std::to_string(c.condition)+"%",fb::rgb(220,210,188),2);ftext(44,152,"SELL $"+std::to_string(c.resale_value())+"  REPAIR $"+std::to_string(c.repair_cost())+"  SPARES "+std::to_string(game_.spare_parts().size()),fb::rgb(209,197,175),2);fbutton(38,166,138,34,"REPAIR",!moving&&c.repair_cost()>0);fbutton(190,166,138,34,"SELL CAR",!moving);
            const int bx=static_cast<int>(std::lround(motion_==Motion::Idle?parked_x(c):car_x_));draw_car(c,bx,height_*86/100,wheel_angle_);
        }else{panel(30,88,410,92);ftext(48,112,"EMPTY GARAGE",fb::rgb(245,223,184),3);ftext(48,146,"BUY A CAR FROM THE PAPER.",fb::rgb(216,203,180),2);}
        if(moving){panel(width_/2-150,height_-128,300,40);ftext(width_/2-118,height_-115,motion_==Motion::Exiting?"REVERSING OUT...":"DRIVING IN...",fb::rgb(248,226,183),2);}
        fbutton(30,height_-57,174,38,"CLASSIFIEDS",!moving);fbutton(216,height_-57,154,38,"PARTS / TUNE",!moving&&game_.active_car());fbutton(382,height_-57,148,38,"DRIVE-IN",!moving&&game_.active_car());fbutton(542,height_-57,150,38,"NEXT CAR",!moving&&game_.garage().size()>1);fbutton(width_-180,height_-57,150,38,"MAIN MENU",!moving);ftext(32,height_-82,message_,fb::rgb(236,222,194),2);present();
    }

    void draw_current(){
        ensure_frame();
        switch(screen_){
            case Screen::Menu: draw_menu_fb(); break;
            case Screen::Classifieds: draw_classifieds_fb(); break;
            case Screen::Garage: draw_garage_fb(); break;
            default: LegacySceneApp::draw(); break;
        }
    }
};

} // namespace backyard_racer

int main(int argc,char** argv){
    const InfiltratrProjectInfo& info=backyard_racer::project_info();
    if(!infiltratr_project_info_is_valid(&info)){std::cerr<<"Backyard Racer project metadata is invalid\n";return 1;}
    if(argc==2&&infiltratr_string_equal(argv[1],"--version")){std::cout<<info.program_name<<' '<<info.version<<'\n';return 0;}
    if(argc==2&&infiltratr_string_equal(argv[1],"--project-info"))return infiltratr_project_info_print(stdout,&info)==0?0:1;
    try{backyard_racer::FramebufferApp app(1280,720);return app.run();}
    catch(const std::exception& e){std::cerr<<info.program_name<<" failed to start: "<<e.what()<<'\n';return 1;}
}
