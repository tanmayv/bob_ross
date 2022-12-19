#pragma once

#include <vector>

namespace bob_ross {

struct Point {
  float x, y, z = 0.0f;
};

class BobRoss {
 public:
  BobRoss(int screen_width, int screen_height);
  void UpdateScreenDimension(int screen_width, int screen_height);
  void SetFillColor(int r, int g, int b, int a);
  void Circle(Point origin, float radius);
  void Polygon(std::vector<Point> points, std::vector<Point> indexes);
  void Rect(Point top_left, Point bottom_right);

 private:
  int screen_width_, screen_height_;
};

}  // namespace bob_ross
