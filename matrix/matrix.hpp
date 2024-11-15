#include <array>
#include <cstdint>
#include <vector>

template <std::size_t N, std::size_t M, typename T>
class Matrix;

namespace utils {
template <std::size_t N, typename T>
T GetTrace(const Matrix<N, N, T>& matrix) {
  T result = T();
  for (std::size_t i = 0; i < N; ++i) {
    result += matrix(i, i);
  }
  return result;
}
}  // namespace utils

template <std::size_t N, std::size_t M, typename T = int64_t>
class Matrix {
  using Buffer = std::array<std::array<T, M>, N>;

 public:
  Matrix() {
    DoForEveryElement([](std::size_t index_i, std::size_t index_j,
                         Buffer& buffer) { buffer[index_i][index_j] = T(); });
  }

  Matrix(const std::vector<std::vector<T>>& data) {
    DoForEveryElement(
        [](std::size_t index_i, std::size_t index_j, Buffer& buffer,
           const std::vector<std::vector<T>>& data) {
          buffer[index_i][index_j] = data[index_i][index_j];
        },
        data);
  }
  Matrix(const T& elem) {
    DoForEveryElement(
        [](std::size_t index_i, std::size_t index_j, Buffer& buffer,
           const T& elem) { buffer[index_i][index_j] = elem; },
        elem);
  }

  ~Matrix() = default;

  Matrix& operator+=(const Matrix& other) {
    DoForEveryElement(
        [](std::size_t index_i, std::size_t index_j, Buffer& buffer,
           const Matrix& other) {
          buffer[index_i][index_j] += other(index_i, index_j);
        },
        other);
    return *this;
  }

  Matrix& operator-=(const Matrix& other) {
    DoForEveryElement(
        [](std::size_t index_i, std::size_t index_j, Buffer& buffer,
           const Matrix& other) {
          buffer[index_i][index_j] -= other(index_i, index_j);
        },
        other);
    return *this;
  }

  Matrix operator+(const Matrix& other) const {
    Matrix result(*this);
    result += other;
    return result;
  }

  Matrix operator-(const Matrix& other) const {
    Matrix result(*this);
    result -= other;
    return result;
  }

  Matrix& operator*=(const T& other) {
    DoForEveryElement(
        [](std::size_t index_i, std::size_t index_j, Buffer& buffer,
           const T& other) { buffer[index_i][index_j] *= other; },
        other);
    return *this;
  }

  Matrix operator*(const T& other) const {
    Matrix result(*this);
    result *= other;
    return result;
  }

  template <std::size_t K>
  friend Matrix<N, K, T> operator*(const Matrix<N, M, T>& left,
                                   const Matrix<M, K, T>& right) {
    Matrix<N, K, T> result;
    for (std::size_t i = 0; i < N; ++i) {
      for (std::size_t j = 0; j < K; ++j) {
        for (std::size_t k = 0; k < M; ++k) {
          result(i, j) += left(i, k) * right(k, j);
        }
      }
    }
    return result;
  }

  Matrix<M, N, T> Transposed() const {
    Matrix<M, N, T> result;
    for (std::size_t i = 0; i < N; ++i) {
      for (std::size_t j = 0; j < M; ++j) {
        result(j, i) = buffer_[i][j];
      }
    }
    return result;
  }

  T Trace() const { return utils::GetTrace(*this); }

  T& operator()(std::size_t row, std::size_t column) {
    return buffer_[row][column];
  }
  const T& operator()(std::size_t row, std::size_t column) const {
    return buffer_[row][column];
  }

  bool operator==(const Matrix<N, M, T>& other) const {
    return buffer_ == other.buffer_;
  }

 private:
  template <typename Func, typename... Args>
  void DoForEveryElement(Func func, Args&&... args) {
    for (std::size_t i = 0; i < N; ++i) {
      for (std::size_t j = 0; j < M; ++j) {
        func(i, j, buffer_, args...);
      }
    }
  }

  Buffer buffer_;
};