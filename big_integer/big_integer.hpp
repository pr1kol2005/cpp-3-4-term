#pragma once

#include <algorithm>
#include <compare>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

class BigInt {
  using Byte = unsigned char;

 public:
  BigInt() = default;
  BigInt(int64_t num);
  BigInt(const std::string& str);
  BigInt(const BigInt& other);

  BigInt& operator=(const BigInt& rhs);

  ~BigInt() = default;

  BigInt operator+(const BigInt& rhs) const;
  BigInt operator-(const BigInt& rhs) const;
  BigInt operator*(const BigInt& rhs) const;
  BigInt operator/(const BigInt& rhs) const;
  BigInt operator%(const BigInt& rhs) const;

  BigInt& AbsSubstraction(const BigInt& rhs);

  BigInt& operator+=(const BigInt& rhs);
  BigInt& operator-=(const BigInt& rhs);
  BigInt& operator*=(const BigInt& rhs);
  BigInt& operator/=(const BigInt& rhs);
  BigInt& operator%=(const BigInt& rhs);

  bool operator==(const BigInt& rhs) const;
  std::strong_ordering operator<=>(const BigInt& rhs) const;

  BigInt& operator++();
  BigInt& operator--();
  BigInt operator++(int);
  BigInt operator--(int);

  BigInt operator-() const;

  BigInt Abs() const;

  friend std::ostream& operator<<(std::ostream& out, const BigInt& rhs);
  friend std::istream& operator>>(std::istream& in, BigInt& rhs);

 private:
  std::vector<Byte> digits_;
  bool sign_;
  const int kBase = 10;
  const std::string kInt64MinStr = "-9223372036854775808";
};
