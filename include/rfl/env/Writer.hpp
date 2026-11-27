#ifndef RFL_ENV_WRITER_HPP_
#define RFL_ENV_WRITER_HPP_

#include <cstdlib>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "../Ref.hpp"
#include "../Variant.hpp"
#include "../always_false.hpp"
#include "../common.hpp"

namespace rfl::env {

struct OutputEnvVarType {
  std::string value;
};

struct OutputEnvObjectType;

struct OutputEnvArrayType {
  std::string prefix;
  Ref<std::vector<
      Variant<OutputEnvVarType, OutputEnvArrayType, OutputEnvObjectType>>>
      values;
};

struct OutputEnvObjectType {
  std::string prefix;
  Ref<std::map<std::string, Variant<OutputEnvVarType, OutputEnvArrayType,
                                    OutputEnvObjectType>>>
      fields;
};

/// A Writer class for serializing data into  ENV format. It provides methods to
/// create arrays, objects, and values, and to add nested elements.
class RFL_API Writer {
 public:
  using OutputArrayType = OutputEnvArrayType;
  using OutputObjectType = OutputEnvObjectType;
  using OutputVarType = OutputEnvVarType;

  Writer();

  OutputArrayType array_as_root(const size_t) const noexcept;

  OutputObjectType object_as_root(const size_t) const noexcept;

  template <class T = int>
  OutputVarType null_as_root() const noexcept {
    static_assert(always_false_v<T>,
                  "Writing null to ENV as root variables is not supported.");
    return OutputVarType{};
  }

  template <class T>
  OutputVarType value_as_root(const T&) const noexcept {
    static_assert(
        always_false_v<T>,
        "Writing basic types to ENV as root variables is not supported.");
    return OutputVarType{};
  }

  /// Adds a nested array to a parent array.
  /// @param The expected size (unused, reserved for future optimization)
  /// @param _parent Pointer to the parent array to add to
  /// @return An output array that can be populated with elements
  OutputArrayType add_array_to_array(const size_t,
                                     OutputArrayType* _parent) const;

  /// Adds a nested array to a parent object with the specified field name.
  /// @param _name The name of the field in the parent object
  /// @param The expected size (unused, reserved for future optimization)
  /// @param _parent Pointer to the parent object to add to
  /// @return An output array that can be populated with elements
  OutputArrayType add_array_to_object(const std::string_view& _name,
                                      const size_t,
                                      OutputObjectType* _parent) const;

  /// Adds a nested object to a parent array.
  /// @param The expected size (unused, reserved for future optimization)
  /// @param _parent Pointer to the parent array to add to
  /// @return An output object that can be populated with key-value pairs
  OutputObjectType add_object_to_array(const size_t,
                                       OutputArrayType* _parent) const;

  /// Adds a nested object to a parent object with the specified field name.
  /// @param _name The name of the field in the parent object
  /// @param The expected size (unused, reserved for future optimization)
  /// @param _parent Pointer to the parent object to add to
  /// @return An output object that can be populated with key-value pairs
  OutputObjectType add_object_to_object(const std::string_view& _name,
                                        const size_t,
                                        OutputObjectType* _parent) const;

  /// Adds a value to a parent array.
  /// Supports basic types like strings, numbers, and booleans.
  /// @tparam T The type of the value to add
  /// @param _var The value to add to the array
  /// @param _parent Pointer to the parent array to add to
  /// @return An output variable representing the added value
  /// @throws std::runtime_error if the value cannot be added
  template <class T>
  OutputVarType add_value_to_array(const T& _var,
                                   OutputArrayType* _parent) const {
    const auto val = from_basic_type(_var);
    _parent->values->emplace_back(val);
    return val;
  }

  /// Adds a value to a parent object with the specified field name.
  /// Supports basic types like strings, numbers, and booleans.
  /// @tparam T The type of the value to add
  /// @param _name The name of the field in the parent object
  /// @param _var The value to add to the object
  /// @param _parent Pointer to the parent object to add to
  /// @return An output variable representing the added value
  /// @throws std::runtime_error if the field cannot be added
  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name,
                                    const T& _var,
                                    OutputObjectType* _parent) const {
    const auto val = from_basic_type(_var);
    _parent->fields->emplace(std::string(_name), val);
    return val;
  }

  /// Adds a null value to a parent array.
  /// @param _parent Pointer to the parent array to add to
  /// @return An output variable representing the null value
  OutputVarType add_null_to_array(OutputArrayType* _parent) const;

  /// Adds a null value to a parent object with the specified field name.
  /// @param _name The name of the field in the parent object
  /// @param _parent Pointer to the parent object to add to
  /// @return An output variable representing the null value
  OutputVarType add_null_to_object(const std::string_view& _name,
                                   OutputObjectType* _parent) const;

  /// Finalizes a JSON array after all elements have been added.
  /// This is a no-op for yyjson as arrays don't need explicit finalization.
  /// @param Pointer to the array (unused)
  void end_array(OutputArrayType*) const noexcept;

  /// Finalizes a JSON object after all fields have been added.
  /// This is a no-op for yyjson as objects don't need explicit finalization.
  /// @param Pointer to the object (unused)
  void end_object(OutputObjectType*) const noexcept;

 private:
  template <class T>
  OutputVarType from_basic_type(const T& _var) const {
    using U = std::remove_cvref_t<T>;

    if constexpr (std::same_as<U, std::string>) {
      return OutputVarType{.value = _var};

    } else if constexpr (std::same_as<U, bool>) {
      return OutputVarType{.value = _var ? "true" : "false"};

    } else if constexpr (std::is_integral_v<U>) {
      return OutputVarType{.value = std::to_string(_var)};

    } else if constexpr (std::is_floating_point_v<U>) {
      return OutputVarType{.value = std::to_string(_var)};

    } else {
      static_assert(always_false_v<T>,
                    "Unsupported type for ENV serialization.");
    }
  }
};

}  // namespace rfl::env

#endif
