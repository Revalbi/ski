#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
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
const std::uint64_t kemeny_buntetes = 5 * 1000000ll;
const float cel_scale = 1.2f;
const float alja_cel_tavolsag = 800.0f;
const float sielo_world_y = 100.0f;
const float vegrajz_scale = 1.2f;

enum class HitEvent { NONE, ZASZLO, KEMENY };

// proba

sf::Texture fa_texture, szikla_texture, zaszlo_texture_kek,
    zaszlo_texture_piros, siel_texture_balra, siel_texture_jobbra,
    siel_texture_egyenesen, celvonal_texture, celkapu_texture, vegrajz_texture;

sf::Sprite bal_sprite, jobb_sprite, egyenes_sprite, vegrajz_sprite;

sf::Font font;

int moved = -1;
sf::Vector2f eger_pos;
std::string file_nev;
bool turbo = false;
bool jobbra = false;
bool balra = false;
bool fut = true;
bool automata = false;
bool draw_hitbox = false;
bool korvege = false;
std::uint64_t added_time = 0;
std::uint64_t score_ido = 0;
float world_y = 0.0f;
float alja;
float sielo_magassag;

class CelKapu;
CelKapu* cel_kapu;

class Bigyo;
std::vector<Bigyo*> bigyok;
std::vector<Bigyo*> also_bigyok;

std::ostream& operator<<(std::ostream& os, sf::Vector2f const& v) {
  os << "(" << v.x << ", " << v.y << ")";
  return os;
}

struct hely {
  float x, y;
};

const float fak_hitbox_sfx = 0.45f;
const float fak_hitbox_sfy_top = 0.8f;
const float fak_hitbox_sfy_bottom = 0.07f;

sf::FloatRect compute_fak_hitbox(sf::FloatRect const& fak_Global_bounds) {
  sf::FloatRect res_fak;
  res_fak.left =
      fak_Global_bounds.width * fak_hitbox_sfx + fak_Global_bounds.left;
  res_fak.width =
      fak_Global_bounds.width - 2.0f * fak_Global_bounds.width * fak_hitbox_sfx;
  res_fak.top =
      fak_Global_bounds.height * fak_hitbox_sfy_top + fak_Global_bounds.top;
  res_fak.height =
      fak_Global_bounds.height -
      fak_Global_bounds.height * (fak_hitbox_sfy_top + fak_hitbox_sfy_bottom);
  return res_fak;
}

const float sziklak_hitbox_sfx = 0.7f;
const float sziklak_hitbox_sfy_top = 0.3f;
const float sziklak_hitbox_sfy_bottom = 0.3f;

sf::FloatRect compute_sziklak_hitbox(
    sf::FloatRect const& sziklak_Global_bounds) {
  sf::FloatRect res_sziklak;
  res_sziklak.left = sziklak_Global_bounds.width * sziklak_hitbox_sfx +
                     sziklak_Global_bounds.left;
  res_sziklak.width = sziklak_Global_bounds.width -
                      2.0f * sziklak_Global_bounds.width * sziklak_hitbox_sfx;
  res_sziklak.top = sziklak_Global_bounds.height * sziklak_hitbox_sfy_top +
                    sziklak_Global_bounds.top;
  res_sziklak.height = sziklak_Global_bounds.height -
                       sziklak_Global_bounds.height *
                           (sziklak_hitbox_sfy_top + sziklak_hitbox_sfy_bottom);
  return res_sziklak;
}

class Bigyo {
 public:
  virtual void setPosition(float x, float y) = 0;
  virtual sf::FloatRect getGlobalBounds() const = 0;
  virtual void draw(sf::RenderTarget& rt) const = 0;
  virtual void move(const sf::Vector2f& offset) = 0;
  virtual void kiir(std::ostream& os) = 0;
  virtual float top() = 0;
  virtual HitEvent utkozes_vizsgalat(sf::FloatRect const& other) const {
    if (hitbox().intersects(other)) {
      return utkozes_tipus;
    }
    return HitEvent::NONE;
  }

 protected:
  HitEvent utkozes_tipus = HitEvent::KEMENY;
  virtual sf::FloatRect hitbox() const = 0;
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
  void draw(sf::RenderTarget& rt) const {
    rt.draw(s);
    if (draw_hitbox) {
      sf::FloatRect fak_global_bounds = getGlobalBounds();
      const sf::FloatRect hb_2 = compute_fak_hitbox(fak_global_bounds);
      sf::RectangleShape rectangle;
      rectangle.setSize(sf::Vector2f{hb_2.width, hb_2.height});
      rectangle.setOutlineColor(sf::Color::Yellow);
      rectangle.setFillColor(sf::Color{0, 0, 0, 0});
      rectangle.setOutlineThickness(5);
      rectangle.setPosition(hb_2.left, hb_2.top);
      rt.draw(rectangle);
    }
  }
  void move(const sf::Vector2f& offset) { s.move(offset); }
  void kiir(std::ostream& os) {
    sf::Vector2f p = s.getPosition();
    os << "fa " << p.x << " " << p.y;
  }
  float top() { return s.getGlobalBounds().top; }

  sf::FloatRect hitbox() const {
    sf::FloatRect fak_global_bounds = getGlobalBounds();
    return compute_fak_hitbox(fak_global_bounds);
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
  void draw(sf::RenderTarget& rt) const {
    rt.draw(sz);
    if (draw_hitbox) {
      sf::FloatRect sziklak_global_bounds = getGlobalBounds();
      const sf::FloatRect hb_3 = compute_sziklak_hitbox(sziklak_global_bounds);
      sf::RectangleShape rectangle;
      rectangle.setSize(sf::Vector2f{hb_3.width, hb_3.height});
      rectangle.setOutlineColor(sf::Color::Yellow);
      rectangle.setFillColor(sf::Color{0, 0, 0, 0});
      rectangle.setOutlineThickness(5);
      rectangle.setPosition(hb_3.left, hb_3.top);
      rt.draw(rectangle);
    }
  }
  void move(const sf::Vector2f& offset) { sz.move(offset); }
  void kiir(std::ostream& os) {
    sf::Vector2f p = sz.getPosition();
    os << "szikla " << p.x << " " << p.y;
  }
  float top() { return sz.getGlobalBounds().top; }
  sf::FloatRect hitbox() const override {
    sf::FloatRect sziklak_global_bounds = getGlobalBounds();
    return compute_sziklak_hitbox(sziklak_global_bounds);
  }

 private:
  sf::Sprite sz;
};

class CelVonal : public Bigyo {
 public:
  CelVonal(float x, float y) : c{celvonal_texture} {
    c.setScale(cel_scale, cel_scale);
    c.setPosition(x, y);
  }
  CelVonal(sf::Vector2f const& v) : CelVonal(v.x, v.y) {}
  void setPosition(float x, float y) { c.setPosition(x, y); }
  sf::FloatRect getGlobalBounds() const { return c.getGlobalBounds(); }
  void draw(sf::RenderTarget& rt) const {
    rt.draw(c);
    //    if (draw_hitbox) {
    //      sf::FloatRect fak_global_bounds = getGlobalBounds();
    //      const sf::FloatRect hb_2 = compute_fak_hitbox(fak_global_bounds);
    //      sf::RectangleShape rectangle;
    //      rectangle.setSize(sf::Vector2f{hb_2.width, hb_2.height});
    //      rectangle.setOutlineColor(sf::Color::Yellow);
    //      rectangle.setFillColor(sf::Color{0, 0, 0, 0});
    //      rectangle.setOutlineThickness(5);
    //      rectangle.setPosition(hb_2.left, hb_2.top);
    //      rt.draw(rectangle);
    //    }
  }
  void move(const sf::Vector2f& offset) { c.move(offset); }
  void kiir(std::ostream& os) {
    sf::Vector2f p = c.getPosition();
    os << "celvonal " << p.x << " " << p.y;
  }
  float top() { return c.getGlobalBounds().top; }

  sf::FloatRect hitbox() const { return sf::FloatRect{}; }

 private:
  sf::Sprite c;
};

class CelKapu : public Bigyo {
 public:
  CelKapu(float x, float y) : ck{celkapu_texture} {
    ck.setScale(cel_scale, cel_scale);
    ck.setPosition(x, y);
  }
  CelKapu(sf::Vector2f const& v) : CelKapu(v.x, v.y) {}
  void setPosition(float x, float y) { ck.setPosition(x, y); }
  sf::FloatRect getGlobalBounds() const { return ck.getGlobalBounds(); }
  void draw(sf::RenderTarget& rt) const {
    rt.draw(ck);
    //    if (draw_hitbox) {
    //      sf::FloatRect fak_global_bounds = getGlobalBounds();
    //      const sf::FloatRect hb_2 = compute_fak_hitbox(fak_global_bounds);
    //      sf::RectangleShape rectangle;
    //      rectangle.setSize(sf::Vector2f{hb_2.width, hb_2.height});
    //      rectangle.setOutlineColor(sf::Color::Yellow);
    //      rectangle.setFillColor(sf::Color{0, 0, 0, 0});
    //      rectangle.setOutlineThickness(5);
    //      rectangle.setPosition(hb_2.left, hb_2.top);
    //      rt.draw(rectangle);
    //    }
  }
  void move(const sf::Vector2f& offset) { ck.move(offset); }
  void kiir(std::ostream& os) {
    sf::Vector2f p = ck.getPosition();
    os << "celkapu " << p.x << " " << p.y;
  }
  float top() { return ck.getGlobalBounds().top; }

  sf::FloatRect hitbox() const { return sf::FloatRect{}; }

 private:
  sf::Sprite ck;
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
  HitEvent utkozes_vizsgalat(sf::FloatRect const& other) {
    return HitEvent::NONE;
  }
  sf::FloatRect hitbox() const override { return sf::FloatRect{}; }

 private:
  sf::Sprite bal, jobb;
  bool kek;
};

// 1280 x 720

void init(std::string fn) {
  file_nev = fn;
  font.loadFromFile("font1.ttf");
  fa_texture.loadFromFile("fa2.png");
  szikla_texture.loadFromFile("szikla.png");
  zaszlo_texture_kek.loadFromFile("zaszlo_kek.png");
  zaszlo_texture_piros.loadFromFile("zaszlo_piros.png");
  siel_texture_balra.loadFromFile("siel_balra3.png");
  siel_texture_jobbra.loadFromFile("siel_jobbra3.png");
  siel_texture_egyenesen.loadFromFile("skiel_egyenes2.png");
  celvonal_texture.loadFromFile("celvonal2.png");
  celkapu_texture.loadFromFile("celkapu.png");
  vegrajz_texture.loadFromFile("sielo_erem1.png");

  bal_sprite.setTexture(siel_texture_balra);
  jobb_sprite.setTexture(siel_texture_jobbra);
  egyenes_sprite.setTexture(siel_texture_egyenesen);

  bal_sprite.setScale(sielo_scale, sielo_scale);
  jobb_sprite.setScale(sielo_scale, sielo_scale);
  egyenes_sprite.setScale(sielo_scale, sielo_scale);

  bal_sprite.setPosition(640 - bal_sprite.getGlobalBounds().width / 2.0f,
                         sielo_world_y);
  jobb_sprite.setPosition(640 - jobb_sprite.getGlobalBounds().width / 2.0f,
                          sielo_world_y);
  egyenes_sprite.setPosition(
      640 - egyenes_sprite.getGlobalBounds().width / 2.0f, sielo_world_y);

  sielo_magassag = egyenes_sprite.getGlobalBounds().height;

  vegrajz_sprite.setTexture(vegrajz_texture);
  vegrajz_sprite.setPosition(90, -200);
  vegrajz_sprite.setScale(vegrajz_scale, vegrajz_scale);

  // jobb_sprite.move(egyenes_sprite.getPosition().x +
  //                     jobb_sprite.getGlobalBounds().width -
  //                     egyenes_sprite.getGlobalBounds().width,
  //                 0);

  std::ifstream input{fn};

  std::string sor;

  alja = std::numeric_limits<float>::min();

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
    if (bigyok.size() == 0) {
      continue;
    }
    sf::FloatRect gb = bigyok[bigyok.size() - 1]->getGlobalBounds();
    float bottom = gb.top + gb.height;
    if (alja < bottom) {
      alja = bottom;
    }
  }
  cel_kapu = new CelKapu{0, alja + alja_cel_tavolsag};
  bigyok.push_back(cel_kapu);
  also_bigyok.push_back(new CelVonal{0, alja + alja_cel_tavolsag});
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

void draw_alsok(sf::RenderTarget& rt) {
  for (int i = 0; i < also_bigyok.size(); ++i) {
    Bigyo* s = also_bigyok[i];
    s->draw(rt);
  }
}

sf::FloatRect sielo_hitbox() {
  sf::FloatRect br;
  if (jobbra == true) {
    br = jobb_sprite.getGlobalBounds();
  } else if (balra == true) {
    br = bal_sprite.getGlobalBounds();
  } else {
    br = egyenes_sprite.getGlobalBounds();
  }
  const sf::FloatRect hb = compute_sielo_hitbox(br);
  return hb;
}

void draw_sielo(sf::RenderTarget& rt) {
  if (jobbra == true) {
    rt.draw(jobb_sprite);
  } else if (balra == true) {
    rt.draw(bal_sprite);
  } else {
    rt.draw(egyenes_sprite);
  }
  if (draw_hitbox) {
    const sf::FloatRect hb = sielo_hitbox();
    sf::RectangleShape rectangle;
    rectangle.setSize(sf::Vector2f{hb.width, hb.height});
    rectangle.setOutlineColor(sf::Color::Yellow);
    rectangle.setFillColor(sf::Color{0, 0, 0, 0});
    rectangle.setOutlineThickness(5);
    rectangle.setPosition(hb.left, hb.top);
    rt.draw(rectangle);
  }
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
  float sebesseg = turbo ? -8.0f : -5.0f;
  float dy = sebesseg * fus / 16000.0f;
  for (int i = 0; i < bigyok.size(); ++i) {
    bigyok[i]->move(sf::Vector2f{0.0f, dy});
  }
  for (int i = 0; i < also_bigyok.size(); ++i) {
    also_bigyok[i]->move(sf::Vector2f{0.0f, dy});
  }
  world_y += dy;
}

void advance_score(std::int64_t fus, bool turbo) {
  if (!automata) {
    if (turbo == false) {
      score_ido = score_ido + fus;
    } else if (turbo == true) {
      score_ido = score_ido + fus * 5;
    }
  }
}

HitEvent sielo_utkozes() {
  sf::FloatRect br = sielo_hitbox();
  for (int i = 0; i < bigyok.size(); ++i) {
    HitEvent e = bigyok[i]->utkozes_vizsgalat(br);
    if (e != HitEvent::NONE) {
      return e;
    }
  }
  return HitEvent::NONE;
}

void draw_time_and_score(sf::RenderTarget& rt, std::uint64_t kor_ido_us,
                         std::uint64_t score_ido) {
  std::uint64_t ido = (kor_ido_us + added_time) / 10000;
  std::uint64_t secs = ido / 100;
  std::uint64_t frac = ido - secs * 100;
  std::ostringstream os;
  os << "time: " << secs << ":" << (frac < 10 ? "0" : "") << frac;
  sf::Text text(os.str(), font);
  text.setCharacterSize(60);
  text.setStyle(sf::Text::Bold);
  text.setFillColor(sf::Color::Blue);
  if (!korvege) {
    text.setPosition(1020, 80);
  } else {
    text.setPosition(540, 420);
  }
  rt.draw(text);
  ido = score_ido / 10000;
  secs = ido / 100;
  os;
  std::ostringstream os2;
  os2 << "score: " << secs * 10;
  sf::Text text2(os2.str(), font);
  text2.setCharacterSize(60);
  text2.setStyle(sf::Text::Bold);
  text2.setFillColor(sf::Color::Red);
  if (!korvege) {
    text2.setPosition(1020, 200);
  } else {
    text2.setPosition(540, 540);
  }
  rt.draw(text2);
}

void check_automata() {
  if (world_y < -1 * (alja - sielo_world_y - sielo_magassag)) {
    automata = true;
    jobbra = false;
    balra = false;
  }
}

float sielo_position_x(sf::Sprite const& s) {
  float sielo_kozep_pos_x = s.getPosition().x + s.getGlobalBounds().width / 2;
  return sielo_kozep_pos_x;
}

void automata_siel(std::int64_t fus) {
  if (world_y <
      -1 * (alja - sielo_world_y - sielo_magassag + alja_cel_tavolsag + cel_kapu->getGlobalBounds().height - 200)) {
    korvege = true;
  }

  turbo = true;

  float sielo_pos;

  if (balra) {
    sielo_pos = sielo_position_x(bal_sprite);
  } else if (jobbra) {
    sielo_pos = sielo_position_x(jobb_sprite);
  } else {
    sielo_pos = sielo_position_x(egyenes_sprite);
  }

  if (sielo_pos < 640 - 1.25 * fus / 16000.0f) {
    jobbra = true;
    balra = false;
    return;
  } else if (sielo_pos > 640 + 1.25 * fus / 16000.0f) {
    balra = true;
    jobbra = false;
    return;
  } else {
    jobbra = false;
    balra = false;
    return;
  }
}

void draw_vege(sf::RenderTarget& rt) {
  rt.draw(vegrajz_sprite);
  //  std::cout << "1";
}

int main(int argc, char* argv[]) {
  init(argv[1]);
  sf::RenderWindow window(sf::VideoMode(1280, 720), "ski");
  window.setFramerateLimit(60);
  sf::Event event;
  sf::Clock c;
  sf::Clock kor_clock;
  std::uint64_t kor_ido_us = 0;
  while (true) {
    std::int64_t fus = c.restart().asMicroseconds();
    while (window.pollEvent(event)) {
      // "close requested" event: we close the window
      if (event.type == sf::Event::Closed) {
        window.close();
        return 0;
      } else if (!automata && event.type == sf::Event::KeyPressed) {
        on_key_down(event.key, window);
      } else if (!automata && event.type == sf::Event::KeyReleased) {
        on_key_up(event.key, window);
      }
    }
    if (fut) {
      switch (sielo_utkozes()) {
        case HitEvent::KEMENY:
          added_time = added_time + kemeny_buntetes;
          fut = false;
          break;
      }
      if (automata) {
        automata_siel(fus);
      } else {
        check_automata();
      }

      move(fus, turbo);
      advance_score(fus, turbo);
      sielo_move(jobbra, balra, fus);

      if (!korvege) {
        kor_ido_us = kor_clock.getElapsedTime().asMicroseconds();
      }
    }
    window.clear(sf::Color::White);
    if (korvege) {
      draw_vege(window);
    }

    draw_alsok(window);
    draw_sielo(window);
    draw_fak(window);
    draw_time_and_score(window, kor_ido_us, score_ido);
    window.display();
  }
}
