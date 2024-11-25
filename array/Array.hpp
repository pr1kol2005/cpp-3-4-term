#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <iterator>

inline constexpr std::size_t kDynamicExtent = static_cast<std::size_t>(-1);

namespace strategy {
// TODO : Stategies

template <typename ArrayType>
struct DefaultCreation {
  template <typename... Args>
  static ArrayType create(Args&&... args) {
    return ArrayType(std::forward<Args>(args)...);
  }

  // static void copy(ArrayType& lhs, const ArrayType& rhs) {
  //   // TODO : copy
  // }
};

template <typename Tuple, typename... Args, std::size_t... I>
bool args_match_help(const Tuple& saved_args,
                     const std::tuple<Args...>& new_args,
                     std::index_sequence<I...>) {
  return ((std::get<I>(saved_args) == std::get<I>(new_args)) && ...);
}

template <typename Tuple, typename... Args>
bool args_match(const Tuple& saved_args, const Args&... args) {
  return args_match_help(saved_args, std::make_tuple(args...),
                         std::index_sequence_for<Args...>{});
}

template <typename ArrayType>
struct Singleton {
 private:
  static inline ArrayType* instance{nullptr};
  static inline std::tuple<> stored_args{};

 public:
  Singleton(Singleton& other) = delete;
  Singleton& operator=(Singleton& other) = delete;

  template <typename... Args>
  static ArrayType create(Args&&... args) {
    if (!instance) {
      instance = new ArrayType(std::forward<Args>(args)...);
      stored_args = std::make_tuple(args...);
    } else if (!args_math(stored_args, std::forward<Args>(args)...)) {
      throw std::runtime_error("Singleton already created");
    }
    return *instance;
  }
};

template <typename ArrayType>
struct CountedCreation {
  static inline std::size_t count{0};

  template <typename... Args>
  static ArrayType create(Args&&... args) {
    ++count;
    return ArrayType(std::forward<Args>(args)...);
  }

  // static void copy(ArrayType& lhs, const ArrayType& rhs) {
  //   // TODO : copy
  //   ++count;
  // }

  static std::size_t get_created_count() { return count; }
};

}  // namespace strategy

namespace memres {

class MemoryResource {
 public:
  virtual void* allocate(std::size_t count) = 0;
  virtual void deallocate(void* ptr) = 0;
  virtual ~MemoryResource() = default;
};

class NewDeleteResource : public MemoryResource {
 public:
  void* allocate(std::size_t count) override { return ::operator new(count); }
  void deallocate(void* ptr) override { ::operator delete(ptr); }
};

class MallocFreeResource : public MemoryResource {
 public:
  void* allocate(std::size_t count) override { return std::malloc(count); }
  void deallocate(void* ptr) override { std::free(ptr); }
};

MemoryResource* default_resource = new NewDeleteResource();

MemoryResource* GetDefaultResource() { return default_resource; }

MemoryResource* SetDefaultResource(MemoryResource* resource) {
  MemoryResource* old_resource = default_resource;
  default_resource = resource;
  return old_resource;
}

}  // namespace memres

namespace details {

template <typename T>
class ArrayIterator {
  // TODO : ArrayIterator
};

template<typename T, std::size_t Extent>
class DataHolder {
 protected:
  T buffer_[Extent];
};

template<typename T>
class DataHolder<T, kDynamicExtent> {
 protected:
  T* buffer_;
  std::size_t size_;
  std::size_t capacity_;
  memres::MemoryResource* resource_;
};

template<typename T>
class DataHolder<T, 0> {
};

template <typename T, std::size_t Extent, template <typename> typename Creation>
class ArrayBase : public DataHolder<T, Extent> {
  // TODO : ArrayBase
 public:
  using iterator = ArrayIterator<T>;
  using const_iterator = ArrayIterator<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  T& operator[](std::size_t index);
  const T& operator[](std::size_t index) const;

  T& at(std::size_t index);
  const T& at(std::size_t index) const;

  T* data();
  const T* data() const;

  T& front();
  const T& front() const;

  T& back();
  const T& back() const;

  std::size_t size() const;

  bool empty() const;

  iterator begin();
  const_iterator begin() const;

  iterator end();
  const_iterator end() const;

  const_iterator cbegin() const;
  const_iterator cend() const;

  reverse_iterator rbegin();
  const_reverse_iterator rbegin() const;

  reverse_iterator rend();
  const_reverse_iterator rend() const;

  const_reverse_iterator crbegin() const;
  const_reverse_iterator crend() const;
};

}  // namespace details

template <typename T, std::size_t Extent, template <typename> typename Creation = strategy::DefaultCreation>
class Array : public details::ArrayBase<T, Extent, Creation>, public Creation<Array<T, Extent, Creation>> {
  // TODO : Array
 public:
  template <typename... Args>
  static Array create(Args&&... args) {
    return Creation<Array>::create(std::forward<Args>(args)...);
  }

 private:
  // TODO : Make visiable to strategy class
  template <typename... Args>
  Array(Args&&... args) {
    static_assert(Extent >= sizeof...(args), "Too many arguments");
    std::size_t index = 0;
    ((this->buffer_[index++] = std::forward<Args>(args)), ...);
  }

  friend class Creation<Array>;
};

template <typename T, template <typename> typename Creation>
class Array<T, kDynamicExtent, Creation>
    : public details::ArrayBase<T, kDynamicExtent, Creation>, public Creation<Array<T, kDynamicExtent, Creation>> {
  // TODO : Vector
 public:
  template <typename... Args>
  static Array create(Args&&... args) {
    return Creation<Array>::create(std::forward<Args>(args)...);
  }

  void push_back(T);
  void pop_back();
  std::size_t capacity() const;
  void resize(std::size_t new_size);
  void reserve(std::size_t new_capacity);
  void clean();
  void shrink_to_fit();

 private:
  Array(std::size_t count = 0, const T& value = T(),
        memres::MemoryResource* resource = memres::GetDefaultResource());

  friend class Creation<Array>;
};

namespace traits {
// TODO : Edit traits

// template <typename T>
// std::size_t GetSize(T) {
//   return 0;
// }

// template <typename T>
// std::size_t GetRank(T) {
//   return 0;
// }

// template <typename T, std::size_t N>
// std::size_t GetRank(const T (&array)[N]) {
//   std::size_t rank = 0;
//   if (GetSize(array) == 0) {
//     return rank;
//   }
//   rank = 1 + GetRank(array[0]);
//   return rank;
// }

// template <typename T>
// std::size_t GetTotalElements(T) {
//   return 1;
// }

// template <typename T, std::size_t N>
// std::size_t GetTotalElements(const T (&array)[N]) {
//   std::size_t num_elem = 1;
//   num_elem *= (GetSize(array) * GetTotalElements(array[0]));
//   return num_elem;
// }

// TODO : GetExtent
// template <typename I>
// std::size_t GetExtent(Array array) {
//   return 0;
// }

}  // namespace traits
