#include "geometry.hpp"

// ANCHOR Functions

bool IsPointOnLine(const Point& start, const Point& end, const Point& point) {
  Vector a_b(start, end);
  Vector a_p(start, point);
  if (a_b.LengthSquared() != 0) {
    return (a_b ^ a_p) == 0;
  }
  return start.ContainsPoint(point);
};

bool IsPointOnSegment(const Point& start, const Point& end,
                      const Point& point) {
  Vector a_b(start, end);
  Vector a_p(start, point);
  if (a_b.LengthSquared() != 0) {
    if (!IsPointOnLine(start, end, point)) {
      return false;
    }
    int64_t scalar_product = a_b * a_p;
    if (scalar_product < 0) {
      return false;
    }
    return scalar_product <= a_b.LengthSquared();
  }
  return start.ContainsPoint(point);
};

bool IsSegmentCrossesLine(const Point& start_i, const Point& end_i,
                          const Point& start_ii, const Point& end_ii) {
  Vector a_b(start_i, end_i);
  Vector a_c(start_i, start_ii);
  Vector a_d(start_i, end_ii);
  return (a_b ^ a_c) * (a_b ^ a_d) <= 0;
};

bool IntersectSegments(const Point& start_1_a, const Point& end_1_b,
                       const Point& start_2_c, const Point& end_2_d) {
  Vector a_b(start_1_a, end_1_b);
  Vector a_c(start_1_a, start_2_c);
  Vector a_d(start_1_a, end_2_d);
  Vector c_d(start_2_c, end_2_d);
  Vector c_a(start_2_c, start_1_a);
  Vector c_b(start_2_c, end_1_b);
  if (a_b.LengthSquared() != 0 && c_d.LengthSquared() != 0) {
    if (IsPointOnSegment(start_1_a, end_1_b, start_2_c) ||
        IsPointOnSegment(start_1_a, end_1_b, end_2_d) ||
        IsPointOnSegment(start_2_c, end_2_d, start_1_a) ||
        IsPointOnSegment(start_2_c, end_2_d, end_1_b)) {
      return true;
    }
    if ((a_b ^ a_c) == 0 && (a_b ^ a_d) == 0 && (c_d ^ c_a) == 0 &&
        (c_d ^ c_b) == 0) {
      return IsPointOnSegment(start_1_a, end_1_b, start_2_c) ||
             IsPointOnSegment(start_1_a, end_1_b, end_2_d) ||
             (IsPointOnSegment(start_2_c, end_2_d, start_1_a) &&
              IsPointOnSegment(start_2_c, end_2_d, end_1_b));
    }
    return IsSegmentCrossesLine(start_1_a, end_1_b, start_2_c, end_2_d) &&
           IsSegmentCrossesLine(start_2_c, end_2_d, start_1_a, end_1_b);
  }
  if (a_b.LengthSquared() != 0) {
    return IsPointOnSegment(start_1_a, end_1_b, start_2_c);
  }
  if (c_d.LengthSquared() != 0) {
    return IsPointOnSegment(start_2_c, end_2_d, start_1_a);
  }
  return start_1_a.ContainsPoint(start_2_c);
};

bool IsPointOnRay(const Point& start_a, const Point& end_b,
                  const Point& point) {
  if (!IsPointOnLine(start_a, end_b, point)) {
    return false;
  }
  Vector a_b(start_a, end_b);
  Vector a_p(start_a, point);
  if (a_b.LengthSquared() != 0) {
    return (a_b * a_p) >= 0;
  }
  return start_a.ContainsPoint(point);
};

bool IsSegmentCrossesRay(const Point& start_a, const Point& end_b,
                         const Point& start_c, const Point& end_d) {
  if (IsPointOnRay(start_a, end_b, start_c) ||
      IsPointOnRay(start_a, end_b, end_d)) {
    return true;
  }
  Vector a_b(start_a, end_b);
  Vector a_c(start_a, start_c);
  Vector a_d(start_a, end_d);
  Vector c_d(start_c, end_d);
  Vector c_a(start_c, start_a);
  Vector c_b(start_c, end_b);
  if (a_b.LengthSquared() != 0 && c_d.LengthSquared() != 0) {
    return ((a_b ^ a_c) * (a_b ^ a_d) <= 0) && ((c_d ^ c_a) * (c_d ^ c_b) <= 0);
  }
  if (a_b.LengthSquared() != 0) {
    return IsPointOnRay(start_a, end_b, start_c);
  }
  if (c_d.LengthSquared() != 0) {
    return Segment(start_c, end_d).ContainsPoint(start_a);
  }
  return start_a.ContainsPoint(start_c);
};

// ANCHOR Vector

Vector::Vector() = default;

Vector::Vector(int64_t xxx, int64_t yyy) : x_(xxx), y_(yyy) {}

Vector::Vector(const Point& start, const Point& end)
    : x_(end.GetX() - start.GetX()), y_(end.GetY() - start.GetY()) {}

int64_t Vector::operator*(const Vector& rhs) const {
  return (x_ * rhs.x_) + (y_ * rhs.y_);
}

int64_t Vector::operator^(const Vector& rhs) const {
  return (x_ * rhs.y_) - (y_ * rhs.x_);
}

Vector Vector::operator+(const Vector& rhs) const {
  Vector result = *this;
  result += rhs;
  return result;
}

Vector Vector::operator-(const Vector& rhs) const {
  Vector result = *this;
  result -= rhs;
  return result;
}

Vector operator*(const Vector& vector, int64_t scalar) {
  Vector result = vector;
  result *= scalar;
  return result;
}
Vector operator*(int64_t scalar, const Vector& vector) {
  return vector * scalar;
}

Vector Vector::operator/(int64_t scalar) const {
  Vector result = *this;
  result /= scalar;
  return result;
}

Vector& Vector::operator+=(const Vector& rhs) {
  x_ += rhs.x_;
  y_ += rhs.y_;
  return *this;
}

Vector& Vector::operator-=(const Vector& rhs) {
  x_ -= rhs.x_;
  y_ -= rhs.y_;
  return *this;
}

Vector& Vector::operator*=(int64_t scalar) {
  x_ *= scalar;
  y_ *= scalar;
  return *this;
}

Vector& Vector::operator/=(int64_t scalar) {
  x_ /= scalar;
  y_ /= scalar;
  return *this;
}

Vector Vector::operator-() const {
  Vector result(-x_, -y_);
  return result;
}

int64_t Vector::GetX() const { return x_; }

int64_t Vector::GetY() const { return y_; }

bool Vector::operator==(const Vector& rhs) const {
  return x_ == rhs.x_ && y_ == rhs.y_;
}

int64_t Vector::LengthSquared() const { return (x_ * x_) + (y_ * y_); }

// ANCHOR Point

Point::Point() = default;

Point::Point(int64_t xxx, int64_t yyy) : x_(xxx), y_(yyy) {}

Point::Point(const Point& point) : x_(point.x_), y_(point.y_) {}

Point Point::operator-() const {
  Point result(-x_, -y_);
  return result;
}

Vector Point::operator-(const Point& rhs) const {
  Vector result = Vector(rhs, *this);
  return result;
}

int64_t Point::GetX() const { return x_; }

int64_t Point::GetY() const { return y_; }

Point::~Point() = default;

Point& Point::Move(const Vector& vector) {
  x_ += vector.GetX();
  y_ += vector.GetY();
  return *this;
}

bool Point::ContainsPoint(const Point& point) const {
  return (point.x_ == x_ && point.y_ == y_);
}

bool Point::CrossSegment(const Segment& segment) const {
  return IsPointOnSegment(segment.GetA(), segment.GetB(), *this);
}

Point* Point::Clone() const {
  Point* result = new Point(x_, y_);
  return result;
}

// ANCHOR Segment

Segment::Segment() = default;

Segment::Segment(const Point& start, const Point& end) : a_(start), b_(end) {}

Point Segment::GetA() const { return a_; }

Point Segment::GetB() const { return b_; }

Segment::~Segment() = default;

Segment& Segment::Move(const Vector& vector) {
  a_.Move(vector);
  b_.Move(vector);
  return *this;
}

bool Segment::ContainsPoint(const Point& point) const {
  return IsPointOnSegment(a_, b_, point);
}

bool Segment::CrossSegment(const Segment& segment) const {
  return IntersectSegments(a_, b_, segment.a_, segment.b_);
}

Segment* Segment::Clone() const {
  Segment* result = new Segment(a_, b_);
  return result;
}

// ANCHOR Line

Line::Line() = default;

Line::Line(const Point& start, const Point& end) : a_(start), b_(end) {}

int64_t Line::GetA() const { return a_.GetY() - b_.GetY(); }

int64_t Line::GetB() const { return b_.GetX() - a_.GetX(); }

int64_t Line::GetC() const {
  return -(GetA() * a_.GetX()) - (GetB() * a_.GetY());
}

Line::~Line() = default;

Line& Line::Move(const Vector& vector) {
  a_.Move(vector);
  b_.Move(vector);
  return *this;
}

bool Line::ContainsPoint(const Point& point) const {
  return IsPointOnLine(a_, b_, point);
}

bool Line::CrossSegment(const Segment& segment) const {
  return IsSegmentCrossesLine(a_, b_, segment.GetA(), segment.GetB());
}

Line* Line::Clone() const {
  Line* result = new Line(a_, b_);
  return result;
}

// ANCHOR Ray

Ray::Ray() = default;

Ray::Ray(const Point& start, const Point& end) : a_(start), b_(end) {}

Point Ray::GetA() const { return a_; }

Vector Ray::GetVector() const { return b_ - a_; }

Ray::~Ray() = default;

Ray& Ray::Move(const Vector& vector) {
  a_.Move(vector);
  b_.Move(vector);
  return *this;
}

bool Ray::ContainsPoint(const Point& point) const {
  return IsPointOnRay(a_, b_, point);
}

bool Ray::CrossSegment(const Segment& segment) const {
  return IsSegmentCrossesRay(a_, b_, segment.GetA(), segment.GetB());
}

Ray* Ray::Clone() const {
  Ray* result = new Ray(a_, b_);
  return result;
}

// ANCHOR Circle

Circle::Circle() = default;

Circle::Circle(const Point& center, int64_t radius)
    : center_(center), radius_(radius) {}

Point Circle::GetCentre() const { return center_; }

int64_t Circle::GetRadius() const { return radius_; }

Circle::~Circle() = default;

Circle& Circle::Move(const Vector& vector) {
  center_.Move(vector);
  return *this;
}

bool Circle::ContainsPoint(const Point& point) const {
  return (point - center_).LengthSquared() <= radius_ * radius_;
}

bool Circle::ContainsPointWithoutHull(const Point& point) const {
  return (point - center_).LengthSquared() < radius_ * radius_;
}

bool Circle::CrossSegment(const Segment& segment) const {
  Vector a_b(segment.GetA(), segment.GetB());
  Vector b_a(segment.GetB(), segment.GetA());
  Vector a_o(segment.GetA(), center_);
  Vector b_o(segment.GetB(), center_);
  if (ContainsPointWithoutHull(segment.GetA()) &&
      ContainsPointWithoutHull(segment.GetB())) {
    return false;
  }
  if ((a_o * a_b) <= 0) {
    return ContainsPoint(segment.GetA());
  }
  if ((b_o * b_a) <= 0) {
    return ContainsPoint(segment.GetB());
  }
  return ((a_o ^ a_b) * (a_o ^ a_b) <= a_b.LengthSquared() * radius_ * radius_);
}

Circle* Circle::Clone() const {
  Circle* result = new Circle(center_, radius_);
  return result;
}
