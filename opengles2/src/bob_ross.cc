#include <bob_ross/bob_ross.h>

namespace bob_ross {

BobRoss::BobRoss(int screen_width, int screen_height) {}
void BobRoss::UpdateScreenDimension(int screen_width, int screen_height) {}
void BobRoss::SetFillColor(Color color) {}
void BobRoss::Circle(Point origin, float radius) {}
void BobRoss::Polygon(std::vector<Point> points, std::vector<Point> indexes) {}
void BobRoss::Rect(Point top_left, Point bottom_right) {}

}  // namespace bob_ross
