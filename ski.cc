#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <vector>

const float v_le = 50;  // pixels per second

struct hely {
  float x, y;
};

// 1280 x 720

std::vector<hely> fak{
    {120.0, 75.0},
    {520.0, 100.0},
    {320.0, 125.0},
    {920.0, 150.0},
};

void draw_triangle(sf::RenderTarget &rt) {
  sf::ConvexShape polygon;
  polygon.setPointCount(3);
  polygon.setPoint(0, sf::Vector2f(0, 0));
  polygon.setPoint(1, sf::Vector2f(0, 10));
  polygon.setPoint(2, sf::Vector2f(25, 5));
  polygon.setOutlineColor(sf::Color::Red);
  polygon.setOutlineThickness(5);
  polygon.setPosition(10, 20);

  rt.draw(polygon);
}

sf::Texture fa_texture;
std::vector<sf::Sprite> fa_spriteok;

void init() {
  fa_texture.loadFromFile("fa.png");

  for (hely const& h: fak) {
    fa_spriteok.emplace_back(fa_texture);
    sf::Sprite& fa_sprite = fa_spriteok.back();
    fa_sprite.setScale(0.15, 0.15);
    fa_sprite.setPosition(h.x, h.y);
  } 

}

void draw_fak(sf::RenderTarget &rt) {
  for (sf::Sprite& s: fa_spriteok) {
    rt.draw(s);
  }
}

void move_fak(sf::Time ft) {
    float dy = v_le * ft.asMicroseconds() / -1000000.0;
    for (sf::Sprite& s: fa_spriteok) {
      s.move(0.0, dy);
    }
}

int main() {
  init();
  sf::RenderWindow window(sf::VideoMode(1280, 720), "ski");
  sf::Event event;
  sf::Clock frame_clock;
  while (true) {
    if (window.pollEvent(event)) {
      // "close requested" event: we close the window
      if (event.type == sf::Event::Closed) {
        window.close();
        break;
      
      }
    }
    sf::Time ft = frame_clock.restart();
    move_fak(ft);
    window.clear(sf::Color::White);
    draw_triangle(window);
    draw_fak(window);
    window.display();
  }
}
