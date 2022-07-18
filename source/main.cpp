#include <blah.h>
#define ImTextureID Blah::TextureRef *
#include <imgui.h>
#include "blah_imgui.h"
using namespace Blah;
struct Editor{
    int width =500;
    int height =width;
    Vec2f mouse_pos;
    Vec2f mouse_prev;
    Vec2f offset;
    float zoom = 0.1f;
    TextureRef main_tex;
    Vector<Subtexture> subs;
    Vec2f tile_size = Vec2f(8.f,8.f);
    int * data = nullptr;
    Color current_color;
    void init();
    void update();
    void save();
    bool in_range(Vec2f test);
    Batch b;
    bool changed = false;
    int current = -1;
};
struct UI{
    struct Element{
        // meta
        String name;
        Rectf rect;
        Color bg;
        Color fg;
        Subtexture tex;
        // data to be changed for user
        bool pressed;
        bool toggle;
        float valf;
        int vali;
        String vals;
    };
    Vector<Element> elements;
    void add(Vec2f pos,String name);
    void render();
    Batch batch;
};
Batch batch;
SpriteFont font;
void Editor::init(){
    main_tex = Texture::create("platformer.png");
    font = SpriteFont("default.ttf",15);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            subs.push_back(Subtexture(main_tex,Rectf(tile_size.x*j,tile_size.y*i,tile_size.x,tile_size.y)));
        }
    }
    data = new int[width*height];
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            data[j*width + i] = current;
        }
    }
//    data = new int[width*height] {-1};
    current = 5;
}
void Editor::update(){
    App::backbuffer()->clear(Color::black);
    changed = false;
    int m_x,m_y;
    m_x = int(Calc::floor(mouse_pos.x/tile_size.x));
    m_y = int(Calc::floor(mouse_pos.y/tile_size.y));
    ImGui::NewFrame();

    static bool demo = true;
    blah_imgui_dock(&demo);
    bool imgui = ImGui::IsAnyItemFocused() ||ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered() ||ImGui::IsWindowFocused();
    {
        if(ImGui::Begin("picker",NULL,0)){
            ImVec2 max_region = ImGui::GetContentRegionMax();
            float current_x = 0;
            for (int i = 0; i < subs.size(); ++i) {
                Subtexture t = subs[i];
                ImGui::PushID(i);
                ImVec2 im_size = ImVec2(20,20);
                if(ImGui::ImageButton( &main_tex,im_size,
                                   ImVec2(t.tex_coords[0].x,t.tex_coords[0].y),
                                   ImVec2(t.tex_coords[2].x,t.tex_coords[2].y),
                                   1
                                   )) {
                    current = i;
                }
                current_x += im_size.x;
                if(current_x+im_size.x <= max_region.x){
                    ImGui::SameLine(0,0);
                }
                else {
                    current_x =0;
                }
                ImGui::PopID();
            }
        ImGui::End();
        }
        imgui = imgui||ImGui::IsWindowHovered() ||ImGui::IsWindowFocused();
    }
    if(Input::down(MouseButton::Left) && in_range(mouse_pos) && !imgui){
        changed = true;
        data[m_y*width + m_x] = current;
    }
	else if (Input::down(MouseButton::Right)){
        changed = true;
        data[m_y*width + m_x] = -1;
    }
    if(Input::down(MouseButton::Middle)){
        offset += Input::mouse() - mouse_prev;
    }
    if(!imgui){
        zoom -= float(Input::mouse_wheel().y)  *0.05;
        zoom = Calc::clamp(zoom,0.001,2000);
    }
    Vec2f center = Vec2f(0,0);
    auto transform = Mat3x2f::create_transform(center+offset, Vec2f::zero, Vec2f(1/zoom,1/zoom), 0);
    batch.push_matrix(transform);
    mouse_pos = Vec2f::transform(Input::mouse(),batch.peek_matrix().invert());
    batch.set_sampler( TextureSampler(TextureFilter::Nearest));
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int index = j*width + i;
            Vec2f pos = Vec2f(8*i,8*j);
            int id = data[index];
//            batch.rect_line(Rectf(8*i,8*j,8,8),0.5,Color::white);
            if(id!=-1)
                batch.tex(subs[id],pos,Vec2f::zero,Vec2f(1.f,1.f),0,Color::white);
        }
    }
    batch.pop_matrix();

    batch.str(font,String::fmt("%2d %2d\n",m_x,m_y),Vec2f(10,10),Color::white);

    batch.render();
    batch.clear();
    mouse_prev = Input::mouse();
}

void Editor::save() {
//    tex.
}

bool Editor::in_range(Vec2f test) {
    int x,y;
    x = int(Calc::floor(test.x));
    y = int(Calc::floor(test.y));
    Recti t = Recti(0,0,width,height);
    return t.contains(Vec2i(x,y));
}


Editor editor;
void init(){
    blah_imgui_startup();
    editor.init();
}
void update(){
    blah_imgui_update();
    editor.update();
    blah_imgui_render();

}

int main()
{
    Config config;
    config.name = "blah app";
    config.on_startup = init;
    config.on_render = update;
    App::run(&config);
    return 0;
}
