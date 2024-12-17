#pragma once

#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>

template <typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  struct Node;

  void clear_nodes() {
    while (head_ != nullptr) {
      Node* temp = head_;
      head_ = head_->next;
      node_allocator_traits::destroy(node_allocator_, temp);
      node_allocator_traits::deallocate(node_allocator_, temp, 1);
    }
    tail_ = nullptr;
    size_ = 0;
  }

 public:
  template <typename U>
  class ListIterator;

  using value_type = T;
  using allocator_type = Allocator;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = std::allocator_traits<Allocator>::pointer;
  using const_pointer = const std::allocator_traits<Allocator>::pointer;
  using iterator = ListIterator<T>;
  using const_iterator = ListIterator<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // TODO : Constructors
  List() = default;

  explicit List(size_t count, const Allocator& alloc = Allocator())
      : node_allocator_(alloc) {
    try {
      for (size_t i = 0; i < count; ++i) {
        emplace_back();
      }
    } catch (...) {
      clear_nodes();
      throw;
    }
  }

  List(size_t count, const T& value, const Allocator& alloc = Allocator())
      : node_allocator_(alloc) {
    try {
      for (size_t i = 0; i < count; ++i) {
        push_back(value);
      }
    } catch (...) {
      clear_nodes();
      throw;
    }
  }

  List(std::initializer_list<T> init, const Allocator& alloc = Allocator())
      : node_allocator_(alloc) {
    try {
      for (const T& value : init) {
        push_back(value);
      }
    } catch (...) {
      clear_nodes();
      throw;
    }
  }

  ~List() { clear_nodes(); }

  // TODO : Assignment
  List(const List& other)
      : node_allocator_(
            node_allocator_traits::select_on_container_copy_construction(
                other.node_allocator_)) {
    try {
      for (const T& value : other) {
        push_back(value);
      }
    } catch (...) {
      clear_nodes();
      throw;
    }
  }

  List& operator=(const List& other) {
    if (this == &other) {
      return *this;
    }

    List temp(0, node_allocator_);

    for (const T& value : other) {
      temp.push_back(value);
    }

    std::swap(head_, temp.head_);
    std::swap(tail_, temp.tail_);
    std::swap(size_, temp.size_);

    if (node_allocator_traits::propagate_on_container_copy_assignment::value) {
      node_allocator_ = other.node_allocator_;
    } else if (node_allocator_ == other.node_allocator_) {
      node_allocator_ = other.node_allocator_;
    }

    return *this;
  }

  allocator_type get_allocator() const { return node_allocator_; }

  // TODO : Iterators
  iterator begin() { return iterator(head_, this); }
  const_iterator begin() const { return const_iterator(head_, this); }
  iterator end() { return iterator(nullptr, this); }
  const_iterator end() const { return const_iterator(nullptr, this); }
  const_iterator cbegin() const { return const_iterator(head_, this); }
  const_iterator cend() const { return const_iterator(nullptr, this); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  // TODO : Element access
  reference front() {
    if (head_ == nullptr) {
      throw std::runtime_error("List is empty");
    }
    return head_->value;
  }

  const_reference front() const {
    if (head_ == nullptr) {
      throw std::runtime_error("List is empty");
    }
    return head_->value;
  }

  reference back() {
    if (tail_ == nullptr) {
      throw std::runtime_error("List is empty");
    }
    return tail_->value;
  }

  const_reference back() const {
    if (tail_ == nullptr) {
      throw std::runtime_error("List is empty");
    }
    return tail_->value;
  }

  bool empty() const { return size_ == 0; }

  size_type size() const { return size_; }

  // TODO : Modifiers
  template <typename U>
  void push_back(U&& value) {
    Node* new_node = node_allocator_traits::allocate(node_allocator_, 1);
    try {
      node_allocator_traits::construct(node_allocator_, new_node,
                                       std::forward<U>(value));
      if (tail_) {
        tail_->next = new_node;
        new_node->prev = tail_;
        tail_ = new_node;
      } else {
        head_ = tail_ = new_node;
      }
      ++size_;
    } catch (...) {
      node_allocator_traits::deallocate(node_allocator_, new_node, 1);
      throw;
    }
  }

  void emplace_back() {
    Node* new_node = node_allocator_traits::allocate(node_allocator_, 1);
    try {
      node_allocator_traits::construct(node_allocator_, new_node);

      if (tail_) {
        tail_->next = new_node;
        new_node->prev = tail_;
        tail_ = new_node;
      } else {
        head_ = tail_ = new_node;
      }
      ++size_;
    } catch (...) {
      node_allocator_traits::deallocate(node_allocator_, new_node, 1);
      throw;
    }
  }

  template <typename U>
  void push_front(U&& value) {
    Node* new_node = node_allocator_traits::allocate(node_allocator_, 1);
    try {
      node_allocator_traits::construct(node_allocator_, new_node,
                                       std::forward<U>(value));

      if (head_) {
        head_->prev = new_node;
        new_node->next = head_;
        head_ = new_node;
      } else {
        head_ = tail_ = new_node;
      }
      ++size_;
    } catch (...) {
      node_allocator_traits::deallocate(node_allocator_, new_node, 1);
      throw;
    }
  }

  void pop_back() {
    if (tail_ == nullptr) {
      return;
    }
    Node* temp = tail_;
    tail_ = tail_->prev;

    if (tail_) {
      tail_->next = nullptr;
    } else {
      head_ = nullptr;
    }

    node_allocator_traits::destroy(node_allocator_, temp);
    node_allocator_traits::deallocate(node_allocator_, temp, 1);
    --size_;
  }

  void pop_front() {
    if (head_ == nullptr) {
      return;
    }
    Node* temp = head_;
    head_ = head_->next;

    if (head_) {
      head_->prev = nullptr;
    } else {
      tail_ = nullptr;
    }

    node_allocator_traits::destroy(node_allocator_, temp);
    node_allocator_traits::deallocate(node_allocator_, temp, 1);
    --size_;
  }

 private:
  // TODO : Node class
  struct Node {
    T value;
    Node* prev = nullptr;
    Node* next = nullptr;

    Node() = default;

    template <typename U>
    Node(U&& value) : value(std::forward<U>(value)) {}
  };

  using node_allocator =
      std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using node_allocator_traits = std::allocator_traits<node_allocator>;

  Node* head_ = nullptr;
  Node* tail_ = nullptr;
  size_t size_ = 0;
  node_allocator node_allocator_;
};

// TODO : Iterator class
template <typename T, typename Allocator>
template <typename U>
class List<T, Allocator>::ListIterator {
 private:
  Node* node_;
  const List<T, Allocator>* list_;

 public:
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = U;
  using difference_type = std::ptrdiff_t;
  using pointer = U*;
  using const_pointer = const U*;
  using reference = U&;
  using const_reference = const U&;

  ListIterator(Node* node, const List<T, Allocator>* list)
      : node_(node), list_(list) {}

  reference operator*() {
    if (node_ == nullptr) {
      throw std::out_of_range("Dereferencing past-the-last element");
    }
    return node_->value;
  }

  const_reference operator*() const {
    if (node_ == nullptr) {
      throw std::out_of_range("Dereferencing past-the-last element");
    }
    return node_->value;
  }

  pointer operator->() {
    if (node_ == nullptr) {
      throw std::out_of_range("Accessing past-the-last element");
    }
    return &(node_->value);
  }

  const_pointer operator->() const {
    if (node_ == nullptr) {
      throw std::out_of_range("Accessing past-the-last element");
    }
    return &(node_->value);
  }

  ListIterator& operator++() {
    if (node_) {
      node_ = node_->next;
    }
    return *this;
  }

  ListIterator operator++(int) {
    ListIterator temp = *this;
    ++(*this);
    return temp;
  }

  ListIterator& operator--() {
    if (node_) {
      node_ = node_->prev;
    } else {
      node_ = list_->tail_;
    }
    return *this;
  }

  ListIterator operator--(int) {
    ListIterator temp = *this;
    --(*this);
    return temp;
  }

  bool operator==(const ListIterator& other) const {
    return node_ == other.node_;
  }

  bool operator!=(const ListIterator& other) const {
    return node_ != other.node_;
  }
};
