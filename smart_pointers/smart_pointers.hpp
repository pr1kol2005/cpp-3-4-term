#define WEAK_PTR_IMPLEMENTED

#pragma once

#include <stdexcept>

class BadWeakPtr : public std::runtime_error {
 public:
  BadWeakPtr() : std::runtime_error("BadWeakPtr") {}
};

template <class T>
class WeakPtr;

template <class T>
class SharedPtr;

struct Counter {
  size_t strong_count = 0;
  size_t weak_count = 0;
};

template <class T>
class SharedPtr {
  T* ptr_;
  Counter* counter_;

  friend class WeakPtr<T>;

 public:
  SharedPtr() : ptr_(nullptr), counter_(nullptr) {}

  SharedPtr(std::nullptr_t) : ptr_(nullptr), counter_(nullptr) {}

  explicit SharedPtr(T* pointer) : ptr_(pointer), counter_(new Counter{1, 0}) {}

  SharedPtr(const WeakPtr<T>& other) {
    if (other.expired()) {
      throw BadWeakPtr{};
    }
    ptr_ = other.ptr_;
    counter_ = other.counter_;
    if (counter_ != nullptr) {
      counter_->strong_count++;
    }
  }

  SharedPtr(const SharedPtr& other) {
    ptr_ = other.ptr_;
    counter_ = other.counter_;
    if (counter_ != nullptr) {
      counter_->strong_count++;
    }
  }

  SharedPtr& operator=(const SharedPtr& other) {
    if (this != &other) {
      reset();
      ptr_ = other.ptr_;
      counter_ = other.counter_;
      if (counter_ != nullptr) {
        counter_->strong_count++;
      }
    }
    return *this;
  }

  SharedPtr(SharedPtr&& other) noexcept : ptr_(nullptr), counter_(nullptr) {
    swap(other);
  }

  SharedPtr& operator=(SharedPtr&& other) noexcept {
    if (this != &other) {
      reset();
      swap(other);
    }
    return *this;
  }

  ~SharedPtr() { reset(); }

  void reset(T* pointer = nullptr) {
    if (counter_ != nullptr) {
      counter_->strong_count--;
      if (counter_->strong_count == 0) {
        if (counter_->weak_count == 0) {
          delete counter_;
        }
        delete ptr_;
      }
    }
    ptr_ = pointer;
    counter_ = pointer ? new Counter{1, 0} : nullptr;
  }

  void swap(SharedPtr& other) {
    std::swap(ptr_, other.ptr_);
    std::swap(counter_, other.counter_);
  }

  T* get() const { return ptr_; }

  size_t use_count() const {
    return (counter_ != nullptr) ? counter_->strong_count : 0;
  }

  size_t weak_count() const {
    return (counter_ != nullptr) ? counter_->weak_count : 0;
  }

  T& operator*() const { return *ptr_; }

  T* operator->() const { return ptr_; }

  explicit operator bool() const { return static_cast<bool>(ptr_); }
};

template <class T>
class WeakPtr {
  T* ptr_;
  Counter* counter_;

  friend class SharedPtr<T>;

 public:
  WeakPtr() : ptr_(nullptr), counter_(nullptr) {}

  WeakPtr(std::nullptr_t) : ptr_(nullptr), counter_(nullptr) {}

  WeakPtr(const SharedPtr<T>& other) {
    ptr_ = other.ptr_;
    counter_ = other.counter_;
    if (counter_ != nullptr) {
      counter_->weak_count++;
    }
  }

  WeakPtr(const WeakPtr& other) {
    ptr_ = other.ptr_;
    counter_ = other.counter_;
    if (counter_ != nullptr) {
      counter_->weak_count++;
    }
  }

  WeakPtr& operator=(const WeakPtr& other) {
    if (this != &other) {
      reset();
      ptr_ = other.ptr_;
      counter_ = other.counter_;
      if (counter_ != nullptr) {
        counter_->weak_count++;
      }
    }
    return *this;
  }

  WeakPtr(WeakPtr&& other) noexcept : ptr_(nullptr), counter_(nullptr) {
    swap(other);
  }

  WeakPtr& operator=(WeakPtr&& other) noexcept {
    if (this != &other) {
      reset();
      swap(other);
    }
    return *this;
  }

  ~WeakPtr() { reset(); }

  void reset() {
    if (counter_ != nullptr) {
      counter_->weak_count--;
      if (counter_->strong_count == 0 && counter_->weak_count == 0) {
        delete counter_;
      }
    }
    ptr_ = nullptr;
    counter_ = nullptr;
  }

  void swap(WeakPtr& other) {
    std::swap(ptr_, other.ptr_);
    std::swap(counter_, other.counter_);
  }

  size_t use_count() const {
    return (counter_ != nullptr) ? counter_->strong_count : 0;
  }

  size_t weak_count() const {
    return (counter_ != nullptr) ? counter_->weak_count : 0;
  }

  bool expired() const { return use_count() == 0; }

  SharedPtr<T> lock() const {
    return expired() ? SharedPtr<T>() : SharedPtr<T>(*this);
  }
};