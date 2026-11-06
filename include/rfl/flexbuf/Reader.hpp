#ifndef FLEXBUF_READER_HPP_
#define FLEXBUF_READER_HPP_

#include <flatbuffers/flexbuffers.h>

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <type_traits>

#include "../Bytestring.hpp"
#include "../Ref.hpp"
#include "../Result.hpp"
#include "../Vectorstring.hpp"
#include "../always_false.hpp"
#include "../internal/ptr_cast.hpp"

namespace rfl {
namespace flexbuf {

struct Reader {
  using InputArrayType = flexbuffers::Vector;
  using InputObjectType = flexbuffers::Map;
  using InputVarType = flexbuffers::Reference;

  template <class T>
  static constexpr bool has_custom_constructor =
      (requires(InputVarType var) { T::from_flexbuf(var); });

  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType& _arr) const noexcept {
    if (_idx >= _arr.size()) {
      return error("Index " + std::to_string(_idx) + " of of bounds.");
    }
    return _arr[_idx];
  }

  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    const auto keys = _obj.Keys();
    for (size_t i = 0; i < keys.size(); ++i) {
      if (_name == keys[i].AsString().c_str()) {
        return _obj.Values()[i];
      }
    }
    return error("Map does not contain any element called '" + _name + "'.");
  }

  bool is_empty(const InputVarType& _var) const noexcept {
    return _var.IsNull();
  }

  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      if (!_var.IsString()) {
        return error("Could not cast to a string.");
      }
      return std::string(_var.AsString().c_str());

    } else if constexpr (std::is_same<std::remove_cvref_t<T>,
                                      rfl::Bytestring>() ||
                         std::is_same<std::remove_cvref_t<T>,
                                      rfl::Vectorstring>()) {
      using VectorType = std::remove_cvref_t<T>;
      using ValueType = typename VectorType::value_type;
      if (!_var.IsBlob()) {
        if constexpr (std::is_same<std::remove_cvref_t<T>, rfl::Bytestring>()) {
          return error("Could not cast to bytestring.");
        } else {
          return error("Could not cast to vectorstring.");
        }
      }
      const auto blob = _var.AsBlob();
      const auto data = internal::ptr_cast<const ValueType*>(blob.data());
      return VectorType(data, data + blob.size());

    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      if (!_var.IsBool()) {
        return error("Could not cast to boolean.");
      }
      return _var.AsBool();

    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      if (!_var.IsFloat()) {
        return error("Could not cast to double.");
      }
      return static_cast<T>(_var.AsDouble());

    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      if (!_var.IsIntOrUint()) {
        return error("Could not cast to int.");
      }
      return static_cast<T>(_var.AsInt64());

    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    const auto size = _arr.size();
    if constexpr (requires { _array_reader.reserve(size); }) {
      if (auto err = _array_reader.reserve(size)) {
        return err;
      }
    }
    for (size_t i = 0; i < size; ++i) {
      auto err = _array_reader.read(InputVarType(_arr[i]));
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    const auto keys = _obj.Keys();
    const auto values = _obj.Values();
    const auto num_values = std::min(keys.size(), values.size());

    for (size_t i = 0; i < num_values; ++i) {
      _object_reader.read(std::string_view(keys[i].AsString().c_str()),
                          values[i]);
    }

    return std::nullopt;
  }

  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    if (!_var.IsVector()) {
      return error("Could not cast to Vector.");
    }
    return _var.AsVector();
  }

  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    if (!_var.IsMap()) {
      return error("Could not cast to Map!");
    }
    return _var.AsMap();
  }

  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType& _var) const noexcept {
    try {
      return T::from_flexbuf(_var);
    } catch (std::exception& e) {
      return error(e.what());
    }
  }
};

}  // namespace flexbuf
}  // namespace rfl

#endif
