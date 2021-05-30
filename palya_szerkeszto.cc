#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

const float v_le = 50;  // pixels per second

struct hely {
  float x, y;
};

// 1280 x 720

std::vector<hely> fak;

sf::Texture fa_texture;
std::vector<sf::Sprite> fa_spriteok;

void init(std::string fn) {
  fa_texture.loadFromFile("fa.png");

  std::ifstream input{fn};

  std::string sor;

  while (std::getline(input, sor)) {
    std::string mi;
    float x, y;
    std::istringstream is{sor};
    is >> mi >> x >> y;
    if (mi != "fa") {
      std::cerr << "rossz sor: " << sor << std::endl;
    }
    fak.emplace_back(hely{x, y});
  }

  for (hely const& h : fak) {
    fa_spriteok.emplace_back(fa_texture);
    sf::Sprite& fa_sprite = fa_spriteok.back();
    fa_sprite.setScale(0.15, 0.15);
    fa_sprite.setPosition(h.x, h.y);
  }
}

void draw_fak(sf::RenderTarget& rt) {
  for (sf::Sprite& s : fa_spriteok) {
    rt.draw(s);
  }
}


int main(int argc, char* argv[]) {
  init(argv[1]);
  sf::RenderWindow window(sf::VideoMode(1280, 720), "ski");
  sf::Event event;
  while (true) {
    if (window.pollEvent(event)) {
      // "close requested" event: we close the window
      if (event.type == sf::Event::Closed) {
        window.close();
        break;
      }
    }
    window.clear(sf::Color::White);
    draw_fak(window);
    window.display();
  }
}
