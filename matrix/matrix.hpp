#include <array>

template <size_t N, size_t M, typename T>
class Matrix;

template <size_t N, typename T>
T GetTrace(const Matrix<N, N, T>& matrix) {
  T result = T();
  for (size_t i = 0; i < N; ++i) {
    result += matrix(i, i);
  }
  return result;
}

template <size_t N, size_t M, typename T = int64_t>
class Matrix {
 public:
  Matrix() {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        buffer_[i][j] = T();
      }
    }
  }

  Matrix(const std::vector<std::vector<T>>& data) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        buffer_[i][j] = data[i][j];
      }
    }
  }
  Matrix(T elem) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        buffer_[i][j] = elem;
      }
    }
  }

  ~Matrix() = default;

  Matrix& operator+=(const Matrix& other) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        buffer_[i][j] += other(i, j);
      }
    }
    return *this;
  }

  Matrix& operator-=(const Matrix& other) {
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        buffer_[i][j] -= other(i, j);
      }
    }
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
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        buffer_[i][j] *= other;
      }
    }
    return *this;
  }

  Matrix operator*(const T& other) const {
    Matrix result(*this);
    result *= other;
    return result;
  }

  template <size_t K>
  friend Matrix<N, K, T> operator*(const Matrix<N, M, T>& left,
                                   const Matrix<M, K, T>& right) {
    Matrix<N, K, T> result;
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < K; ++j) {
        for (size_t k = 0; k < M; ++k) {
          result(i, j) += left(i, k) * right(k, j);
        }
      }
    }
    return result;
  }

  Matrix<M, N, T> Transposed() const {
    Matrix<M, N, T> result;
    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < M; ++j) {
        result(j, i) = buffer_[i][j];
      }
    }
    return result;
  }

  T Trace() const { return GetTrace(*this); }

  T& operator()(size_t i, size_t j) { return buffer_[i][j]; }
  const T& operator()(size_t i, size_t j) const { return buffer_[i][j]; }

  bool operator==(const Matrix<N, M, T>& other) const {
    return buffer_ == other.buffer_;
  }

 private:
  std::array<std::array<T, M>, N> buffer_;
};