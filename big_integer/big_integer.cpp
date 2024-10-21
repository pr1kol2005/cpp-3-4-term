#include "big_integer.hpp"

const std::string BigInt::kInt64MinStr =
    std::to_string(std::numeric_limits<int64_t>::min());

BigInt::BigInt(int64_t num) : BigInt(std::to_string(num)) {}

BigInt::BigInt(const std::string& str) {
  auto last = str.rend();
  if (str[0] == '-') {
    sign_ = false;
    --last;
  } else {
    sign_ = true;
  }
  for (auto it = str.rbegin(); it != last; it++) {
    digits_.push_back(*it - '0');
  }
  if (digits_.size() == 1 && digits_[0] == 0) {
    sign_ = true;
  }
  if (digits_.empty()) {
    digits_.push_back(0);
    sign_ = true;
  }
}

BigInt::BigInt(const BigInt& other)
    : sign_(other.sign_), digits_(other.digits_) {}

BigInt& BigInt::operator=(const BigInt& rhs) {
  sign_ = rhs.sign_;
  digits_ = rhs.digits_;
  return *this;
}

BigInt& BigInt::operator+=(const BigInt& rhs) {
  if (!sign_ && rhs.sign_) {
    sign_ = true;
    *this = rhs - *this;
    return *this;
  }
  if (sign_ && !rhs.sign_) {
    *this -= (-rhs);
    return *this;
  }
  int carry = 0;
  digits_.reserve(std::max(digits_.size(), rhs.digits_.size()) + 1);
  for (std::size_t i = 0; i < digits_.size(); i++) {
    int curr = 0;
    if (i < digits_.size()) {
      curr += digits_[i];
    }
    if (i < rhs.digits_.size()) {
      curr += rhs.digits_[i];
    }
    curr += carry;
    digits_[i] = (curr % kBase);
    carry = curr / kBase;
  }
  if (carry != 0) {
    digits_.push_back(carry);
  }
  return *this;
}

BigInt BigInt::operator+(const BigInt& rhs) const {
  BigInt temp(*this);
  temp += rhs;
  return temp;
}

void BigInt::RemoveZeros() {
  while (digits_.size() > 1 && digits_.back() == 0) {
    digits_.pop_back();
  }
}

BigInt& BigInt::AbsSubstraction(const BigInt& rhs) {
  int carry = 0;
  for (std::size_t i = 0; i < digits_.size(); i++) {
    int64_t curr = 0;
    if (i < digits_.size()) {
      curr += digits_[i];
    }
    if (i < rhs.digits_.size()) {
      curr -= rhs.digits_[i];
    }
    curr -= carry;
    if (curr < 0) {
      curr += kBase;
      carry = 1;
    } else {
      carry = 0;
    }
    digits_[i] = (curr % kBase);
  }
  RemoveZeros();
  return *this;
}

BigInt& BigInt::operator-=(const BigInt& rhs) {
  if (!sign_ && rhs.sign_) {
    sign_ = true;
    *this += rhs;
    sign_ = !sign_;
    return *this;
  }
  if (sign_ && !rhs.sign_) {
    *this += (-rhs);
    return *this;
  }
  if (!sign_ && !rhs.sign_) {
    *this = -rhs - -*this;
    return *this;
  }
  if (rhs > *this) {
    *this = rhs - *this;
    sign_ = !sign_;
    return *this;
  }
  return AbsSubstraction(rhs);
}

BigInt BigInt::operator-(const BigInt& rhs) const {
  BigInt temp(*this);
  temp -= rhs;
  return temp;
}

BigInt& BigInt::operator*=(const BigInt& rhs) {
  if (*this == 0 || rhs == 0) {
    *this = 0;
    return *this;
  }
  std::vector<int> column_sum(digits_.size() + rhs.digits_.size(), 0);

  for (std::size_t i = 0; i < digits_.size(); i++) {
    for (std::size_t j = 0; j < rhs.digits_.size(); j++) {
      column_sum[i + j] += digits_[i] * rhs.digits_[j];
    }
  }

  int carry = 0;
  int curr = 0;
  digits_.resize(column_sum.size());

  for (std::size_t i = 0; i < column_sum.size(); i++) {
    curr = column_sum[i] + carry;
    digits_[i] = curr % kBase;
    carry = curr / kBase;
  }

  RemoveZeros();

  sign_ = (sign_ == rhs.sign_);
  return *this;
}

BigInt BigInt::operator*(const BigInt& rhs) const {
  BigInt temp(*this);
  temp *= rhs;
  return temp;
}

BigInt& BigInt::operator/=(const BigInt& rhs) {
  if (Abs() < rhs.Abs()) {
    *this = 0;
    return *this;
  }

  int curr = 0;
  BigInt temp = 0;
  std::vector<int8_t> column_sum;
  int i = digits_.size() - 1;

  for (; temp * kBase + digits_[i] < rhs; --i) {
    temp *= kBase;
    temp += digits_[i];
  }
  for (; i >= 0; --i) {
    temp = temp * kBase + digits_[i];
    for (curr = kBase - 1; BigInt(curr) * rhs > temp; --curr) {
    }
    temp -= BigInt(curr) * rhs;
    column_sum.push_back(curr);
  }
  digits_ = column_sum;
  std::reverse(digits_.begin(), digits_.end());
  sign_ = (sign_ == rhs.sign_);
  return *this;
}

BigInt BigInt::operator/(const BigInt& rhs) const {
  BigInt temp(*this);
  temp /= rhs;
  return temp;
}

BigInt& BigInt::operator%=(const BigInt& rhs) {
  bool new_sign = sign_;
  sign_ = true;
  if (*this < rhs.Abs()) {
    sign_ = new_sign;
    return *this;
  }

  int curr = 0;
  BigInt temp = 0;
  int i = digits_.size() - 1;

  for (; temp * kBase + digits_[i] < rhs; --i) {
    temp *= kBase;
    temp += digits_[i];
  }
  while (i >= 0) {
    temp = temp * kBase + digits_[i];
    for (curr = kBase - 1; BigInt(curr) * rhs > temp; --curr) {
    }
    temp -= BigInt(curr) * rhs;
    --i;
  }
  *this = temp;
  sign_ = new_sign;
  return *this;
}

BigInt BigInt::operator%(const BigInt& rhs) const {
  BigInt temp(*this);
  temp %= rhs;
  return temp;
}

BigInt& BigInt::operator++() {
  *this += 1;
  return *this;
}

BigInt BigInt::operator++(int) {
  BigInt temp(*this);
  *this += 1;
  return temp;
}

BigInt& BigInt::operator--() {
  *this -= 1;
  return *this;
}

BigInt BigInt::operator--(int) {
  BigInt temp(*this);
  *this -= 1;
  return temp;
}

bool BigInt::operator==(const BigInt& rhs) const {
  if (sign_ != rhs.sign_) {
    return false;
  }
  return digits_ == rhs.digits_;
}

std::strong_ordering BigInt::operator<=>(const BigInt& rhs) const {
  if (!sign_ && rhs.sign_) {
    return std::strong_ordering::less;
  }
  if (sign_ && !rhs.sign_) {
    return std::strong_ordering::greater;
  }

  std::vector<int8_t> lhs_reversed = digits_;
  std::vector<int8_t> rhs_reversed = rhs.digits_;
  std::reverse(lhs_reversed.begin(), lhs_reversed.end());
  std::reverse(rhs_reversed.begin(), rhs_reversed.end());

  if (!sign_ && !rhs.sign_) {
    if (digits_.size() != rhs.digits_.size()) {
      return rhs.digits_.size() <=> digits_.size();
    }
    return (rhs_reversed <=> lhs_reversed);
  }
  if (digits_.size() != rhs.digits_.size()) {
    return digits_.size() <=> rhs.digits_.size();
  }
  return (lhs_reversed <=> rhs_reversed);
}

BigInt BigInt::operator-() const {
  BigInt res(*this);
  if (digits_.size() != 1 || digits_[0] != 0) {
    res.sign_ = !res.sign_;
  }
  return res;
}

BigInt BigInt::Abs() const {
  BigInt res(*this);
  res.sign_ = true;
  return res;
}

std::ostream& operator<<(std::ostream& out, const BigInt& rhs) {
  if (!rhs.sign_) {
    out << '-';
  }
  for (auto it = rhs.digits_.rbegin(); it != rhs.digits_.rend(); it++) {
    out << static_cast<int8_t>(*it + '0');
  }
  return out;
}

std::istream& operator>>(std::istream& in, BigInt& rhs) {
  std::string str;
  in >> str;
  rhs = BigInt(str);
  return in;
}
