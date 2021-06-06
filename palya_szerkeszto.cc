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
const float szikla_scale = 0.15;
const float zaszlo_scale = 0.1f;
const float zaszlo_tavolsag = 200.0f;

sf::Texture fa_texture, szikla_texture, zaszlo_texture_kek, zaszlo_texture_piros;

int selected = -1;
int moved = -1;
sf::Vector2f eger_pos;

struct hely {
  float x, y;
};

class Bigyo {
 public:
  virtual void setPosition(float x, float y) = 0;
  virtual sf::FloatRect getGlobalBounds() const = 0;
  virtual void draw(sf::RenderTarget& rt) const = 0;
  virtual void move(const sf::Vector2f& offset) = 0;
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

 private:
  sf::Sprite sz;
};


class Zaszlok : public Bigyo {
 public:
  Zaszlok(float x, float y, bool kek)
      : bal{kek ? zaszlo_texture_kek : zaszlo_texture_piros},
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

 private:
  sf::Sprite bal, jobb;
};

// 1280 x 720
std::vector<Bigyo*> bigyok;

void init(std::string fn) {
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

void on_move(sf::Event::MouseMoveEvent const& e) {
  sf::Vector2f uj_pos(e.x, e.y);
  for (int i = bigyok.size() - 1; i >= 0; --i) {
    if (i == moved) {
      bigyok[i]->move(uj_pos - eger_pos);
    }
    if (bigyok[i]->getGlobalBounds().contains(e.x, e.y)) {
      selected = i;
      break;
    } else {
      selected = -1;
    }
  }
  eger_pos = uj_pos;
}

void on_button_pressed(sf::Event::MouseButtonEvent const& e) {
  if (selected == -1) {
    return;
  }
  int last = bigyok.size() - 1;
  std::swap(bigyok[selected], bigyok[last]);
  selected = last;
  moved = selected;

  eger_pos = sf::Vector2f(e.x, e.y);
}

void on_button_released() { moved = -1; }

void on_key_down(sf::Event::KeyEvent const& e) {
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
      bigyok.push_back(new Zaszlok(
          eger_pos - sf::Vector2f(m.x, m.y) * fa_scale * 0.5f, false));
      break;
    }
    case sf::Keyboard::K: {
      sf::Vector2u m = zaszlo_texture_kek.getSize();
      bigyok.push_back(new Zaszlok(
          eger_pos - sf::Vector2f(m.x, m.y) * fa_scale * 0.5f, true));
      break;
    }
  }
}

std::ostream& operator<<(std::ostream& os, sf::Vector2f const& v) {
  os << "(" << v.x << ", " << v.y << ")";
  return os;
}

void on_scroll(sf::Event::MouseWheelScrollEvent const& e,
               sf::RenderTarget& rt) {
  sf::View v = rt.getView();
  float height = v.getSize().y;
  v.move(0, height / 10.0f * e.delta * -1.0f);
  rt.setView(v);
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
        on_move(event.mouseMove);
      } else if (event.type == sf::Event::MouseButtonPressed) {
        on_button_pressed(event.mouseButton);
      } else if (event.type == sf::Event::MouseButtonReleased) {
        on_button_released();
      } else if (event.type == sf::Event::KeyPressed) {
        on_key_down(event.key);
      } else if (event.type == sf::Event::MouseWheelScrolled) {
        on_scroll(event.mouseWheelScroll, window);
      }
    }
    window.clear(sf::Color::White);
    draw_fak(window);
    window.display();
  }
}
