#ifndef RFL_YAML_READER_HPP_
#define RFL_YAML_READER_HPP_

#include <yaml-cpp/yaml.h>

#include <array>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../Result.hpp"
#include "../always_false.hpp"

namespace rfl {
namespace yaml {

struct Reader {
  struct YAMLInputArray {
    /// @brief Constructs a YAMLInputArray from a YAML::Node.
    /// @param _node The YAML node representing a sequence.
    YAMLInputArray(const YAML::Node& _node) : node_(_node) {}
    YAML::Node node_;
  };

  struct YAMLInputObject {
    /// @brief Constructs a YAMLInputObject from a YAML::Node.
    /// @param _node The YAML node representing a mapping.
    YAMLInputObject(const YAML::Node& _node) : node_(_node) {}
    YAML::Node node_;
  };

  struct YAMLInputVar {
    /// @brief Constructs a YAMLInputVar from a YAML::Node.
    /// @param _node The YAML node representing a generic value.
    YAMLInputVar(const YAML::Node& _node) : node_(_node) {}
    YAML::Node node_;
  };

  using InputArrayType = YAMLInputArray;
  using InputObjectType = YAMLInputObject;
  using InputVarType = YAMLInputVar;

  /// @brief Trait to check if a type T has a from_json_obj method.
  /// @tparam T The type to check.
  template <class T, class = void>
  struct has_from_json_obj : std::false_type {};

  /// @brief Compile-time check for custom constructor from YAML object.
  /// @tparam T The type to check.
  template <class T>
  static constexpr bool has_custom_constructor =
      (requires(InputVarType var) { T::from_yaml_obj(var); });

  /// @brief Constructs a Reader from a YAML string view.
  /// @param _yaml_str The YAML document as a string view.
  Reader(const std::string_view& _yaml_str) noexcept : yaml_str_(_yaml_str) {}

  /// @brief Retrieves an element from a YAML sequence by index.
  /// @param _idx The index of the element in the array.
  /// @param _arr The YAMLInputArray representing the sequence.
  /// @return Result containing the InputVarType or an error if out of bounds.
  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType& _arr) const noexcept {
    if (_idx >= _arr.node_.size()) {
      return error("Index " + std::to_string(_idx) + " of of bounds.");
    }
    return InputVarType(_arr.node_[_idx]);
  }

  /// @brief Retrieves a field from a YAML mapping by name.
  /// @param _name The name of the field.
  /// @param _obj The YAMLInputObject representing the mapping.
  /// @return Result containing the InputVarType or an error if the field does
  /// not exist.
  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    auto var = InputVarType(_obj.node_[_name]);
    if (!var.node_) {
      return error("Object contains no field named '" + _name + "'.");
    }
    return var;
  }

  /// @brief Checks if a YAMLInputVar is empty or null.
  /// @param _var The variable to check.
  /// @return True if the node is empty or null, false otherwise.
  bool is_empty(const InputVarType& _var) const noexcept {
    return !_var.node_ || _var.node_.IsNull();
  }

  /// @brief Converts a YAMLInputVar to a basic C++ type (string, bool, float,
  /// or int).
  /// @tparam T The target type.
  /// @param _var The YAMLInputVar to convert.
  /// @return Result containing the converted value or an error if conversion
  /// fails.
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    try {
      if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>() ||
                    std::is_same<std::remove_cvref_t<T>, bool>() ||
                    std::is_floating_point<std::remove_cvref_t<T>>()) {
        auto result = _var.node_.as<std::remove_cvref_t<T>>();
        if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
          // In case of multi-line YAML literal strings, yaml-cpp may parse an
          // extra new line which is not there intentionally. This may break
          // multiple re-serialization checks, for this reason we trim trailing
          // new-lines here.
          //
          // This is only done for literal blocks which doesn't have tags or
          // anchors
          if (_var.node_.Tag() == "!" &&
              yaml_str_[_var.node_.Mark().pos] == '|') {
            auto last_non_new_line = result.find_last_not_of("\r\n");
            if (last_non_new_line != std::string::npos) {
              result = result.substr(0, last_non_new_line + 1);
            }
          }
        }
        return result;

      } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
        return static_cast<T>(_var.node_.as<std::remove_cvref_t<int64_t>>());

      } else {
        static_assert(rfl::always_false_v<T>, "Unsupported type.");
      }

    } catch (std::exception& e) {
      return error(e.what());
    }
  }

  /// @brief Converts a YAMLInputVar to a YAMLInputArray if it is a sequence.
  /// @param _var The variable to convert.
  /// @return Result containing the InputArrayType or an error if not a
  /// sequence.
  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    if (!_var.node_.IsSequence()) {
      return error("Could not cast to sequence!");
    }
    return InputArrayType(_var.node_);
  }

  /// @brief Reads each element of a YAML sequence using a provided ArrayReader.
  /// @tparam ArrayReader The type of the array reader functor.
  /// @param _array_reader The functor to process each element.
  /// @param _arr The YAMLInputArray to read from.
  /// @return Optional error if any element fails to be read, otherwise nullopt.
  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    if constexpr (requires { _array_reader.reserve(_arr.node_.size()); }) {
      if (auto err = _array_reader.reserve(_arr.node_.size())) {
        return err;
      }
    }
    for (size_t i = 0; i < _arr.node_.size(); ++i) {
      auto err = _array_reader.read(_arr.node_[i]);
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  /// @brief Reads each key-value pair of a YAML mapping using a provided
  /// ObjectReader.
  /// @tparam ObjectReader The type of the object reader functor.
  /// @param _object_reader The functor to process each field.
  /// @param _obj The YAMLInputObject to read from.
  /// @return Optional error if any field fails to be read, otherwise nullopt.
  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    for (const auto& p : _obj.node_) {
      try {
        const auto k = p.first.as<std::string>();
        _object_reader.read(std::string_view(k), InputVarType(p.second));
      } catch (std::exception& e) {
        continue;
      }
    }
    return std::nullopt;
  }

  /// @brief Converts a YAMLInputVar to a YAMLInputObject if it is a mapping.
  /// @param _var The variable to convert.
  /// @return Result containing the InputObjectType or an error if not a
  /// mapping.
  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    if (!_var.node_.IsMap()) {
      return error("Could not cast to map!");
    }
    return InputObjectType(_var.node_);
  }

  /// @brief Uses a custom constructor to convert a YAMLInputVar to type T.
  /// @tparam T The target type with a static from_yaml_obj method.
  /// @param _var The YAMLInputVar to convert.
  /// @return Result containing the constructed T or an error if construction
  /// fails.
  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType _var) const noexcept {
    try {
      return T::from_yaml_obj(_var);
    } catch (std::exception& e) {
      return error(e.what());
    }
  }

 private:
  /// @brief The YAML document as a string view, used for context and error
  /// reporting.
  std::string_view yaml_str_;
};

}  // namespace yaml
}  // namespace rfl

#endif
