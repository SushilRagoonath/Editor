#include <blah.h>
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
    bool tile_picking = false;
    int * data = nullptr;
    void init();
    void update();
    void save();
    void tile_picker();
    void draw_map();
    bool in_range(Vec2f test);
    Batch b;
    bool changed = false;
    int current = -1;
};
Batch batch;
SpriteFont font;
void Editor::init(){
    font = SpriteFont("default.ttf",15);
    // TODO image specific
    main_tex = Texture::create("platformer.png");
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
    // aligns mouse pick to grid
    m_x = int(Calc::floor(mouse_pos.x/tile_size.x));
    m_y = int(Calc::floor(mouse_pos.y/tile_size.y));
    if(Input::pressed(Key::Space)){
        tile_picking = !tile_picking;
    }
    if(tile_picking){
        tile_picker();
    }
    else if(Input::down(MouseButton::Left) && in_range(mouse_pos)){
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

    zoom -= float(Input::mouse_wheel().y) * 0.05;
    zoom = Calc::clamp(zoom,0.001,2000); // dont want the image to be reversed with -zoom
    if(!tile_picking) draw_map();
    batch.str(font,String::fmt("%2d %2d\n",m_x,m_y),Vec2f(10,10),Color::white);

    batch.render();
    batch.clear();
    mouse_prev = Input::mouse();
}

void Editor::save() {
//    tex.
}
// check pos is in range of image
bool Editor::in_range(Vec2f test) {
    int x,y;
    x = int(Calc::floor(test.x));
    y = int(Calc::floor(test.y));
    Recti t = Recti(0,0,width,height);
    return t.contains(Vec2i(x,y));
}

void Editor::tile_picker() {
    batch.set_sampler( TextureSampler(TextureFilter::Nearest));

    batch.push_matrix(Mat3x2f::create_transform(Vec2f(main_tex->width()/2,main_tex->height()/2),Vec2f::zero,Vec2f(10,10),0));
    // mouse picking
    Vec2 mouse_picker = Vec2f::transform(Input::mouse(),batch.peek_matrix().invert());
    int m_x,m_y;
    m_x = int(Calc::floor(mouse_picker.x));
    m_y = int(Calc::floor(mouse_picker.y));
    for (int i = 0; i < subs.size(); ++i) {
        batch.tex(subs[i],subs[i].source.top_left());
        // hover
        if(subs[i].source.contains(Vec2f(m_x,m_y))){
            batch.rect(subs[i].source,Color(134,103,255,64));
        }
        // button test
        if(subs[i].source.contains(Vec2f(m_x,m_y)) && Input::pressed(MouseButton::Left)){
            current = i;
        }
    }
    // outline for selected
    batch.rect_line(subs[current].source,0.5,Color::orange);
    batch.pop_matrix();
}

void Editor::draw_map() {
    Vec2f center = Vec2f(0,0);
    auto transform = Mat3x2f::create_transform(center+offset, Vec2f::zero, Vec2f(1/zoom,1/zoom), 0);
    batch.push_matrix(transform);
    // mouse pick
    mouse_pos = Vec2f::transform(Input::mouse(),batch.peek_matrix().invert());
    batch.set_sampler( TextureSampler(TextureFilter::Nearest));
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            int index = j*width + i;
            Vec2f pos = Vec2f(8*i,8*j);
            int id = data[index];
            if(id!=-1) // -1 invalid
                batch.tex(subs[id],pos,Vec2f::zero,Vec2f(1.f,1.f),0,Color::white);
        }
    }
    batch.pop_matrix();

}


Editor editor;
void init(){
    editor.init();
}
void update(){
    editor.update();

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
