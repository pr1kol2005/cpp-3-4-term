#pragma once

#include <string>
#include <vector>

class BigInteger {
  using Byte = unsigned char;

 public:
  BigInteger();

  BigInteger(int64_t num);

  BigInteger(std::string str);

  ~BigInteger() = default;

  BigInteger operator+(const BigInteger& rhs);

 private:
  std::vector<Byte> digits_;
  bool sign_;
};
