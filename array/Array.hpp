#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <tuple>

inline constexpr std::size_t kDynamicExtent = static_cast<std::size_t>(-1);

namespace strategy {
// TODO : Stategies

template <typename ArrayType>
struct DefaultCreation {
  template <typename... Args>
  static ArrayType create(Args&&... args) {
    return ArrayType(std::forward<Args>(args)...);
  }
  DefaultCreation() = default;
  DefaultCreation(DefaultCreation& other) = default;
  DefaultCreation& operator=(DefaultCreation& other) = default;

  // static void object_copied_response() {}

  static void object_destroyed_response() {}
};

template <typename Tuple, typename... Args, std::size_t... I>
bool ArgsMatchHelp(const Tuple& saved_args,
                   const std::tuple<Args...>& new_args) {
  return ((std::get<I>(saved_args) == std::get<I>(new_args)) && ...);
}

template <typename Tuple, typename... Args>
bool ArgsMatch(const Tuple& saved_args, const Args&... args) {
  return ArgsMatchHelp(saved_args, std::make_tuple(args...));
}

template <typename ArrayType>
struct Singleton {
 private:
  static inline ArrayType* instance{nullptr};
  static inline std::tuple<> stored_args{};

 public:
  Singleton() = default;
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

  // static void object_copied_response() {
  //   throw std::runtime_error("Singleton already created");
  // }

  static void object_destroyed_response() {
    delete instance;
    instance = nullptr;
    stored_args = {};
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

  CountedCreation() = default;

  CountedCreation(CountedCreation& other) {
    if (this != &other) {
      ++count;
    }
  }

  CountedCreation& operator=(CountedCreation& other) {
    if (this != &other) {
      ++count;
    }
    return *this;
  }

  static void object_destroyed_response() {
    if (count == 0) {
      throw std::runtime_error("All objects allready destroyed");
    }
    --count;
  }

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

// TODO : DataHolder
template <typename T, std::size_t Extent>
class DataHolder {
 protected:
  alignas(T) std::byte buffer_[Extent * sizeof(T)];

  // using ArrayType = ::Array<T, Extent, Creation>;

 public:
  std::size_t size() const { return Extent; }

  T* data() { return reinterpret_cast<T*>(buffer_); }
  const T* data() const { return reinterpret_cast<const T*>(buffer_); }

  DataHolder() = default;

  DataHolder(const DataHolder& other) {
    if (this != &other) {
      std::uninitialized_copy(other.data(), other.data() + other.size(),
                              data());
      // Creation<DataHolder>::object_copied_response();
    }
  }

  DataHolder& operator=(const DataHolder& other) {
    if (this != &other) {
      std::uninitialized_copy(other.data(), other.data() + other.size(),
                              data());
      // Creation<DataHolder>::object_copied_response();
    }
    return *this;
  }
};

template <typename T>
class DataHolder<T, kDynamicExtent> {
 protected:
  std::byte* buffer_;
  std::size_t size_;
  std::size_t capacity_;
  memres::MemoryResource* resource_;

  // using ArrayType = ::Array<T, kDynamicExtent, Creation>;

 public:
  std::size_t size() const { return size_; }

  T* data() { return reinterpret_cast<T*>(buffer_); }
  const T* data() const { return reinterpret_cast<const T*>(buffer_); }

  DataHolder() = default;

  DataHolder(const DataHolder& other) {
    if (this != &other) {
      size_ = other.size();
      capacity_ = other.size();
      resource_ = memres::GetDefaultResource();
      buffer_ = reinterpret_cast<std::byte*>(
          other.resource_->allocate(capacity_ * sizeof(T)));
      std::uninitialized_copy(other.data(), other.data() + other.size(),
                              data());
      //Creation<DataHolder>::object_copied_response();
    }
  }

  DataHolder& operator=(const DataHolder& other) {
    if (this != &other) {
      std::byte* memory = reinterpret_cast<std::byte*>(
          other.resource_->allocate(other.capacity_ * sizeof(T)));
      auto new_begin = reinterpret_cast<T*>(memory);

      std::uninitialized_copy(other.data(), other.data() + other.size(),
                              new_begin);

      std::destroy(data(), data() + size_);
      resource_->deallocate(reinterpret_cast<void*>(buffer_));

      buffer_ = memory;
      size_ = other.size_;
      capacity_ = other.capacity_;

      //Creation<DataHolder>::object_copied_response();
    }
    return *this;
  }
};

template <typename T>
class DataHolder<T, 0> {
 protected:
  // using ArrayType = ::Array<T, 0, Creation>;

 public:
  std::size_t size() const { return 0; }

  T* data() { return nullptr; }
  const T* data() const { return nullptr; }

  DataHolder() = default;

  DataHolder(const DataHolder& /*unused*/) {
    //Creation<DataHolder>::object_copied_response();
  }
  DataHolder& operator=(const DataHolder& /*unused*/) {
   //Creation<DataHolder>::object_copied_response();
    return nullptr;
  }
};

template <typename T, std::size_t Extent>
class ArrayBase : public DataHolder<T, Extent> {
  // TODO : ArrayBase
 public:
  using iterator = T*;
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  reference operator[](std::size_t index) { return *(begin() + index); }
  const_reference operator[](std::size_t index) const {
    return *(cbegin() + index);
  }

  reference at(std::size_t index) {
    if (index >= this->size()) {
      throw std::out_of_range("Out of range");
    }
    return *(begin() + index);
  }
  const_reference at(std::size_t index) const {
    if (index >= this->size()) {
      throw std::out_of_range("Out of range");
    }
    return *(cbegin() + index);
  }

  reference front() { return *begin(); }
  const_reference front() const { return *cbegin(); }

  reference back() { return *(begin() + this->size() - 1); }
  const_reference back() const { return *(cbegin() + this->size() - 1); }

  bool empty() const { return this->size() == 0; }

  iterator begin() { return this->data(); }
  const_iterator begin() const { return this->data(); }

  iterator end() { return this->data() + this->size(); }
  const_iterator end() const { return this->data() + this->size(); }

  const_iterator cbegin() const { return this->data(); }
  const_iterator cend() const { return this->data() + this->size(); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(cend());
  }

  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(cbegin());
  }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cend());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }
};

}  // namespace details

template <typename T, std::size_t Extent,
          template <typename> typename Creation = strategy::DefaultCreation>
class Array : public details::ArrayBase<T, Extent>,
              public Creation<Array<T, Extent, Creation>> {
  // TODO : Array
 public:
  ~Array() { Creation<Array>::object_destroyed_response(); }

  template <typename... Args>
  static Array create(Args&&... args) {
    std::initializer_list<T> init_list = {std::forward<Args>(args)...};
    return Creation<Array>::create(init_list);
  }

 private:
  Array(std::initializer_list<T> init_list) {
    std::uninitialized_copy(init_list.begin(), init_list.end(), this->begin());
  }

  friend class Creation<Array>;
};

template <typename T, template <typename> typename Creation>
class Array<T, kDynamicExtent, Creation>
    : public details::ArrayBase<T, kDynamicExtent>,
      public Creation<Array<T, kDynamicExtent, Creation>> {
  // TODO : Vector
 public:
  template <typename... Args>
  static Array create(Args&&... args) {
    return Creation<Array>::create(std::forward<Args>(args)...);
  }

  ~Array() {
    Creation<Array>::object_destroyed_response();
    this->clear();
    this->resource_->deallocate(reinterpret_cast<void*>(this->buffer_));
    this->buffer_ = nullptr;
    this->size_ = 0;
    this->capacity_ = 0;
  }

  void clear() {
    std::destroy(this->begin(), this->end());
    this->size_ = 0;
  }

  void push_back(const T& value) {
    if (!this->capacity_) {
      Array(1, value);
      return;
    }
    if (this->size_ == this->capacity_) {
      auto temp = this->buffer_;
      this->buffer_ = reinterpret_cast<std::byte*>(
          this->resource_->allocate(2 * this->capacity_ * sizeof(T)));
      auto new_begin = reinterpret_cast<T*>(temp);
      std::uninitialized_fill_n(this->end(), 1, value);
      std::uninitialized_move(new_begin, new_begin + this->size_,
                              this->begin());
      std::destroy(new_begin, new_begin + this->size_);
      ++this->size_;
      this->capacity_ *= 2;
      this->resource_->deallocate(reinterpret_cast<void*>(temp));
    } else {
      std::uninitialized_fill_n(this->begin() + this->size_, 1, value);
      ++this->size_;
    }
  }

  void pop_back() {
    (this->begin() + this->size_ - 1)->~T();
    --this->size_;
  }

  std::size_t capacity() const { return this->capacity_; }

  void resize(std::size_t new_size) {
    if (new_size > this->size_) {
      if (new_size <= this->capacity_) {
        std::uninitialized_default_construct(this->end(),
                                             this->begin() + new_size);
        this->size_ = new_size;
      } else {
        std::byte* memory = reinterpret_cast<std::byte*>(
            this->resource_->allocate(new_size * sizeof(T)));
        auto new_begin = reinterpret_cast<T*>(memory);
        std::uninitialized_default_construct(new_begin + this->size_,
                                             new_begin + new_size);
        std::uninitialized_move(this->begin(), this->end(), new_begin);
        std::destroy(this->begin(), this->end());
        this->resource_->deallocate(reinterpret_cast<void*>(this->buffer_));
        this->buffer_ = memory;
        this->capacity_ = new_size;
        this->size_ = new_size;
      }
    } else {
      std::destroy(this->begin() + new_size, this->begin() + this->size_);
      this->size_ = new_size;
    }
  }

  void resize(std::size_t new_size, const T& value) {
    if (new_size > this->size_) {
      if (new_size <= this->capacity_) {
        std::uninitialized_default_construct(this->end(),
                                             this->begin() + new_size);
        this->size_ = new_size;
      } else {
        std::byte* memory = reinterpret_cast<std::byte*>(
            this->resource_->allocate(new_size * sizeof(T)));
        auto new_begin = reinterpret_cast<T*>(memory);
        std::uninitialized_fill(new_begin + this->size_, new_begin + new_size,
                                value);
        std::uninitialized_move(this->begin(), this->end(), new_begin);
        std::destroy(this->begin(), this->end());
        this->resource_->deallocate(reinterpret_cast<void*>(this->buffer_));
        this->buffer_ = memory;
        this->capacity_ = new_size;
        this->size_ = new_size;
      }
    } else {
      std::destroy(this->begin() + new_size, this->begin() + this->size_);
      this->size_ = new_size;
    }
  }

  void reserve(std::size_t new_capacity) {
    if (new_capacity > this->capacity_) {
      std::byte* memory = reinterpret_cast<std::byte*>(
          this->resource_->allocate(new_capacity * sizeof(T)));
      auto new_begin = reinterpret_cast<T*>(memory);
      std::uninitialized_move(this->begin(), this->end(), new_begin);
      std::destroy(this->begin(), this->end());
      this->resource_->deallocate(reinterpret_cast<void*>(this->buffer_));
      this->buffer_ = memory;
      this->capacity_ = new_capacity;
    }
  }
  void shrink_to_fit() {
    if (!this->size_) {
      this->capacity_ = 0;
      this->resource_->deallocate(reinterpret_cast<void*>(this->buffer_));
      this->buffer_ = nullptr;
    } else {
      std::byte* memory = reinterpret_cast<std::byte*>(
          this->resource_->allocate(this->size_ * sizeof(T)));
      auto new_begin = reinterpret_cast<T*>(memory);
      std::uninitialized_move(this->begin(), this->end(), new_begin);
      std::destroy(this->begin(), this->end());
      this->resource_->deallocate(reinterpret_cast<void*>(this->buffer_));
      this->buffer_ = memory;
      this->capacity_ = this->size_;
    }
  }

 private:
  Array(memres::MemoryResource* resource = memres::GetDefaultResource()) {
    this->buffer_ = nullptr;
    this->size_ = 0;
    this->capacity_ = 0;
    this->resource_ = resource;
  }

  Array(std::size_t count,
        memres::MemoryResource* resource = memres::GetDefaultResource()) {
    this->buffer_ =
        reinterpret_cast<std::byte*>(resource->allocate(count * sizeof(T)));
    this->size_ = count;
    this->capacity_ = count;
    this->resource_ = resource;
    std::uninitialized_default_construct(this->begin(), this->end());
  }

  Array(std::size_t count, const T& value,
        memres::MemoryResource* resource = memres::GetDefaultResource()) {
    this->buffer_ =
        reinterpret_cast<std::byte*>(resource->allocate(count * sizeof(T)));
    this->size_ = count;
    this->capacity_ = count;
    this->resource_ = resource;
    std::uninitialized_fill_n(this->begin(), count, value);
  }

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
