#pragma once

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <typeindex>

// ANCHOR : Exceptions
struct UnknownParametr : public std::logic_error {
  explicit UnknownParametr(const std::string& message)
      : std::logic_error("UnknownParametr: " + message) {}
};

struct BadConfigValueType : std::logic_error {
  explicit BadConfigValueType(const std::string& message)
      : std::logic_error("BadConfigValueType: " + message) {}
};

struct MainConfiguratorTag {};

template <typename T, typename ConfiguratorTag = MainConfiguratorTag>
class Setting;

// ANCHOR : Traits
template <typename T, typename = void>
struct has_output_operator : std::false_type {};

template <typename T>
struct has_output_operator<T, std::void_t<decltype(std::declval<std::ostream&>()
                                                   << std::declval<T>())> >
    : std::true_type {};

template <typename T>
inline constexpr bool has_output_operator_v = has_output_operator<T>::value;

template <typename T, typename = void>
struct has_input_operator : std::false_type {};

template <typename T>
struct has_input_operator<
    T,
    std::void_t<decltype(std::declval<std::istream&>() >> std::declval<T&>())> >
    : std::true_type {};

template <typename T>
inline constexpr bool has_input_operator_v = has_input_operator<T>::value;

// ANCHOR : SettingInterface
class SettingInterface {
   public:
  void (*destructor_)(void*);
  void* self_;

  std::type_index (*GetType_)();
  void* (*GetValue_)(void*);
  std::string (*GetValueAsString_)(void*);
  std::string (*GetHelpText_)(void*);
  void (*SetValueAsString_)(void*, const std::string&);
  void (*Drop_)(void*);

  template <typename T>
  static std::type_index GetTypeImpl() {
    return typeid(T);
  }

  template <typename T>
  static void* GetValueImpl(void* self) {
    auto* setting = static_cast<Setting<T>*>(self);
    return setting->HasValue() ? &setting->GetValue() : nullptr;
  }

  template <typename T>
  static auto GetValueAsStringImpl(void* self)
      -> std::enable_if_t<has_output_operator_v<T>, std::string> {
    auto* setting = static_cast<Setting<T>*>(self);
    std::stringstream ss;
    if (setting->HasValue()) {
      ss << std::boolalpha << setting->GetValue();
      return ss.str();
    }
    return "";
  }

  template <typename T>
  static auto GetValueAsStringImpl(void*)
      -> std::enable_if_t<!has_output_operator_v<T>, std::string> {
    throw BadConfigValueType("Type doesn't support output operator");
  }

  template <typename T>
  static void SetValueImpl(void* self, T&& value) {
    auto* setting = static_cast<Setting<T>*>(self);
    setting->SetValue(std::forward<T>(value));
  }

  template <typename T>
  static auto SetValueAsStringImpl(void* self, const std::string& value)
      -> std::enable_if_t<has_input_operator_v<T>, void> {
    auto* setting = static_cast<Setting<T>*>(self);
    std::stringstream ss(value);
    T object;
    ss >> std::boolalpha >> object;
    setting->SetValue(std::forward<T>(object));
  }

  template <typename T>
  static auto SetValueAsStringImpl(void* self, const std::string&)
      -> std::enable_if_t<!has_input_operator_v<T>, void> {
    throw BadConfigValueType("Type doesn't support input operator" +
                             static_cast<Setting<T>*>(self)->name_);
  }

  template <typename T>
  static std::string GetHelpTextImpl(void* self) {
    const auto& help = static_cast<Setting<T>*>(self)->help_text_;
    return help.has_value() ? *help : "";
  }

  template <typename T>
  static void DropImpl(void* self) {
    auto* setting = static_cast<Setting<T>*>(self);
    setting->Drop();
  }

  ~SettingInterface() {
    if (destructor_) {
      destructor_(self_);
    }
  }
};

// ANCHOR : Configurator
template <typename Tag = MainConfiguratorTag>
class Configurator {
   private:
  struct SettingInfo {
    std::type_index type_info;
    std::unique_ptr<SettingInterface> interface;

    SettingInfo(std::type_index type_info,
                std::unique_ptr<SettingInterface> interface)
        : type_info(type_info), interface(std::move(interface)) {}
  };

  using string_umap_type = std::unordered_map<std::string, std::string>;
  using settings_umap_type = std::unordered_map<std::string, SettingInfo>;

   public:
  static Configurator& GetInstance() {
    static Configurator instance;
    return instance;
  }

  void RegisterSetting(std::string name,
                       std::unique_ptr<SettingInterface> interface) {
    std::type_index type_info = interface->GetType_();

    auto [it, inserted] =
        settings_map_.try_emplace(name, type_info, std::move(interface));
    if (!inserted) {
      throw std::logic_error("Duplicate parameter registration: " + name);
    }

    auto help = it->second.interface->GetHelpText_(it->second.interface->self_);
    help_map_[name] = help;
  }

  template <typename T>
  std::optional<T> SetValue(const std::string& name, T&& value) {
    auto it = AccessSetting(name);

    auto old_value = GetValue<T>(name);

    SettingInterface::SetValueImpl(it->second.interface->self_,
                                   std::forward<T>(value));

    return old_value;
  }

  template <typename T>
  std::optional<T> GetValue(const std::string& name) const {
    auto it = AccessSetting(name);

    if (it->second.type_info != typeid(T)) {
      throw BadConfigValueType("Type mismatch for parameter: " + name);
    }

    void* value_ptr =
        it->second.interface->GetValue_(it->second.interface->self_);

    if (!value_ptr) {
      return std::nullopt;
    }

    if constexpr (std::is_copy_constructible_v<T>) {
      return *static_cast<T*>(value_ptr);
    } else {
      return std::move(*static_cast<T*>(value_ptr));
    }
  }

  std::string GetValueAsString(const std::string& name) const {
    auto it = AccessSetting(name);

    return it->second.interface->GetValueAsString_(it->second.interface->self_);
  }

  string_umap_type GetHelp() const { return help_map_; }

  const std::string& GetHelp(const std::string& name) const {
    auto it = AccessSetting(name);

    return help_map_.at(name);
  }

  void Init(const std::map<std::string, std::string>& conf_values) {
    for (const auto& [name, value] : conf_values) {
      auto it = AccessSetting(name);

      it->second.interface->SetValueAsString_(it->second.interface->self_,
                                              value);
    }
  }

  void Init(const std::string& filename) {
    std::map<std::string, std::string> conf_values;

    try {
      YAML::Node config = YAML::LoadFile(filename);

      for (const auto& entry : config) {
        std::string key = entry.first.as<std::string>();
        std::string value = entry.second.as<std::string>();
        conf_values[key] = value;
      }
    } catch (const YAML::Exception& e) {
      throw std::runtime_error("Failed to parse YAML file: " +
                               std::string(e.what()));
    }

    Init(conf_values);
  }

  void Drop(const std::string& name) {
    auto it = AccessSetting(name);

    it->second.interface->Drop_(it->second.interface->self_);
  }

   private:
  Configurator() {}
  Configurator(const Configurator&) = delete;
  Configurator& operator=(const Configurator&) = delete;

  auto AccessSetting(const std::string& name) const {
    auto it = settings_map_.find(name);
    if (it == settings_map_.end()) {
      throw UnknownParametr("Parameter not found: " + name);
    }
    return it;
  }

  string_umap_type help_map_;
  settings_umap_type settings_map_;
};
