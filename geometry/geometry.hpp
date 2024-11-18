#pragma once

#include <cstdint>

class Point;
class Segment;
class Line;
class Ray;
class Circle;

// ANCHOR Vector

class Vector {
 public:
  Vector();
  Vector(const Point& start, const Point& end);
  Vector(int64_t init_x, int64_t init_y);
  ~Vector() = default;

  int64_t operator*(const Vector& rhs) const;
  int64_t operator^(const Vector& rhs) const;

  Vector operator+(const Vector& rhs) const;
  Vector operator-(const Vector& rhs) const;
  friend Vector operator*(const Vector& vector, int64_t scalar);
  friend Vector operator*(int64_t scalar, const Vector& vector);
  Vector operator/(int64_t scalar) const;
  Vector& operator+=(const Vector& rhs);
  Vector& operator-=(const Vector& rhs);
  Vector& operator*=(int64_t scalar);
  Vector& operator/=(int64_t scalar);

  Vector operator-() const;

  int64_t GetX() const;
  int64_t GetY() const;

  bool operator==(const Vector& rhs) const;

  int64_t LengthSquared() const;

 private:
  int64_t x_;
  int64_t y_;
};

// ANCHOR IShape

class IShape {
 public:
  virtual ~IShape() = default;

  virtual IShape& Move(const Vector& vector) = 0;
  virtual bool ContainsPoint(const Point& point) const = 0;
  virtual bool CrossSegment(const Segment& segment) const = 0;
  virtual IShape* Clone() const = 0;
};

// ANCHOR Point

class Point : public IShape {
 public:
  Point();
  Point(int64_t init_x, int64_t init_y);
  Point(const Point& point);

  Point operator-() const;

  Vector operator-(const Point& rhs) const;

  int64_t GetX() const;
  int64_t GetY() const;

  ~Point() override;
  Point& Move(const Vector& vector) override;
  bool ContainsPoint(const Point& point) const override;
  bool CrossSegment(const Segment& segment) const override;
  Point* Clone() const override;

 private:
  int64_t x_;
  int64_t y_;
};

// ANCHOR Segment

class Segment : public IShape {
 public:
  Segment();
  Segment(const Point& start, const Point& end);

  Point GetA() const;
  Point GetB() const;

  ~Segment() override;
  Segment& Move(const Vector& vector) override;
  bool ContainsPoint(const Point& point) const override;
  bool CrossSegment(const Segment& segment) const override;
  Segment* Clone() const override;

 private:
  Point start_;
  Point end_;
};

// ANCHOR Line

class Line : public IShape {
 public:
  Line();
  Line(const Point& start, const Point& end);

  int64_t GetA() const;
  int64_t GetB() const;
  int64_t GetC() const;

  ~Line() override;
  Line& Move(const Vector& vector) override;
  bool ContainsPoint(const Point& point) const override;
  bool CrossSegment(const Segment& segment) const override;
  Line* Clone() const override;

 private:
  Point start_;
  Point end_;
};

// ANCHOR Ray

class Ray : public IShape {
 public:
  Ray();
  Ray(const Point& start, const Point& end);

  Point GetA() const;
  Vector GetVector() const;

  ~Ray() override;
  Ray& Move(const Vector& vector) override;
  bool ContainsPoint(const Point& point) const override;
  bool CrossSegment(const Segment& segment) const override;
  Ray* Clone() const override;

 private:
  Point start_;
  Point end_;
};

// ANCHOR Circle

class Circle : public IShape {
 public:
  Circle();
  Circle(const Point& center, int64_t radius);

  Point GetCentre() const;
  int64_t GetRadius() const;

  ~Circle() override;
  Circle& Move(const Vector& vector) override;
  bool ContainsPoint(const Point& point) const override;
  bool CrossSegment(const Segment& segment) const override;
  Circle* Clone() const override;

  bool ContainsPointWithoutHull(const Point& point) const;

 private:
  Point center_;
  int64_t radius_;
};