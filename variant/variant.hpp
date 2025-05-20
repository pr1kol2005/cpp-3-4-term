#pragma once

#include <exception>
#include <initializer_list>
#include <memory>
#include <utility>
#include <concepts>

// ANCHOR - Variant forward declaration
template <typename... Types>
class Variant;

struct BadVariantAccess : public std::exception {};

// ANCHOR - details
namespace details {

// ANCHOR - VariadicUnion
template <typename... Types>
union VariadicUnion;

template <typename Head, typename... Tail>
union VariadicUnion<Head, Tail...> {
  VariadicUnion() {}
  ~VariadicUnion() {}

  template <typename T>
  T& get() & {
    if constexpr (std::is_same_v<Head, T>) {
      return value;
    } else {
      return tail.template get<T>();
    }
  }

  template <typename T>
  const T& get() const& {
    if constexpr (std::same_as<Head, T>) {
      return value;
    } else {
      return tail.template get<T>();
    }
  }

  template <typename T>
  T&& get() && {
    if constexpr (std::same_as<Head, T>) {
      return std::move(value);
    } else {
      return std::move(tail).template get<T>();
    }
  }

  template <typename T, typename... Args>
  T& emplace_construct(Args&&... args) {
    if constexpr (std::same_as<Head, T>) {
      return *std::construct_at(std::addressof(value),
                                std::forward<Args>(args)...);
    } else {
      return tail.template emplace_construct<T>(std::forward<Args>(args)...);
    }
  }

  void destroy(std::size_t active_index) {
    if (active_index == 0) {
      std::destroy_at(std::addressof(value));
    } else {
      tail.destroy(active_index - 1);
    }
  }

  Head value;
  VariadicUnion<Tail...> tail;
};

template <>
union VariadicUnion<> {
  void destroy(std::size_t /*unused*/) {}
};

}  // namespace details

// ANCHOR - traits
namespace traits {

template <typename T>
struct Length;

template <typename... Types>
struct Length<Variant<Types...>> {
  constexpr static std::size_t kValue = sizeof...(Types);
};

template <typename T, typename Var>
struct Index;

template <typename T, typename Head, typename... Tail>
struct Index<T, Variant<Head, Tail...>> {
  constexpr static std::size_t kValue = 1 + Index<T, Variant<Tail...>>::kValue;
};

template <typename Head, typename... Tail>
struct Index<Head, Variant<Head, Tail...>> {
  constexpr static std::size_t kValue = 0;
};

template <std::size_t I, typename Var>
struct Type;

template <std::size_t I, typename Head, typename... Tail>
struct Type<I, Variant<Head, Tail...>> {
  using type = typename Type<I - 1, Variant<Tail...>>::type;
};

template <typename Head, typename... Tail>
struct Type<0, Variant<Head, Tail...>> {
  using type = Head;
};

}  // namespace traits

template <typename Var>
struct VariantTraits {
  constexpr static std::size_t kLength = traits::Length<Var>::kValue;
  template <typename T>
  constexpr static std::size_t kIndex = traits::Index<T, Var>::kValue;

  template <std::size_t I>
  using type = typename traits::Type<I, Var>::type;
};

// ANCHOR - VariantFieldBase
template <typename... Types>
struct VariantFieldBase {
  details::VariadicUnion<Types...> data;
  std::size_t active_index = 0;
};

// ANCHOR - VariantBase (VariantAlternative)
template <typename T, typename Derived>
struct VariantBase {
  VariantBase() {}

  VariantBase(std::decay_t<T>&& value) {
    auto* self = static_cast<Derived*>(this);
    self->data.template emplace_construct<T>(std::move(value));
    self->active_index = VariantTraits<Derived>::template kIndex<T>;
  }

  template <typename U>
    requires std::constructible_from<T, U&&>
  VariantBase(U&& value) {
    auto* self = static_cast<Derived*>(this);
    self->data.template emplace_construct<T>(std::forward<U>(value));
    self->active_index = VariantTraits<Derived>::template kIndex<T>;
  }

  Derived& operator=(const T& value) {
    auto* self = static_cast<Derived*>(this);
    constexpr std::size_t kNewIndex =
        VariantTraits<Derived>::template kIndex<T>;
    if (self->active_index != kNewIndex) {
      self->data.destroy(self->active_index);
    }
    self->data.template emplace_construct<T>(value);
    self->active_index = kNewIndex;
    return *self;
  }

  Derived& operator=(T&& value) {
    auto* self = static_cast<Derived*>(this);
    constexpr std::size_t kNewIndex =
        VariantTraits<Derived>::template kIndex<T>;
    if (self->active_index != kNewIndex) {
      self->data.destroy(self->active_index);
    }
    self->data.template emplace_construct<T>(std::move(value));
    self->active_index = kNewIndex;
    return *self;
  }

  template <typename U>
    requires std::is_assignable_v<T, U&&>
  Derived& operator=(U&& value) {
    auto* self = static_cast<Derived*>(this);
    constexpr std::size_t kNewIndex =
        VariantTraits<Derived>::template kIndex<T>;
    if (self->active_index != kNewIndex) {
      self->data.destroy(self->active_index);
    }
    self->data.template emplace_construct<T>(std::forward<U>(value));
    self->active_index = kNewIndex;
    return *self;
  }
};

// ANCHOR - Variant
template <typename... Types>
class Variant : private VariantFieldBase<Types...>,
                private VariantBase<Types, Variant<Types...>>... {
  template <typename T, typename Derived>
  friend struct VariantBase;

  template <typename... Ts>
  friend struct VariantFieldBase;

  // ANCHOR - copy & move helpers
  template <std::size_t I>
  void copy_helper(const Variant& other) {
    if constexpr (I < VariantTraits<Variant>::kLength) {
      if (other.active_index == I) {
        using type = typename VariantTraits<Variant>::template type<I>;
        data.template emplace_construct<type>(other.data.template get<type>());
      } else {
        copy_helper<I + 1>(other);
      }
    }
  }

  template <std::size_t I>
  void move_helper(Variant&& other) {
    if constexpr (I < VariantTraits<Variant>::kLength) {
      if (other.active_index == I) {
        using type = typename VariantTraits<Variant>::template type<I>;
        data.template emplace_construct<type>(
            std::move(other.data.template get<type>()));
      } else {
        move_helper<I + 1>(std::move(other));
      }
    }
  }

   public:
  using VariantFieldBase<Types...>::data;
  using VariantFieldBase<Types...>::active_index;
  using VariantBase<Types, Variant<Types...>>::VariantBase...;
  using VariantBase<Types, Variant>::operator=...;

  Variant()
      : VariantFieldBase<Types...>(),
        VariantBase<Types, Variant<Types...>>()... {
    using first_type = typename VariantTraits<Variant>::template type<0>;
    data.template emplace_construct<first_type>();
    active_index = 0;
  }

  Variant(const Variant& other)
      : VariantFieldBase<Types...>(),
        VariantBase<Types, Variant<Types...>>()... {
    active_index = other.active_index;
    copy_helper<0>(other);
  }

  Variant(Variant&& other) noexcept {
    active_index = other.active_index;
    move_helper<0>(std::move(other));
  }

  Variant& operator=(const Variant& other) {
    if (&other != this) {
      data.destroy(active_index);
      active_index = other.active_index;
      copy_helper<0>(other);
    }
    return *this;
  }

  Variant& operator=(Variant&& other) noexcept {
    if (&other != this) {
      data.destroy(active_index);
      active_index = other.active_index;
      move_helper<0>(std::move(other));
    }
    return *this;
  }

  ~Variant() { data.destroy(active_index); }

  // ANCHOR - emplace
  template <typename U, typename... Args>
    requires std::constructible_from<U, Args...>
  U& emplace(Args&&... args) {
    constexpr std::size_t kNewIndex =
        VariantTraits<Variant>::template kIndex<U>;
    if (active_index != kNewIndex) {
      data.destroy(active_index);
    }
    data.template emplace_construct<U>(std::forward<Args>(args)...);
    active_index = kNewIndex;
    return data.template get<U>();
  }

  template <typename U, typename Elem = typename std::decay_t<U>::value_type>
  U& emplace(std::initializer_list<Elem> init_list) {
    constexpr std::size_t kNewIndex =
        VariantTraits<Variant>::template kIndex<U>;
    if (active_index != kNewIndex) {
      data.destroy(active_index);
    }
    data.template emplace_construct<U>(init_list);
    active_index = kNewIndex;
    return data.template get<U>();
  }

  template <std::size_t I, typename... Args>
  decltype(auto) emplace(Args&&... args) {
    using ith_type =
        typename VariantTraits<Variant<Types...>>::template type<I>;
    if (active_index != I) {
      data.destroy(active_index);
    }
    data.template emplace_construct<ith_type>(std::forward<Args>(args)...);
    active_index = I;
    return data.template get<ith_type>();
  }
};

// ANCHOR - get<T>
template <typename T, typename... Types>
decltype(auto) get(Variant<Types...>& var) {
  if (var.active_index !=
      VariantTraits<Variant<Types...>>::template kIndex<T>) {
    throw BadVariantAccess{};
  }
  return var.data.template get<T>();
}

template <typename T, typename... Types>
decltype(auto) get(const Variant<Types...>& var) {
  if (var.active_index !=
      VariantTraits<Variant<Types...>>::template kIndex<T>) {
    throw BadVariantAccess{};
  }
  return var.data.template get<T>();
}

template <typename T, typename... Types>
decltype(auto) get(Variant<Types...>&& var) {
  if (std::move(var).active_index !=
      VariantTraits<Variant<Types...>>::template kIndex<T>) {
    throw BadVariantAccess{};
  }
  return std::move(var).data.template get<T>();
}

// ANCHOR - get<I>
template <std::size_t I, typename Var>
decltype(auto) get(Var&& var) {
  using ith_type = typename VariantTraits<std::decay_t<Var>>::template type<I>;
  return get<ith_type>(std::forward<Var>(var));
}

// ANCHOR - holds_alternative
template <typename T, typename... Types>
bool holds_alternative(const Variant<Types...>& var) {
  return (var.active_index ==
          VariantTraits<Variant<Types...>>::template kIndex<T>);
}

// ANCHOR - visit
namespace details {
template <typename Visitor, typename Var, std::size_t... I>
decltype(auto) visit_impl(Visitor&& vis, Var&& var,
                          std::index_sequence<I...> /*unused*/) {
  using var_type = std::decay_t<Var>;

  using res_type = decltype(std::forward<Visitor>(vis)(
      get<typename VariantTraits<var_type>::template type<0>>(
          std::forward<Var>(var))));

  using func = res_type (*)(Visitor&&, Var&&);

  static constexpr func kTable[] = {
      +[](Visitor&& vis_nested, Var&& var_nested) -> res_type {
        return std::forward<Visitor>(vis_nested)(
            get<typename VariantTraits<var_type>::template type<I>>(
                std::forward<Var>(var_nested)));
      }...};

  return kTable[var.active_index](std::forward<Visitor>(vis),
                                  std::forward<Var>(var));
}
}  // namespace details

template <typename Visitor, typename Var>
decltype(auto) visit(Visitor&& vis, Var&& var) {
  return details::visit_impl(
      std::forward<Visitor>(vis), std::forward<Var>(var),
      std::make_index_sequence<VariantTraits<std::decay_t<Var>>::kLength>{});
}

template <typename Visitor, typename Var, typename... Vars>
decltype(auto) visit(Visitor&& vis, Var&& var, Vars&&... vars) {
  return visit(
      [&](auto&& var_val) -> decltype(auto) {
        return visit(
            [&](auto&&... rest_vals) -> decltype(auto) {
              return std::forward<Visitor>(vis)(
                  std::forward<decltype(var_val)>(var_val),
                  std::forward<decltype(rest_vals)>(rest_vals)...);
            }, std::forward<Vars>(vars)...);
      },
      std::forward<Var>(var));
}
