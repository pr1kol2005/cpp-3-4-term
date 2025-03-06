#pragma once

#include <memory>

template <class T>
class WeakPtr;

template <class T>
class SharedPtr {
 public:
  SharedPtr() : ptr_(nullptr), counter_(nullptr) {}

  SharedPtr(std::nullptr_t) : ptr_(nullptr), counter_(nullptr) {}

  template <class Y>
  explicit SharedPtr(Y* pointer)
      : ptr_(pointer), counter_(new ControlBlockWithPtr<Y>(pointer)) {}

  template <class Y, class Deleter>
  SharedPtr(Y* pointer, Deleter deleter)
      : ptr_(pointer),
        counter_(new ControlBlockWithPtr<Y, Deleter>(pointer, deleter)) {}

  template <class Y, class Deleter, class Allocator>
  SharedPtr(Y* pointer, Deleter deleter, Allocator alloc)
      : ptr_(pointer),
        counter_(ControlBlockWithPtr<Y, Deleter, Allocator>::create(
            pointer, deleter, alloc)) {}

  template <class Y>
  SharedPtr(const SharedPtr<Y>& other) noexcept
      : ptr_(static_cast<T*>(other.ptr_)),
        counter_(reinterpret_cast<ControlBlock*>(other.counter_)) {
    if (counter_) {
      counter_->strong_count++;
    }
  }

  template <class Y>
  SharedPtr& operator=(const SharedPtr<Y>& other) noexcept {
    if (this != reinterpret_cast<const SharedPtr*>(&other)) {
      reset();
      ptr_ = static_cast<T*>(other.ptr_);
      counter_ = reinterpret_cast<ControlBlock*>(other.counter_);
      if (counter_) {
        counter_->strong_count++;
      }
    }
    return *this;
  }

  SharedPtr(const WeakPtr<T>& other) {
    ptr_ = other.ptr_;
    counter_ = other.counter_;
    if (counter_ != nullptr) {
      counter_->strong_count++;
    }
  }

  SharedPtr(const SharedPtr& other)
      : ptr_(other.ptr_), counter_(other.counter_) {
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
      --counter_->strong_count;
      if (counter_->strong_count == 0) {
        counter_->dispose();

        if (counter_->weak_count == 0) {
          counter_->destroy();
        }
      }
    }
    ptr_ = pointer;
    counter_ = pointer ? new ControlBlockWithPtr<T>(pointer) : nullptr;
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

  template <class Y, class... Args>
  friend SharedPtr<Y> MakeShared(Args&&... args);

  template <class Y, class... Args, class Allocator>
  friend SharedPtr<Y> AllocateShared(Allocator alloc, Args&&... args);

 private:
  struct ControlBlock;
  friend class WeakPtr<T>;
  template <class>
  friend class SharedPtr;

  // TODO : inheritance support
  T* ptr_;
  ControlBlock* counter_;

  struct ControlBlock {
    size_t strong_count = 1;
    size_t weak_count = 0;

    virtual void dispose() = 0;
    virtual void destroy() = 0;
    virtual ~ControlBlock() = default;
  };

  template <class Y, class Deleter = std::default_delete<Y>,
            class Allocator = std::allocator<Y>>
  struct ControlBlockWithPtr : public ControlBlock {
    using alloc_traits = std::allocator_traits<Allocator>;
    using cb_alloc_type =
        typename alloc_traits::template rebind_alloc<ControlBlockWithPtr>;
    using cb_alloc_type_traits = std::allocator_traits<cb_alloc_type>;

    Y* ptr;
    Deleter del;
    Allocator alloc;

    ControlBlockWithPtr(Y* ptr, Deleter del = Deleter(),
                        Allocator alloc = Allocator())
        : ptr(ptr), del(del), alloc(alloc) {}

    void dispose() override { del(ptr); }

    static ControlBlockWithPtr* create(Y* pointer, Deleter deleter,
                                       Allocator alloc) {
      cb_alloc_type cb_alloc(alloc);
      ControlBlockWithPtr* cb_ptr = cb_alloc_type_traits::allocate(cb_alloc, 1);
      cb_alloc_type_traits::construct(cb_alloc, cb_ptr, pointer, deleter,
                                      alloc);
      return cb_ptr;
    }

    void destroy() override {
      cb_alloc_type cb_alloc(alloc);
      cb_alloc_type_traits::destroy(cb_alloc, this);
      cb_alloc_type_traits::deallocate(cb_alloc, this, 1);
    }
  };

  template <class Y, class Allocator = std::allocator<Y>>
  struct ControlBlockWithObject : public ControlBlock {
    using alloc_traits = std::allocator_traits<Allocator>;
    using cb_alloc_type =
        typename alloc_traits::template rebind_alloc<ControlBlockWithObject>;
    using cb_alloc_type_traits = std::allocator_traits<cb_alloc_type>;
    Y object;
    Allocator alloc;

    template <class... Args>
    ControlBlockWithObject(Args&&... args)
        : object{std::forward<Args>(args)...} {}

    template <class... Args>
    ControlBlockWithObject(Allocator alloc, Args&&... args)
        : alloc(alloc), object{std::forward<Args>(args)...} {}

    template <class... Args>
    static ControlBlockWithObject* create(Allocator alloc, Args&&... args) {
      cb_alloc_type cb_alloc(alloc);
      ControlBlockWithObject* cb_ptr =
          cb_alloc_type_traits::allocate(cb_alloc, 1);
      cb_alloc_type_traits::construct(cb_alloc, cb_ptr, alloc,
                                      std::forward<Args>(args)...);
      return cb_ptr;
    }

    void dispose() override { alloc_traits::destroy(alloc, &object); }
    void destroy() override {
      cb_alloc_type cb_alloc(alloc);
      cb_alloc_type_traits::deallocate(cb_alloc, this, 1);
    }
  };

  template <class Y, class Allocator = std::allocator<Y>>
  explicit SharedPtr(ControlBlockWithObject<Y, Allocator>* cb_ptr)
      : ptr_(&cb_ptr->object), counter_(cb_ptr) {}
};

template <class T, class... Args>
SharedPtr<T> MakeShared(Args&&... args) {
  auto* cb_ptr = new SharedPtr<T>::template ControlBlockWithObject<T>(
      std::forward<Args>(args)...);
  return SharedPtr<T>(cb_ptr);
}

template <class T, class... Args, class Allocator>
SharedPtr<T> AllocateShared(Allocator alloc, Args&&... args) {
  // TODO implement AllocateShared
  auto* cb_ptr =
      SharedPtr<T>::template ControlBlockWithObject<T, Allocator>::create(
          alloc, std::forward<Args>(args)...);
  return SharedPtr<T>(cb_ptr);
}

template <class T>
class WeakPtr {
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
        counter_->destroy();
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

 private:
  friend class SharedPtr<T>;

  T* ptr_;
  SharedPtr<T>::ControlBlock* counter_;
};