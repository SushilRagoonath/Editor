#include <blah.h>
using namespace Blah;


struct Editor{
    int width =500;
    int height =width;
    Vec2f mouse_pos;
    Vec2f mouse_prev;
    Vec2f offset;
    float zoom = 1.f;
    TextureRef tex;
    TargetRef target;
    Color * data = nullptr;
    Color current_color;
    void init();
    void update();
    void save();
    bool in_range(Vec2f test);
    Batch b;
    Vector<Color*> old_data;
    bool changed = false;

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
    target = Target::create(width,height);
    font = SpriteFont("default.ttf",14);
    tex = target->texture(0);
//    tex = Texture::create(width,height,TextureFormat::RGBA);
    data = new Color[width*height];
    tex->get_data(data);
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            data[i*width + j] = Color(255,0,0,0);
        }
    }
    tex->set_data(data);
    current_color = Color(0,255,0,255);
}
void Editor::update(){
    App::backbuffer()->clear(Color::black);
    changed = false;
    int m_x,m_y;
    m_x = int(Calc::floor(mouse_pos.x));
    m_y = int(Calc::floor(mouse_pos.y));
    zoom -= float(Input::mouse_wheel().y)  *0.1;
    zoom = Calc::clamp(zoom,0.1,200);
    if(Input::down(MouseButton::Left) && in_range(mouse_pos)){
        changed = true;
        b.push_matrix(batch.peek_matrix());
        b.circle(mouse_pos,10,20,current_color);
        b.pop_matrix();
        b.render(target);
        b.clear();
        tex->get_data(data);
    }
    if(Input::down(MouseButton::Middle)){
        offset += Input::mouse() - mouse_prev;
    }
    if(Input::down(Key::LeftControl) &&Input::down(Key::Z) &&old_data.size()>0 ){
        delete []data;
        data = old_data.pop();
        tex->set_data(data);
    }
//    // only modify on cpu after this
//    tex->get_data(data);
//        data[m_y*tex->width() + m_x] = current_color;

//    // do all modifications before this
//    tex->set_data(data);

//    auto center = App::get_backbuffer_size() / 2;
    Vec2f center = Vec2f(0,0);
    auto transform = Mat3x2f::create_transform(center+offset, Vec2f::zero, Vec2f(1/zoom,1/zoom), 0);
    batch.push_matrix(transform);
    mouse_pos = Vec2f::transform(Input::mouse(),batch.peek_matrix().invert());
    batch.set_sampler( TextureSampler(TextureFilter::Nearest));
    batch.tex(tex);
    batch.pop_matrix();
    batch.str(font,String::fmt("%2d %2d\n",m_x,m_y),Vec2f(10,10),Color::white);

    batch.render();
    batch.clear();
    if(changed){
        old_data.push_back(data);
        data = new Color[width*height];
        Log::info("saved");
    }
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
void init(){ editor.init();}
void update(){ editor.update();}

int main()
{
    Config config;
    config.name = "blah app";
    config.on_startup = init;
    config.on_render = update;
    App::run(&config);
    return 0;
}
