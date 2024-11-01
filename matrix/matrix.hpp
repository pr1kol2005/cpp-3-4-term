#include <array>
#include <vector>

template <std::size_t N, std::size_t M, typename T>
class Matrix;

namespace matrix_utils {
template <std::size_t N, typename T>
T GetTrace(const Matrix<N, N, T>& matrix) {
  T result = T();
  for (std::size_t i = 0; i < N; ++i) {
    result += matrix(i, i);
  }
  return result;
}
}  // namespace matrix_utils

template <std::size_t N, std::size_t M, typename T = int64_t>
class Matrix {
  using Buffer = std::array<std::array<T, M>, N>;

 public:
  Matrix() {
    DoForEveryElement([](std::size_t i, std::size_t j, Buffer& buffer) {
      buffer[i][j] = T();
    });
  }

  Matrix(const std::vector<std::vector<T>>& data) {
    DoForEveryElement(
        [](std::size_t i, std::size_t j, Buffer& buffer,
           const std::vector<std::vector<T>>& data) {
          buffer[i][j] = data[i][j];
        },
        data);
  }
  Matrix(T elem) {
    DoForEveryElement([](std::size_t i, std::size_t j, Buffer& buffer,
                         T elem) { buffer[i][j] = elem; },
                      elem);
  }

  ~Matrix() = default;

  Matrix& operator+=(const Matrix& other) {
    DoForEveryElement([](std::size_t i, std::size_t j, Buffer& buffer,
                         const Matrix& other) { buffer[i][j] += other(i, j); },
                      other);
    return *this;
  }

  Matrix& operator-=(const Matrix& other) {
    DoForEveryElement([](std::size_t i, std::size_t j, Buffer& buffer,
                         const Matrix& other) { buffer[i][j] -= other(i, j); },
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
    DoForEveryElement([](std::size_t i, std::size_t j, Buffer& buffer,
                         const T& other) { buffer[i][j] *= other; },
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

  T Trace() const { return matrix_utils::GetTrace(*this); }

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
  void DoForEveryElement(Func func, Args... args) {
    for (std::size_t i = 0; i < N; ++i) {
      for (std::size_t j = 0; j < M; ++j) {
        func(i, j, buffer_, args...);
      }
    }
  }

  Buffer buffer_;
};