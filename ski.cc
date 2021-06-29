#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

const float v_le = 50;  // pixels per second
const float fa_scale = 0.15;
const float szikla_scale = 0.07;
const float zaszlo_scale = 0.1f;
const float zaszlo_tavolsag = 200.0f;
const float v_h_lassu = 1.0f;
const float v_h_gyors = 0.7f;
const float sielo_scale = 0.25f;

// proba

sf::Texture fa_texture, szikla_texture, zaszlo_texture_kek,
    zaszlo_texture_piros, siel_texture_balra, siel_texture_jobbra,
    siel_texture_egyenesen;

sf::Sprite bal_sprite, jobb_sprite, egyenes_sprite;

int moved = -1;
sf::Vector2f eger_pos;
std::string file_nev;
bool turbo = false;
bool jobbra = false;
bool balra = false;

class Bigyo;
std::vector<Bigyo*> bigyok;

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
  virtual float top() = 0;
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
  float top() { return s.getGlobalBounds().top; }

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
  float top() { return sz.getGlobalBounds().top; }

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
  float top() { return bal.getGlobalBounds().top; }

 private:
  sf::Sprite bal, jobb;
  bool kek;
};

// 1280 x 720

void init(std::string fn) {
  file_nev = fn;
  fa_texture.loadFromFile("fa2.png");
  szikla_texture.loadFromFile("szikla.png");
  zaszlo_texture_kek.loadFromFile("zaszlo_kek.png");
  zaszlo_texture_piros.loadFromFile("zaszlo_piros.png");
  siel_texture_balra.loadFromFile("skiel_balra2.png");
  siel_texture_jobbra.loadFromFile("skiel_jobbra2.png");
  siel_texture_egyenesen.loadFromFile("skiel_egyenes2.png");

  bal_sprite.setTexture(siel_texture_balra);
  jobb_sprite.setTexture(siel_texture_jobbra);
  egyenes_sprite.setTexture(siel_texture_egyenesen);

  bal_sprite.setScale(sielo_scale, sielo_scale);
  jobb_sprite.setScale(sielo_scale, sielo_scale);
  egyenes_sprite.setScale(sielo_scale, sielo_scale);

  bal_sprite.setPosition(640 - bal_sprite.getGlobalBounds().width / 2.0f, 100);
  jobb_sprite.setPosition(640 - jobb_sprite.getGlobalBounds().width / 2.0f,
                          100);
  egyenes_sprite.setPosition(
      640 - egyenes_sprite.getGlobalBounds().width / 2.0f, 100);

  // jobb_sprite.move(egyenes_sprite.getPosition().x +
  //                     jobb_sprite.getGlobalBounds().width -
  //                     egyenes_sprite.getGlobalBounds().width,
  //                 0);

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

const float sielo_hitbox_sfx = 0.45f;
const float sielo_hitbox_sfy_top = 0.5f;
const float sielo_hitbox_sfy_bottom = 0.4f;

sf::FloatRect compute_sielo_hitbox(sf::FloatRect const& br) {
  sf::FloatRect res;
  res.left = br.width * sielo_hitbox_sfx + br.left;
  res.width = br.width - 2.0f * br.width * sielo_hitbox_sfx;
  res.top = br.height * sielo_hitbox_sfy_top + br.top;
  res.height =
      br.height - br.height * (sielo_hitbox_sfy_top + sielo_hitbox_sfy_bottom);
  return res;
}

void draw_fak(sf::RenderTarget& rt) {
  for (int i = 0; i < bigyok.size(); ++i) {
    Bigyo* s = bigyok[i];
    s->draw(rt);
  }
}

void draw_sielo(sf::RenderTarget& rt) {
  sf::FloatRect br;
  if (jobbra == true) {
    rt.draw(jobb_sprite);
    br = jobb_sprite.getGlobalBounds();
  } else if (balra == true) {
    rt.draw(bal_sprite);
    br = bal_sprite.getGlobalBounds();
  } else {
    rt.draw(egyenes_sprite);
    br = egyenes_sprite.getGlobalBounds();
  }
  const sf::FloatRect hb = compute_sielo_hitbox(br);
  sf::RectangleShape rectangle;
  rectangle.setSize(sf::Vector2f{hb.width, hb.height});
  rectangle.setOutlineColor(sf::Color::Yellow);
  rectangle.setFillColor(sf::Color{0,0,0,0});
  rectangle.setOutlineThickness(5);
  rectangle.setPosition(hb.left, hb.top);
  rt.draw(rectangle);
}

void sielo_move(bool jobbra, bool balra, std::int64_t fus) {
  if (balra == true) {
    bal_sprite.setPosition(jobb_sprite.getPosition());
    bal_sprite.move(sf::Vector2f{-2.5f * fus / 16000.0f, 0.0f});
    jobb_sprite.setPosition(bal_sprite.getPosition());
  } else if (jobbra == true) {
    bal_sprite.setPosition(jobb_sprite.getPosition());
    bal_sprite.move(sf::Vector2f{2.5f * fus / 16000.0f, 0.0f});
    jobb_sprite.setPosition(bal_sprite.getPosition());
  }
  egyenes_sprite.setPosition(bal_sprite.getPosition());
}

void on_key_down(sf::Event::KeyEvent const& e, sf::RenderTarget& rt) {
  switch (e.code) {
    case sf::Keyboard::Left: {
      balra = true;
      jobbra = false;
      break;
    }
    case sf::Keyboard::Right: {
      balra = false;
      jobbra = true;
      break;
    }
    case sf::Keyboard::Space: {
      turbo = true;
      break;
    }
  }
}

void on_key_up(sf::Event::KeyEvent const& e, sf::RenderTarget& rt) {
  switch (e.code) {
    case sf::Keyboard::Left: {
      balra = false;
      break;
    }
    case sf::Keyboard::Right: {
      jobbra = false;
      break;
    }
    case sf::Keyboard::Space: {
      turbo = false;
      break;
    }
  }
}

void move(std::int64_t fus, bool turbo) {
  if (turbo == false) {
    for (int i = 0; i < bigyok.size(); ++i) {
      bigyok[i]->move(sf::Vector2f{0.0f, -5.0f * fus / 16000.0f});
    }
  } else if (turbo == true) {
    for (int i = 0; i < bigyok.size(); ++i) {
      bigyok[i]->move(sf::Vector2f{0.0f, -8.0f * fus / 16000.0f});
    }
  }
}

int main(int argc, char* argv[]) {
  init(argv[1]);
  sf::RenderWindow window(sf::VideoMode(1280, 720), "ski");
  window.setFramerateLimit(60);
  sf::Event event;
  sf::Clock c;
  while (true) {
    std::int64_t fus = c.restart().asMicroseconds();
    if (window.pollEvent(event)) {
      // "close requested" event: we close the window
      if (event.type == sf::Event::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::KeyPressed) {
        on_key_down(event.key, window);
      } else if (event.type == sf::Event::KeyReleased) {
        on_key_up(event.key, window);
      }
    }
    move(fus, turbo);
    sielo_move(jobbra, balra, fus);
    window.clear(sf::Color::White);
    draw_sielo(window);
    draw_fak(window);
    window.display();
  }
}
