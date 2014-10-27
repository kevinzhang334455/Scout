
#include <vector>

struct Point 
{ 
  double x, y; 
};

Point operator+(const Point& p1, const Point& p2)
{
  Point result;
  result.x = p1.x + p2.x;
  result.y = p1.y + p2.y;
  return result;
}

Point operator*(const Point& p1, double x)
{
  Point result = p1;
  result.x *= x;
  result.y *= x;
  return result;
}

void f(const std::vector<Point>& a, std::vector<Point>& b) 
{
#pragma scout loop vectorize
  for (std::vector<Point>::const_iterator i = a.begin(), e = a.end(); i != e; ++i)
  {
    b.push_back(*i + (*i * 2.0));
  }
}
