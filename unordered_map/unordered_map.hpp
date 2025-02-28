#pragma once

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <vector>

template <class Key, class T, class Hash = std::hash<Key>,
          class Equal = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
class UnorderedMap {
 private:
  struct Bucket;

 public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<const Key, T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using hasher = Hash;
  using key_equal = Equal;
  using allocator_type = Allocator;
  using allocator_traits = std::allocator_traits<allocator_type>;
  using reference = value_type&;
  using const_reference = const value_type&;
  using value_list_type = std::list<value_type, Allocator>;
  using pointer = allocator_traits::pointer;
  using const_pointer = allocator_traits::const_pointer;
  using iterator = value_list_type::iterator;
  using const_iterator = value_list_type::const_iterator;
  using bucket_list_type = std::vector<Bucket>;

  // ANCHOR Constructors
  UnorderedMap() : max_load_factor_(1.0) { reserve(kDefaultBucketCount); }

  UnorderedMap(const UnorderedMap& other)
      : max_load_factor_(other.max_load_factor_) {
    reserve(other.size());
    try {
      for (const auto& elem : other) {
        insert(elem);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  UnorderedMap(UnorderedMap&& other) noexcept
      : max_load_factor_(other.max_load_factor_),
        value_list_(std::move(other.value_list_)),
        bucket_vector_(std::move(other.bucket_vector_)) {
    other.clear();
  }

  // ANCHOR Assignment
  UnorderedMap& operator=(const UnorderedMap& other) {
    if (this != &other) {
      UnorderedMap temp(other);
      swap(temp);
    }
    return *this;
  }

  UnorderedMap& operator=(UnorderedMap&& other) noexcept {
    if (this != &other) {
      clear();

      max_load_factor_ = other.max_load_factor_;
      value_list_ = std::move(other.value_list_);
      bucket_vector_ = std::move(other.bucket_vector_);

      other.clear();
    }
    return *this;
  }

  // ANCHOR Destructor
  ~UnorderedMap() { clear(); }

  // ANCHOR Iterators
  iterator begin() { return value_list_.begin(); }
  const_iterator begin() const { return value_list_.begin(); }
  const_iterator cbegin() const { return value_list_.cbegin(); }
  iterator end() { return value_list_.end(); }
  const_iterator end() const { return value_list_.end(); }
  const_iterator cend() const { return value_list_.cend(); }

  // ANCHOR Capacity
  bool empty() const { return value_list_.empty(); }
  size_type size() const { return value_list_.size(); }

  // ANCHOR Modifiers
  std::pair<iterator, bool> insert(const value_type& value) {
    rehash_if_necessary();

    size_type hash = get_hash(value.first);
    auto& bucket = bucket_vector_[hash];

    size_type it_shift = 0;
    for (auto it = bucket.begin;
         it_shift < bucket.size && it != value_list_.end(); ++it, ++it_shift) {
      if (compare_keys(it->first, value.first)) {
        return {it, false};
      }
    }

    value_list_.push_back(value);

    if (bucket.size == 0) {
      bucket.begin = --value_list_.end();
    }
    ++bucket.size;

    return {--value_list_.end(), true};
  }

  std::pair<iterator, bool> insert(value_type&& value) {
    rehash_if_necessary();

    size_type hash = get_hash(value.first);
    auto& bucket = bucket_vector_[hash];

    size_type it_shift = 0;
    for (auto it = bucket.begin;
         it_shift < bucket.size && it != value_list_.end(); ++it, ++it_shift) {
      if (compare_keys(it->first, value.first)) {
        return {it, false};
      }
    }

    value_list_.emplace_back(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(const_cast<Key&>(value.first))),
        std::forward_as_tuple(std::move(value.second)));

    if (bucket.size == 0) {
      bucket.begin = --value_list_.end();
    }
    ++bucket.size;

    return {--value_list_.end(), true};
  }

  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      insert(*first);
    }
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    rehash_if_necessary();

    allocator_type alloc = Allocator();
    pointer emplaced_ptr = allocator_traits::allocate(alloc, 1);

    try {
      allocator_traits::construct(alloc, emplaced_ptr,
                                  std::forward<Args>(args)...);
      const key_type& key = emplaced_ptr->first;
      size_type hash = get_hash(key);
      auto& bucket = bucket_vector_[hash];

      size_type it_shift = 0;
      for (auto it = bucket.begin;
           it_shift < bucket.size && it != value_list_.end();
           ++it, ++it_shift) {
        if (compare_keys(it->first, key)) {
          delete_emplaced(alloc, emplaced_ptr);
          return {it, false};
        }
      }

      value_list_.emplace_back(
          std::piecewise_construct,
          std::forward_as_tuple(
              std::move(const_cast<Key&>(emplaced_ptr->first))),
          std::forward_as_tuple(std::move(emplaced_ptr->second)));

      delete_emplaced(alloc, emplaced_ptr);

      if (bucket.size == 0) {
        bucket.begin = --value_list_.end();
      }
      ++bucket.size;
      return {--value_list_.end(), true};
    } catch (...) {
      delete_emplaced(alloc, emplaced_ptr);
      throw;
    }
  }

  iterator erase(iterator pos) {
    size_type hash = get_hash(pos->first);
    auto& bucket = bucket_vector_[hash];

    if (bucket.begin == pos) {
      if (bucket.size == 1) {
        bucket.begin = value_list_.end();
      } else {
        bucket.begin = std::next(pos);
      }
    }
    --bucket.size;
    return value_list_.erase(pos);
  }

  iterator erase(const_iterator pos) {
    return erase(value_list_.erase(pos, pos));
  }

  iterator erase(const_iterator first, const_iterator last) {
    while (first != last) {
      first = erase(first);
    }
    return value_list_.erase(first, first);
  }

  // ANCHOR Lookup
  iterator find(const key_type& key) {
    size_type hash = get_hash(key);
    auto& bucket = bucket_vector_[hash];
    size_type it_shift = 0;
    for (auto it = bucket.begin; it_shift < bucket.size; ++it, ++it_shift) {
      if (key_equal{}(key, it->first)) {
        return it;
      }
    }
    return end();
  }

  const_iterator find(const key_type& key) const {
    size_type hash = get_hash(key);
    auto& bucket = bucket_vector_[hash];
    size_type it_shift = 0;
    for (auto it = bucket.begin; it_shift < bucket.size; ++it, ++it_shift) {
      if (key_equal{}(key, it->first)) {
        return it;
      }
    }
    return end();
  }

  T& at(const key_type& key) {
    auto found_it = find(key);
    if (found_it == end()) {
      throw std::out_of_range("Key not found");
    }
    return found_it->second;
  }

  const T& at(const key_type& key) const {
    auto found_it = find(key);
    if (found_it == end()) {
      throw std::out_of_range("Key not found");
    }
    return found_it->second;
  }

  T& operator[](key_type&& key) {
    auto found_it = find(key);
    if (found_it == end()) {
      return insert({std::forward<key_type>(key), T{}}).first->second;
    }
    return found_it->second;
  }

  // ANCHOR Hash policy
  size_type bucket_count() const { return bucket_vector_.size(); }

  float load_factor() const { return size() / bucket_count(); }

  void max_load_factor(float new_ml) {
    max_load_factor_ = new_ml;
    if (load_factor() > max_load_factor_) {
      rehash(std::ceil(size() / max_load_factor_));
    }
  }

  float max_load_factor() const { return max_load_factor_; }

  void rehash(size_type new_bucket_count) {
    new_bucket_count = get_new_bucket_count(new_bucket_count);
    bucket_list_type new_buckets(new_bucket_count, {value_list_.end(), 0});

    for (auto it = value_list_.begin(); it != value_list_.end(); ++it) {
      size_type new_hash = get_hash(it->first, new_bucket_count);
      Bucket& new_bucket = new_buckets[new_hash];

      if (new_bucket.size == 0) {
        new_bucket.begin = it;
      }
      ++new_bucket.size;
    }

    bucket_vector_.swap(new_buckets);
  }

  void reserve(size_type new_size) {
    if (new_size > size()) {
      rehash(std::ceil(new_size / max_load_factor()));
    }
  }

 private:
  static constexpr size_type kDefaultBucketCount{8};
  float max_load_factor_;
  value_list_type value_list_;
  bucket_list_type bucket_vector_;

  struct Bucket {
    size_type size;
    iterator begin;

    Bucket(iterator begin, size_type size) : size(size), begin(begin) {}
  };

  // ANCHOR Private methods
  void swap(UnorderedMap& other) noexcept {
    std::swap(max_load_factor_, other.max_load_factor_);
    value_list_.swap(other.value_list_);
    bucket_vector_.swap(other.bucket_vector_);
  }

  void clear() {
    value_list_.clear();
    bucket_vector_.clear();
    reserve(kDefaultBucketCount);
  }

  size_type get_hash(const key_type& key) const {
    return hasher{}(key) % bucket_vector_.size();
  }

  size_type get_hash(const key_type& key, size_type modulo) const {
    return hasher{}(key) % modulo;
  }

  size_type get_new_bucket_count(size_type new_bucket_count) const {
    return std::max(
        static_cast<size_type>(std::ceil(size() / max_load_factor())),
        new_bucket_count);
  }

  bool compare_keys(const key_type& key1, const key_type& key2) const {
    return key_equal{}(key1, key2);
  }

  void rehash_if_necessary() {
    if (load_factor() > max_load_factor_) {
      rehash(bucket_count() * 2);
    }
  }

  void delete_emplaced(allocator_type& alloc, pointer& emplaced_ptr) {
    allocator_traits::destroy(alloc, emplaced_ptr);
    allocator_traits::deallocate(alloc, emplaced_ptr, 1);
  }
};
