
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

void f(Point* a, Point* b) 
{
#pragma scout loop vectorize
  for (int i = 0; i < 100; ++i)
  {
    a[i] = b[i];
  }
}
