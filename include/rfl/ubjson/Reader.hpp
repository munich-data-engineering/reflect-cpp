#ifndef RFL_UBJSON_READER_HPP_
#define RFL_UBJSON_READER_HPP_

#include <cstddef>
#include <exception>
// Silence a -Wmaybe-uninitialized false positive in jsoncons
// (utility/bigint.hpp).
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#endif
#include <jsoncons/json.hpp>
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
#endif
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "../Bytestring.hpp"
#include "../Result.hpp"
#include "../Vectorstring.hpp"
#include "../always_false.hpp"
#include "../internal/ptr_cast.hpp"

namespace rfl::ubjson {

class Reader {
 public:
  struct UBJSONInputArray {
    jsoncons::json* val_;
  };

  struct UBJSONInputObject {
    jsoncons::json* val_;
  };

  struct UBJSONInputVar {
    jsoncons::json* val_;
  };

  using InputArrayType = UBJSONInputArray;
  using InputObjectType = UBJSONInputObject;
  using InputVarType = UBJSONInputVar;

  Reader() {}

  ~Reader() = default;

  template <class T>
  static constexpr bool has_custom_constructor =
      (requires(InputVarType var) { T::from_ubjson_obj(var); });

  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType& _arr) const noexcept {
    if (_idx >= _arr.val_->size()) {
      return error("Index out of range.");
    }
    return InputVarType{&_arr.val_->at(_idx)};
  }

  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType& _obj) const noexcept {
    for (auto& kv : _obj.val_->object_range()) {
      if (kv.key() == _name) {
        return InputVarType{&kv.value()};
      };
    }
    return error("Field name '" + _name + "' not found.");
  }

  bool is_empty(const InputVarType& _var) const noexcept {
    return _var.val_->is_null();
  }

  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {
    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      if (!_var.val_->is_string()) {
        return error("Could not cast to string.");
      }
      return _var.val_->as<std::string>();

    } else if constexpr (std::is_same<std::remove_cvref_t<T>,
                                      rfl::Bytestring>() ||
                         std::is_same<std::remove_cvref_t<T>,
                                      rfl::Vectorstring>()) {
      using VectorType = std::remove_cvref_t<T>;
      using ValueType = typename VectorType::value_type;
      if (!_var.val_->is<std::vector<uint8_t>>()) {
        if constexpr (std::is_same<std::remove_cvref_t<T>, rfl::Bytestring>()) {
          return error("Could not cast to bytestring.");
        } else {
          return error("Could not cast to vectorstring.");
        }
      }
      const auto vec = _var.val_->as<std::vector<uint8_t>>();
      const auto data = internal::ptr_cast<const ValueType*>(vec.data());
      return VectorType(data, data + vec.size());

    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      if (!_var.val_->is_bool()) {
        return error("Could not cast to boolean.");
      }
      return _var.val_->as<bool>();

    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      if (_var.val_->is_double()) {
        return static_cast<T>(_var.val_->as<double>());
      }
      return error("Could not cast to floating point value.");

    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      if (_var.val_->is_int64()) {
        return static_cast<T>(_var.val_->as<int64_t>());
      }
      if (_var.val_->is_uint64()) {
        return static_cast<T>(_var.val_->as<uint64_t>());
      }
      return error("Could not cast to integer.");

    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  rfl::Result<InputArrayType> to_array(
      const InputVarType& _var) const noexcept {
    if (!_var.val_->is_array()) {
      return error("Could not cast to an array.");
    }
    return InputArrayType{_var.val_};
  }

  rfl::Result<InputObjectType> to_object(
      const InputVarType& _var) const noexcept {
    if (!_var.val_->is_object()) {
      return error("Could not cast to an object.");
    }
    return InputObjectType{_var.val_};
  }

  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    if constexpr (requires { _array_reader.reserve(_arr.val_->size()); }) {
      if (auto err = _array_reader.reserve(_arr.val_->size())) {
        return err;
      }
    }
    for (auto& val : _arr.val_->array_range()) {
      auto err = _array_reader.read(InputVarType{&val});
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    for (auto& kv : _obj.val_->object_range()) {
      _object_reader.read(kv.key(), InputVarType{&kv.value()});
    }
    return std::nullopt;
  }

  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType& _var) const noexcept {
    try {
      return T::from_ubjson_obj(_var);
    } catch (std::exception& e) {
      return error(e.what());
    }
  }
};

}  // namespace rfl::ubjson

#endif
