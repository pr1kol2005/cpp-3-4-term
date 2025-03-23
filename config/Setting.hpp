#pragma once

#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <utility>

#include "Configurator.hpp"

// ANCHOR : Exceptions
struct BadSettingAccess : public std::logic_error {
  explicit BadSettingAccess(const std::string& message)
      : std::logic_error("BadSettingAccess: " + message) {}
};

// ANCHOR : SettingHandler
template <typename T>
class SettingHandler {
   public:
  SettingHandler() : stored_(nullptr), call_(nullptr), destroy_(nullptr) {}

  template <typename F>
  SettingHandler(F&& handler) {
    using Decayed_F = typename std::decay<F>::type;
    stored_ = new Decayed_F(std::forward<F>(handler));
    call_ = &CallImpl<Decayed_F>;
    destroy_ = &DestroyImpl<Decayed_F>;
  }

  ~SettingHandler() {
    if (stored_) {
      destroy_(stored_);
    }
  }

  void operator()(const T& value) const {
    if (call_) {
      call_(stored_, value);
    }
  }

   private:
  void* stored_;
  void (*call_)(void*, const T&);
  void (*destroy_)(void*);

  template <typename Decayed_F>
  static void CallImpl(void* stored, const T& value) {
    Decayed_F* handler = static_cast<Decayed_F*>(stored);
    (*handler)(value);
  }

  template <typename Decayed_F>
  static void DestroyImpl(void* stored) {
    delete static_cast<Decayed_F*>(stored);
  }
};

// ANCHOR : Setting
template <typename T, typename ConfiguratorTag>
class Setting {
   public:
  Setting() = delete;

  Setting(const std::string& name) : name_(name) { Register(); }

  template <typename U>
  Setting(const std::string& name, U&& default_value)
      : name_(name), value_(std::forward<U>(default_value)) {
    Register();
  }

  template <typename U>
  Setting(const std::string& name, U&& default_value,
          const std::string& help_text)
      : name_(name),
        help_text_(help_text),
        value_(std::forward<U>(default_value)) {
    Register();
  }

  template <typename U, typename Handler>
  Setting(const std::string& name, U&& default_value,
          const std::string& help_text, Handler&& handler)
      : name_(name),
        help_text_(help_text),
        handler_(std::forward<Handler>(handler)),
        value_(std::forward<U>(default_value)) {
    Register();
  }

  bool HasValue() const { return value_.has_value(); }

  T& GetValue() {
    try {
      return value_.value();
    } catch (...) {
      throw BadSettingAccess("No value");
    }
  }

  const T& GetValue() const {
    try {
      return value_.value();
    } catch (...) {
      throw BadSettingAccess("No value");
    }
  }

   private:
  friend class SettingInterface;
  friend class Configurator<ConfiguratorTag>;

  // создать структуру SettingInterface храняющую имя, указатель на
  // себя, таблицу вызовов, зарегистрироваться через статический метод в
  // конфигураторе — добавить созданный SettingInterface в мапу
  void Register() {
    Configurator<ConfiguratorTag>::GetInstance().RegisterSetting(
        name_, CreateInterface());
  }

  std::unique_ptr<SettingInterface> CreateInterface() {
    auto interface = std::make_unique<SettingInterface>();

    interface->self_ = this;
    interface->destructor_ = [](void*) {};

    interface->GetType_ = &SettingInterface::GetTypeImpl<T>;
    interface->GetValue_ = &SettingInterface::GetValueImpl<T>;
    interface->GetValueAsString_ = &SettingInterface::GetValueAsStringImpl<T>;
    interface->GetHelpText_ = &SettingInterface::GetHelpTextImpl<T>;
    interface->SetValueAsString_ = &SettingInterface::SetValueAsStringImpl<T>;
    interface->Drop_ = &SettingInterface::DropImpl<T>;

    // TODO : other methods

    return interface;
  }

  void SetValue(T&& value) {
    if (handler_) {
      handler_.value()(std::forward<T>(value));
    }
    value_ = std::forward<T>(value);
  }

  void Drop() { value_ = std::nullopt; }

  // TODO : private fields
  std::string name_;
  std::optional<std::string> help_text_ = std::nullopt;
  std::optional<SettingHandler<T>> handler_ = std::nullopt;
  std::optional<T> value_ = std::nullopt;
};
