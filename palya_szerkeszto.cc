#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

const float v_le = 50;  // pixels per second
const float fa_scale = 0.15;
const float szikla_scale = 0.07;
const float zaszlo_scale = 0.1f;
const float zaszlo_tavolsag = 200.0f;
const float v_h_lassu = 1.0f;
const float v_h_gyors = 0.7f;

sf::Texture fa_texture, szikla_texture, zaszlo_texture_kek,
    zaszlo_texture_piros;

int selected = -1;
int moved = -1;
sf::Vector2f eger_pos;
std::string file_nev;
bool lassu = false;
bool gyors = false;

std::ostream& operator<<(std::ostream& os, sf::Vector2f const& v) {
  os << "(" << v.x << ", " << v.y << ")";
  return os;
}

struct hely {
  float x, y;
};

class Bigyo {
 public:
  virtual void setPosition(float x, float y) = 0;
  virtual sf::FloatRect getGlobalBounds() const = 0;
  virtual void draw(sf::RenderTarget& rt) const = 0;
  virtual void move(const sf::Vector2f& offset) = 0;
  virtual void kiir(std::ostream& os) = 0;
};

class Fa : public Bigyo {
 public:
  Fa(float x, float y) : s{fa_texture} {
    s.setScale(fa_scale, fa_scale);
    s.setPosition(x, y);
  }
  Fa(sf::Vector2f const& v) : Fa(v.x, v.y) {}
  void setPosition(float x, float y) { s.setPosition(x, y); }
  sf::FloatRect getGlobalBounds() const { return s.getGlobalBounds(); }
  void draw(sf::RenderTarget& rt) const { rt.draw(s); }
  void move(const sf::Vector2f& offset) { s.move(offset); }
  void kiir(std::ostream& os) {
    sf::Vector2f p = s.getPosition();
    os << "fa " << p.x << " " << p.y;
  }

 private:
  sf::Sprite s;
};

class Szikla : public Bigyo {
 public:
  Szikla(float x, float y) : sz{szikla_texture} {
    sz.setScale(szikla_scale, szikla_scale);
    sz.setPosition(x, y);
  }
  Szikla(sf::Vector2f const& v) : Szikla(v.x, v.y) {}
  void setPosition(float x, float y) { sz.setPosition(x, y); }
  sf::FloatRect getGlobalBounds() const { return sz.getGlobalBounds(); }
  void draw(sf::RenderTarget& rt) const { rt.draw(sz); }
  void move(const sf::Vector2f& offset) { sz.move(offset); }
  void kiir(std::ostream& os) {
    sf::Vector2f p = sz.getPosition();
    os << "szikla " << p.x << " " << p.y;
  }

 private:
  sf::Sprite sz;
};

class Zaszlok : public Bigyo {
 public:
  Zaszlok(float x, float y, bool kek)
      : kek{kek},
        bal{kek ? zaszlo_texture_kek : zaszlo_texture_piros},
        jobb{kek ? zaszlo_texture_kek : zaszlo_texture_piros} {
    bal.setScale(zaszlo_scale, zaszlo_scale);
    jobb.setScale(zaszlo_scale, zaszlo_scale);
    bal.setPosition(x, y);
    jobb.setPosition(x + zaszlo_tavolsag, y);
  }
  Zaszlok(sf::Vector2f const& v, bool kek) : Zaszlok(v.x, v.y, kek) {}
  void setPosition(float x, float y) {
    bal.setPosition(x, y);
    jobb.setPosition(x + zaszlo_tavolsag, y);
  }
  sf::FloatRect getGlobalBounds() const {
    sf::FloatRect bgb = bal.getGlobalBounds();
    sf::FloatRect jgb = jobb.getGlobalBounds();
    return sf::FloatRect{bgb.left, bgb.top, jgb.left + jgb.width - bgb.left,
                         bgb.height};
  }
  void draw(sf::RenderTarget& rt) const {
    rt.draw(bal);
    rt.draw(jobb);
  }
  void move(const sf::Vector2f& offset) {
    bal.move(offset);
    jobb.move(offset);
  }
  void kiir(std::ostream& os) {
    sf::Vector2f p = bal.getPosition();
    os << "zaszlo " << (kek ? "kek" : "piros") << " " << p.x << " " << p.y;
  }

 private:
  sf::Sprite bal, jobb;
  bool kek;
};

// 1280 x 720
std::vector<Bigyo*> bigyok;

void init(std::string fn) {
  file_nev = fn;
  fa_texture.loadFromFile("fa2.png");
  szikla_texture.loadFromFile("szikla.png");
  zaszlo_texture_kek.loadFromFile("zaszlo_kek.png");
  zaszlo_texture_piros.loadFromFile("zaszlo_piros.png");

  std::ifstream input{fn};

  std::string sor;

  while (std::getline(input, sor)) {
    std::string mi;

    std::istringstream is{sor};
    is >> mi;
    if (mi == "fa") {
      float x, y;
      is >> x >> y;
      bigyok.push_back(new Fa(x, y));
    } else if (mi == "szikla") {
      float x, y;
      is >> x >> y;
      bigyok.push_back(new Szikla(x, y));
    } else if (mi == "zaszlo") {
      float x, y;
      std::string szin;
      is >> szin >> x >> y;
      bigyok.push_back(new Zaszlok(x, y, szin == "kek"));
    }
  }
}

void draw_fak(sf::RenderTarget& rt) {
  for (int i = 0; i < bigyok.size(); ++i) {
    Bigyo* s = bigyok[i];
    if (i == selected) {
      sf::RectangleShape rectangle;
      auto gb = s->getGlobalBounds();
      rectangle.setSize(sf::Vector2f(gb.width, gb.height));
      rectangle.setFillColor(sf::Color(255, 246, 77, 80));
      rectangle.setPosition(gb.left, gb.top);
      rt.draw(rectangle);
    }
    s->draw(rt);
  }
}

void set_selected(sf::Vector2i const& v, sf::RenderTarget& rt) {
  sf::Vector2f uj_pos = rt.mapPixelToCoords(v);
  selected = -1;
  for (int i = bigyok.size() - 1; i >= 0; --i) {
    if (i == moved) {
      bigyok[i]->move(uj_pos - eger_pos);
    }
    if (bigyok[i]->getGlobalBounds().contains(uj_pos)) {
      selected = i;
      break;
    }
  }
  eger_pos = uj_pos;
}

void on_move(sf::Event::MouseMoveEvent const& e, sf::RenderTarget& rt) {
  set_selected(sf::Vector2i{e.x, e.y}, rt);
}

void on_button_pressed(sf::Event::MouseButtonEvent const& e,
                       sf::RenderTarget& rt) {
  sf::Vector2f cs = rt.mapPixelToCoords(sf::Vector2i{e.x, e.y});
  if (selected == -1) {
    return;
  }
  int last = bigyok.size() - 1;
  std::swap(bigyok[selected], bigyok[last]);
  selected = last;
  moved = selected;

  eger_pos = cs;
}

void on_button_released() { moved = -1; }

void kiment() {
  std::ofstream of{file_nev};
  for (Bigyo* b : bigyok) {
    b->kiir(of);
    of << std::endl;
  }
}

void csikok(sf::RenderTarget& rt, float v, sf::Color szin) {
  sf::Vector2u size = rt.getSize();
  sf::Vector2f bf = rt.mapPixelToCoords(sf::Vector2i(0, 0));
  sf::Vector2f jf = rt.mapPixelToCoords(sf::Vector2i(size.x, 0));
  sf::Vector2f ba = rt.mapPixelToCoords(sf::Vector2i(0, size.y));
  float d = v * (ba.y - bf.y);
  sf::Vector2f s{bf.x - d, bf.y};
  float dx = (jf.x - bf.x) / 20.0;
  sf::VertexArray va{sf::Lines};
  for (float x = s.x; x <= jf.x; x += dx) {
    va.append(sf::Vertex{sf::Vector2f{x, bf.y}, szin});
    va.append(sf::Vertex{sf::Vector2f{x + d, ba.y}, szin});
  }
  for (float x = jf.x + d; x >= bf.x; x -= dx) {
    va.append(sf::Vertex{sf::Vector2f{x, bf.y}, szin});
    va.append(sf::Vertex{sf::Vector2f{x - d, ba.y}, szin});
  }
  rt.draw(va);
}

void draw_csikok(sf::RenderTarget& rt) {
  if (lassu) {
    csikok(rt, v_h_lassu, sf::Color(219, 210, 26));
  }
  if (gyors) {
    csikok(rt, v_h_gyors, sf::Color(250, 157, 157));
  }
}

void on_key_down(sf::Event::KeyEvent const& e, sf::RenderTarget& rt) {
  switch (e.code) {
    case sf::Keyboard::F: {
      sf::Vector2u m = fa_texture.getSize();
      bigyok.push_back(
          new Fa(eger_pos - sf::Vector2f(m.x, m.y) * fa_scale * 0.5f));
      break;
    }
    case sf::Keyboard::S: {
      sf::Vector2u m = szikla_texture.getSize();
      bigyok.push_back(
          new Szikla(eger_pos - sf::Vector2f(m.x, m.y) * szikla_scale * 0.5f));
      break;
    }

    case sf::Keyboard::P: {
      sf::Vector2u m = zaszlo_texture_piros.getSize();
      bigyok.push_back(
          new Zaszlok(eger_pos - sf::Vector2f(m.x, m.y) * zaszlo_scale * 0.5f -
                          sf::Vector2f{zaszlo_tavolsag * 0.5f, 0},
                      false));
      break;
    }
    case sf::Keyboard::K: {
      sf::Vector2u m = zaszlo_texture_kek.getSize();
      bigyok.push_back(
          new Zaszlok(eger_pos - sf::Vector2f(m.x, m.y) * zaszlo_scale * 0.5f -
                          sf::Vector2f{zaszlo_tavolsag * 0.5f, 0},
                      true));
      break;
    }
    case sf::Keyboard::Delete: {
      if (selected != -1) {
        bigyok.erase(bigyok.begin() + selected);
        selected = -1;
      }
      break;
    }
    case sf::Keyboard::M:
      kiment();
      break;
    case sf::Keyboard::G:
      gyors = !gyors;
      break;
    case sf::Keyboard::L:
      lassu = !lassu;
      break;
  }
}

void on_scroll(sf::Event::MouseWheelScrollEvent const& e,
               sf::RenderTarget& rt) {
  sf::View v = rt.getView();
  float height = v.getSize().y;
  v.move(0, height / 10.0f * e.delta * -1.0f);
  rt.setView(v);
  set_selected(sf::Vector2i{e.x, e.y}, rt);
}

int main(int argc, char* argv[]) {
  init(argv[1]);
  sf::RenderWindow window(sf::VideoMode(1280, 720), "ski");
  window.setFramerateLimit(60);
  sf::Event event;
  while (true) {
    if (window.pollEvent(event)) {
      // "close requested" event: we close the window
      if (event.type == sf::Event::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::MouseMoved) {
        on_move(event.mouseMove, window);
      } else if (event.type == sf::Event::MouseButtonPressed) {
        on_button_pressed(event.mouseButton, window);
      } else if (event.type == sf::Event::MouseButtonReleased) {
        on_button_released();
      } else if (event.type == sf::Event::KeyPressed) {
        on_key_down(event.key, window);
      } else if (event.type == sf::Event::MouseWheelScrolled) {
        on_scroll(event.mouseWheelScroll, window);
      }
    }
    window.clear(sf::Color::White);
    draw_csikok(window);
    draw_fak(window);
    window.display();
  }
}
