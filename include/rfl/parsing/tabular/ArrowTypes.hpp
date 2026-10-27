#ifndef RFL_PARSING_TABULAR_ARROWTYPES_HPP_
#define RFL_PARSING_TABULAR_ARROWTYPES_HPP_

// Silence a -Warray-bounds false positive in Apache Arrow
// (buffer_builder.h) with GCC 16.
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
#endif
#include <arrow/api.h>
#ifdef __GNUC__
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
#endif

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "../../Box.hpp"
#include "../../Bytestring.hpp"
#include "../../NamedTuple.hpp"
#include "../../Ref.hpp"
#include "../../Rename.hpp"
#include "../../Timestamp.hpp"
#include "../../Tuple.hpp"
#include "../../concepts.hpp"
#include "../../enums.hpp"
#include "../../internal/StringLiteral.hpp"
#include "../../internal/bitset.hpp"
#include "../../internal/has_reflection_type_v.hpp"
#include "../../internal/ptr_cast.hpp"
#include "../../named_tuple_t.hpp"

namespace rfl::parsing::tabular {

enum class SerializationType { csv, parquet };

template <class T, SerializationType _s>
struct ArrowTypes;

/**
 * @brief Transforms a numerical array to the desired type T.
 * @tparam T The desired type.
 * @tparam _s The serialization type.
 * @param _arr The array to transform.
 * @return The transformed array or an error.
 */
template <class T, SerializationType _s>
Result<Ref<typename ArrowTypes<T, _s>::ArrayType>> transform_numerical_array(
    const std::shared_ptr<arrow::Array>& _arr) noexcept;

template <SerializationType _s>
struct ArrowTypes<bool, _s> {
  using ArrayType = arrow::BooleanArray;
  using BuilderType = arrow::BooleanBuilder;

  /**
   * @brief Returns the Arrow data type for boolean.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::boolean(); }

  /**
   * @brief Adds a boolean value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const bool _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the boolean array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The boolean array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    if (_arr->type()->Equals(data_type())) {
      return Ref<ArrayType>::make(std::static_pointer_cast<ArrayType>(_arr));
    } else {
      return error("Expected boolean array, got " + _arr->type()->ToString() +
                   ".");
    }
  }

  /**
   * @brief Returns the boolean value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The boolean value.
   */
  static Result<bool> get_value(const Ref<ArrayType>& _chunk,
                                const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for boolean arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<uint8_t, _s> {
  using ArrayType = arrow::UInt8Array;
  using BuilderType = arrow::UInt8Builder;
  using T = uint8_t;

  /**
   * @brief Returns the Arrow data type for uint8.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::uint8(); }

  /**
   * @brief Adds a uint8 value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the uint8 array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The uint8 array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return transform_numerical_array<T, _s>(_arr);
  }

  /**
   * @brief Returns the uint8 value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The uint8 value.
   */
  static Result<uint8_t> get_value(const Ref<ArrayType>& _chunk,
                                   const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for uint8 arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<uint16_t, _s> {
  using ArrayType = arrow::UInt16Array;
  using BuilderType = arrow::UInt16Builder;
  using T = uint16_t;

  /**
   * @brief Returns the Arrow data type for uint16.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::uint16(); }

  /**
   * @brief Adds a uint16 value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the uint16 array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The uint16 array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return transform_numerical_array<T, _s>(_arr);
  }

  /**
   * @brief Returns the uint16 value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The uint16 value.
   */
  static Result<uint16_t> get_value(const Ref<ArrayType>& _chunk,
                                    const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for uint16 arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<uint32_t, _s> {
  using ArrayType = arrow::UInt32Array;
  using BuilderType = arrow::UInt32Builder;
  using T = uint32_t;

  /**
   * @brief Returns the Arrow data type for uint32.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::uint32(); }

  /**
   * @brief Adds a uint32 value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the uint32 array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The uint32 array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return transform_numerical_array<T, _s>(_arr);
  }

  /**
   * @brief Returns the uint32 value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The uint32 value.
   */
  static Result<uint32_t> get_value(const Ref<ArrayType>& _chunk,
                                    const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for uint32 arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<uint64_t, _s> {
  using ArrayType = arrow::UInt64Array;
  using BuilderType = arrow::UInt64Builder;
  using T = uint64_t;

  /**
   * @brief Returns the Arrow data type for uint64.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::uint64(); }

  /**
   * @brief Adds a uint64 value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the uint64 array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The uint64 array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return transform_numerical_array<T, _s>(_arr);
  }

  /**
   * @brief Returns the uint64 value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The uint64 value.
   */
  static Result<uint64_t> get_value(const Ref<ArrayType>& _chunk,
                                    const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for uint64 arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<int8_t, _s> {
  using ArrayType = arrow::Int8Array;
  using BuilderType = arrow::Int8Builder;
  using T = int8_t;

  /**
   * @brief Returns the Arrow data type for int8.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::int8(); }

  /**
   * @brief Adds an int8 value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the int8 array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The int8 array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return transform_numerical_array<T, _s>(_arr);
  }

  /**
   * @brief Returns the int8 value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The int8 value.
   */
  static Result<int8_t> get_value(const Ref<ArrayType>& _chunk,
                                  const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for int8 arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<int16_t, _s> {
  using ArrayType = arrow::Int16Array;
  using BuilderType = arrow::Int16Builder;
  using T = int16_t;

  /**
   * @brief Returns the Arrow data type for int16.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::int16(); }

  /**
   * @brief Adds an int16 value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the int16 array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The int16 array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return transform_numerical_array<T, _s>(_arr);
  }

  /**
   * @brief Returns the int16 value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The int16 value.
   */
  static Result<int16_t> get_value(const Ref<ArrayType>& _chunk,
                                   const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for int16 arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<int32_t, _s> {
  using ArrayType = arrow::Int32Array;
  using BuilderType = arrow::Int32Builder;
  using T = int32_t;

  /**
   * @brief Returns the Arrow data type for int32.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::int32(); }

  /**
   * @brief Adds an int32 value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the int32 array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The int32 array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return transform_numerical_array<T, _s>(_arr);
  }

  /**
   * @brief Returns the int32 value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The int32 value.
   */
  static Result<int32_t> get_value(const Ref<ArrayType>& _chunk,
                                   const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for int32 arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<int64_t, _s> {
  using ArrayType = arrow::Int64Array;
  using BuilderType = arrow::Int64Builder;
  using T = int64_t;

  /**
   * @brief Returns the Arrow data type for int64.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::int64(); }

  /**
   * @brief Adds an int64 value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the int64 array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The int64 array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return transform_numerical_array<T, _s>(_arr);
  }

  /**
   * @brief Returns the int64 value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The int64 value.
   */
  static Result<int64_t> get_value(const Ref<ArrayType>& _chunk,
                                   const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for int64 arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<float, _s> {
  using ArrayType = arrow::FloatArray;
  using BuilderType = arrow::FloatBuilder;
  using T = float;

  /**
   * @brief Returns the Arrow data type for float32.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::float32(); }

  /**
   * @brief Adds a float value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the float array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The float array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return transform_numerical_array<T, _s>(_arr);
  }

  /**
   * @brief Returns the float value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The float value.
   */
  static Result<float> get_value(const Ref<ArrayType>& _chunk,
                                 const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for float arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<double, _s> {
  using ArrayType = arrow::DoubleArray;
  using BuilderType = arrow::DoubleBuilder;
  using T = double;

  /**
   * @brief Returns the Arrow data type for float64.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::float64(); }

  /**
   * @brief Adds a double value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the double array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The double array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return transform_numerical_array<T, _s>(_arr);
  }

  /**
   * @brief Returns the double value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The double value.
   */
  static Result<double> get_value(const Ref<ArrayType>& _chunk,
                                  const int64_t _ix) {
    return _chunk->Value(_ix);
  }

  /**
   * @brief Creates a builder for double arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <SerializationType _s>
struct ArrowTypes<std::string, _s> {
  using ArrayType = arrow::StringArray;
  using BuilderType = arrow::StringBuilder;

  /**
   * @brief Returns the Arrow data type for string (utf8).
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::utf8(); }

  /**
   * @brief Adds a string value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto& _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the string array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The string array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    if (_arr->type()->Equals(data_type())) {
      return Ref<ArrayType>::make(std::static_pointer_cast<ArrayType>(_arr));
    } else {
      return error("Expected string array, got " + _arr->type()->ToString() +
                   ".");
    }
  }

  /**
   * @brief Returns the string value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The string value.
   */
  static Result<std::string> get_value(const Ref<ArrayType>& _chunk,
                                       const int64_t _ix) {
    return std::string(_chunk->Value(_ix));
  }

  /**
   * @brief Creates a builder for string arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <class T, SerializationType _s>
  requires std::is_enum_v<T>
struct ArrowTypes<T, _s> {
  using ArrayType = arrow::StringArray;
  using BuilderType = arrow::StringBuilder;

  /**
   * @brief Returns the Arrow data type for enums (utf8).
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::utf8(); }

  /**
   * @brief Adds an enum value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const T _val, BuilderType* _builder) {
    const auto status = _builder->Append(enum_to_string(_val));
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the enum array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The enum array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return ArrowTypes<std::string, _s>::get_array(_arr);
  }

  /**
   * @brief Returns the enum value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The enum value.
   */
  static Result<T> get_value(const Ref<ArrayType>& _chunk, const int64_t _ix) {
    return string_to_enum<T>(std::string(_chunk->Value(_ix)));
  }

  /**
   * @brief Creates a builder for enum arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <class T, SerializationType _s>
  requires concepts::ContiguousByteContainer<T>
struct ArrowTypes<T, _s> {
  using ArrayType = arrow::BinaryArray;
  using BuilderType = arrow::BinaryBuilder;

  /**
   * @brief Returns the Arrow data type for binary.
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::binary(); }

  /**
   * @brief Adds a binary value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto& _val, BuilderType* _builder) {
    const auto status = _builder->Append(
        internal::ptr_cast<const uint8_t*>(_val.data()), _val.size());
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the binary array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The binary array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    if (_arr->type()->Equals(data_type())) {
      return Ref<ArrayType>::make(std::static_pointer_cast<ArrayType>(_arr));

    } else if (_arr->type()->Equals(arrow::utf8())) {
      return transform_string(
          std::static_pointer_cast<arrow::StringArray>(_arr));

    } else {
      return error("Expected binary or string array, got " +
                   _arr->type()->ToString() + ".");
    }
  }

  /**
   * @brief Returns the binary value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The binary value.
   */
  static Result<T> get_value(const Ref<ArrayType>& _chunk, const int64_t _ix) {
    const auto begin = internal::ptr_cast<const typename T::value_type*>(
        _chunk->Value(_ix).data());
    return T(begin, begin + _chunk->Value(_ix).size());
  }

  /**
   * @brief Creates a builder for binary arrays.
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }

  /**
   * @brief Transforms a string array to a binary array.
   * @param _arr The string array.
   * @return The binary array or an error.
   */
  static Result<Ref<arrow::BinaryArray>> transform_string(
      const std::shared_ptr<arrow::StringArray>& _arr) noexcept {
    if (!_arr) {
      return error(
          "transform_string: std::shared_ptr not set. This is a "
          "bug, please report.");
    }

    auto builder = arrow::BinaryBuilder();

    for (int64_t i = 0; i < _arr->length(); ++i) {
      if (_arr->IsNull(i)) {
        const auto status = builder.AppendNull();
        if (!status.ok()) {
          return error(status.message());
        }
      } else {
        const std::string_view s = _arr->Value(i);
        const auto status = builder.Append(
            internal::ptr_cast<const uint8_t*>(s.data()), s.size());
        if (!status.ok()) {
          return error(status.message());
        }
      }
    }

    std::shared_ptr<arrow::Array> res;
    const auto status = builder.Finish(&res);
    return Ref<arrow::BinaryArray>::make(
        std::static_pointer_cast<arrow::BinaryArray>(res));
  }
};

// CSV stores the fixed-length text form; Parquet stores a binary column.
template <std::size_t N, SerializationType _s>
struct ArrowTypes<std::bitset<N>, _s>
    : ArrowTypes<std::conditional_t<_s == SerializationType::csv, std::string,
                                    Bytestring>,
                 _s> {
  using Representation =
      std::conditional_t<_s == SerializationType::csv, std::string, Bytestring>;
  using Base = ArrowTypes<Representation, _s>;
  using ArrayType = typename Base::ArrayType;
  using BuilderType = typename Base::BuilderType;

  static void add_to_builder(const std::bitset<N>& _bits,
                             BuilderType* _builder) {
    if constexpr (_s == SerializationType::csv) {
      Base::add_to_builder(_bits.to_string(), _builder);
    } else {
      Base::add_to_builder(internal::bitset_to_bytes<Bytestring>(_bits),
                           _builder);
    }
  }

  static Result<std::bitset<N>> get_value(const Ref<ArrayType>& _chunk,
                                          const int64_t _ix) {
    return Base::get_value(_chunk, _ix).and_then([](const auto& _value) {
      if constexpr (_s == SerializationType::csv) {
        return internal::bitset_from_string<N>(_value);
      } else {
        return internal::bitset_from_bytes<N>(_value);
      }
    });
  }
};

template <internal::StringLiteral _format, SerializationType _s>
struct ArrowTypes<Timestamp<_format>, _s> {
  enum class TimeUnit { day, second, milli, micro, nano, string };

  using ArrayType = arrow::TimestampArray;
  using BuilderType = arrow::TimestampBuilder;

  /**
   * @brief Returns the Arrow data type for timestamps (milli).
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::timestamp(arrow::TimeUnit::MILLI); }

  /**
   * @brief Adds a timestamp value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto& _val, BuilderType* _builder) {
    const auto status = _builder->Append(_val.to_time_t() * 1000);
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the timestamp array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The timestamp array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    if (_arr->type()->Equals(data_type())) {
      return Ref<ArrayType>::make(std::static_pointer_cast<ArrayType>(_arr));

    } else if (_arr->type()->Equals(
                   arrow::timestamp(arrow::TimeUnit::SECOND))) {
      return transform_time_stamp<TimeUnit::second>(
          std::static_pointer_cast<arrow::TimestampArray>(_arr));

    } else if (_arr->type()->Equals(arrow::timestamp(arrow::TimeUnit::MICRO))) {
      return transform_time_stamp<TimeUnit::micro>(
          std::static_pointer_cast<arrow::TimestampArray>(_arr));

    } else if (_arr->type()->Equals(arrow::timestamp(arrow::TimeUnit::NANO))) {
      return transform_time_stamp<TimeUnit::nano>(
          std::static_pointer_cast<arrow::TimestampArray>(_arr));

    } else if (_arr->type()->Equals(arrow::date32())) {
      return transform_time_stamp<TimeUnit::day>(
          std::static_pointer_cast<arrow::Date32Array>(_arr));

    } else if (_arr->type()->Equals(arrow::date64())) {
      return transform_time_stamp<TimeUnit::milli>(
          std::static_pointer_cast<arrow::Date64Array>(_arr));

    } else if (_arr->type()->Equals(arrow::utf8())) {
      return transform_time_stamp<TimeUnit::string>(
          std::static_pointer_cast<arrow::StringArray>(_arr));

    } else {
      return error("Expected timestamp, date32, date64 or string array, got " +
                   _arr->type()->ToString() + ".");
    }
  }

  /**
   * @brief Returns the timestamp value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The timestamp value.
   */
  static Result<Timestamp<_format>> get_value(const Ref<ArrayType>& _chunk,
                                              const int64_t _ix) {
    return Timestamp<_format>(_chunk->Value(_ix) / 1000);
  }

  /**
   * @brief Creates a builder for timestamp arrays.
   * @return The builder.
   */
  static auto make_builder() {
    return BuilderType(data_type(), arrow::default_memory_pool());
  }

  /**
   * @brief Transforms an array with different time units to a timestamp array.
   * @tparam _unit The time unit of the source array.
   * @tparam SourceArrayType The type of the source array.
   * @param _arr The source array.
   * @return The timestamp array or an error.
   */
  template <TimeUnit _unit, class SourceArrayType>
  static Result<Ref<arrow::TimestampArray>> transform_time_stamp(
      const std::shared_ptr<SourceArrayType>& _arr) noexcept {
    if (!_arr) {
      return error(
          "transform_time_stamp: std::shared_ptr not set. This is a "
          "bug, please report.");
    }

    auto builder =
        arrow::TimestampBuilder(data_type(), arrow::default_memory_pool());

    for (int64_t i = 0; i < _arr->length(); ++i) {
      if (_arr->IsNull(i)) {
        const auto status = builder.AppendNull();
        if (!status.ok()) {
          return error(status.message());
        }
      } else {
        if constexpr (_unit == TimeUnit::day) {
          const auto status = builder.Append(
              static_cast<int64_t>(_arr->Value(i)) * 1000 * 24 * 60 * 60);
          if (!status.ok()) {
            return error(status.message());
          }
        } else if constexpr (_unit == TimeUnit::second) {
          const auto status =
              builder.Append(static_cast<int64_t>(_arr->Value(i) * 1000));
          if (!status.ok()) {
            return error(status.message());
          }
        } else if constexpr (_unit == TimeUnit::milli) {
          const auto status =
              builder.Append(static_cast<int64_t>(_arr->Value(i)));
          if (!status.ok()) {
            return error(status.message());
          }
        } else if constexpr (_unit == TimeUnit::micro) {
          const auto status =
              builder.Append(static_cast<int64_t>(_arr->Value(i) / 1000));
          if (!status.ok()) {
            return error(status.message());
          }
        } else if constexpr (_unit == TimeUnit::nano) {
          const auto status =
              builder.Append(static_cast<int64_t>(_arr->Value(i) / 1000000));
          if (!status.ok()) {
            return error(status.message());
          }
        } else if constexpr (_unit == TimeUnit::string) {
          const auto ts = Timestamp<_format>::make(std::string(_arr->Value(i)));
          if (!ts) {
            return error(ts.error().what());
          }
          const auto status = builder.Append(ts->to_time_t() * 1000);
          if (!status.ok()) {
            return error(status.message());
          }
        } else {
          static_assert(rfl::always_false_v<SourceArrayType>,
                        "Unsupported time unit.");
        }
      }
    }

    std::shared_ptr<arrow::Array> res;
    const auto status = builder.Finish(&res);
    return Ref<arrow::TimestampArray>::make(
        std::static_pointer_cast<arrow::TimestampArray>(res));
  }
};

template <internal::StringLiteral _format>
struct ArrowTypes<Timestamp<_format>, SerializationType::csv> {
  using ArrayType = arrow::TimestampArray;
  using BuilderType = arrow::StringBuilder;

  /**
   * @brief Returns the Arrow data type for timestamps (milli).
   * @return The Arrow data type.
   */
  static auto data_type() { return arrow::timestamp(arrow::TimeUnit::MILLI); }

  /**
   * @brief Adds a timestamp value to the builder (as string for CSV).
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const Timestamp<_format>& _val,
                             BuilderType* _builder) {
    const auto status = _builder->Append(_val.str());
    if (!status.ok()) {
      throw std::runtime_error(status.message());
    }
  }

  /**
   * @brief Returns the timestamp array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The timestamp array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return ArrowTypes<Timestamp<_format>,
                      SerializationType::parquet>::get_array(_arr);
  }

  /**
   * @brief Returns the timestamp value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The timestamp value.
   */
  static Result<Timestamp<_format>> get_value(const Ref<ArrayType>& _chunk,
                                              const int64_t _ix) {
    return ArrowTypes<Timestamp<_format>,
                      SerializationType::parquet>::get_value(_chunk, _ix);
  }

  /**
   * @brief Creates a builder for timestamp arrays (string builder for CSV).
   * @return The builder.
   */
  static auto make_builder() { return BuilderType(); }
};

template <class T, SerializationType _s>
  requires internal::has_reflection_type_v<T>
struct ArrowTypes<T, _s> {
  using ArrayType =
      typename ArrowTypes<typename T::ReflectionType, _s>::ArrayType;
  using BuilderType =
      typename ArrowTypes<typename T::ReflectionType, _s>::BuilderType;

  /**
   * @brief Returns the Arrow data type for T based on its ReflectionType.
   * @return The Arrow data type.
   */
  static auto data_type() {
    return ArrowTypes<typename T::ReflectionType, _s>::data_type();
  }

  /**
   * @brief Adds a value of type T to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto& _val, BuilderType* _builder) {
    ArrowTypes<typename T::ReflectionType, _s>::add_to_builder(
        _val.reflection(), _builder);
  }

  /**
   * @brief Returns the array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return ArrowTypes<typename T::ReflectionType, _s>::get_array(_arr);
  }

  /**
   * @brief Returns the value of type T from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The value of type T.
   */
  static Result<T> get_value(const Ref<ArrayType>& _chunk, const int64_t _ix) {
    return ArrowTypes<std::remove_cvref_t<typename T::ReflectionType>,
                      _s>::get_value(_chunk, _ix)
        .and_then([](const auto& _v) -> Result<T> {
          try {
            return T(_v);
          } catch (const std::exception& e) {
            return error(e.what());
          }
        });
  }

  /**
   * @brief Creates a builder for arrays of type T.
   * @return The builder.
   */
  static auto make_builder() {
    return ArrowTypes<typename T::ReflectionType, _s>::make_builder();
  }
};

template <class T, SerializationType _s>
struct ArrowTypes<std::optional<T>, _s> {
  using ArrayType = typename ArrowTypes<std::remove_cvref_t<T>, _s>::ArrayType;
  using BuilderType =
      typename ArrowTypes<std::remove_cvref_t<T>, _s>::BuilderType;

  /**
   * @brief Returns the Arrow data type for optional T.
   * @return The Arrow data type.
   */
  static auto data_type() { return ArrowTypes<T, _s>::data_type(); }

  /**
   * @brief Adds an optional value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto& _val, BuilderType* _builder) {
    if (_val) {
      ArrowTypes<T, _s>::add_to_builder(*_val, _builder);
    } else {
      const auto status = _builder->AppendNull();
      if (!status.ok()) {
        throw std::runtime_error(status.message());
      }
    }
  }

  /**
   * @brief Returns the array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return ArrowTypes<T, _s>::get_array(_arr);
  }

  /**
   * @brief Returns the optional value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The optional value.
   */
  static auto get_value(const Ref<ArrayType>& _chunk, const int64_t _ix) {
    return ArrowTypes<std::remove_cvref_t<T>, _s>::get_value(_chunk, _ix)
        .transform([](const auto& _v) { return std::make_optional<T>(_v); });
  }

  /**
   * @brief Creates a builder for arrays of optional T.
   * @return The builder.
   */
  static auto make_builder() { return ArrowTypes<T, _s>::make_builder(); }
};

template <class T, SerializationType _s>
struct ArrowTypes<std::shared_ptr<T>, _s> {
  using ArrayType = typename ArrowTypes<std::remove_cvref_t<T>, _s>::ArrayType;
  using BuilderType =
      typename ArrowTypes<std::remove_cvref_t<T>, _s>::BuilderType;

  /**
   * @brief Returns the Arrow data type for shared_ptr T.
   * @return The Arrow data type.
   */
  static auto data_type() { return ArrowTypes<T, _s>::data_type(); }

  /**
   * @brief Adds a shared_ptr value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto& _val, BuilderType* _builder) {
    if (_val) {
      ArrowTypes<T, _s>::add_to_builder(*_val, _builder);
    } else {
      const auto status = _builder->AppendNull();
      if (!status.ok()) {
        throw std::runtime_error(status.message());
      }
    }
  }

  /**
   * @brief Returns the array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return ArrowTypes<T, _s>::get_array(_arr);
  }

  /**
   * @brief Returns the shared_ptr value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The shared_ptr value.
   */
  static auto get_value(const Ref<ArrayType>& _chunk, const int64_t _ix) {
    return ArrowTypes<std::remove_cvref_t<T>, _s>::get_value(_chunk, _ix)
        .transform([](const auto& _v) { return std::make_shared<T>(_v); });
  }

  /**
   * @brief Creates a builder for arrays of shared_ptr T.
   * @return The builder.
   */
  static auto make_builder() { return ArrowTypes<T, _s>::make_builder(); }
};

template <class T, SerializationType _s>
struct ArrowTypes<std::unique_ptr<T>, _s> {
  using ArrayType = typename ArrowTypes<std::remove_cvref_t<T>, _s>::ArrayType;
  using BuilderType =
      typename ArrowTypes<std::remove_cvref_t<T>, _s>::BuilderType;

  /**
   * @brief Returns the Arrow data type for unique_ptr T.
   * @return The Arrow data type.
   */
  static auto data_type() { return ArrowTypes<T, _s>::data_type(); }

  /**
   * @brief Adds a unique_ptr value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto& _val, BuilderType* _builder) {
    if (_val) {
      ArrowTypes<T, _s>::add_to_builder(*_val, _builder);
    } else {
      const auto status = _builder->AppendNull();
      if (!status.ok()) {
        throw std::runtime_error(status.message());
      }
    }
  }

  /**
   * @brief Returns the array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return ArrowTypes<T, _s>::get_array(_arr);
  }

  /**
   * @brief Returns the unique_ptr value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The unique_ptr value.
   */
  static auto get_value(const Ref<ArrayType>& _chunk, const int64_t _ix) {
    return ArrowTypes<std::remove_cvref_t<T>, _s>::get_value(_chunk, _ix)
        .transform([](const auto& _v) { return std::make_unique<T>(_v); });
  }

  /**
   * @brief Creates a builder for arrays of unique_ptr T.
   * @return The builder.
   */
  static auto make_builder() { return ArrowTypes<T, _s>::make_builder(); }
};

template <class T, SerializationType _s>
struct ArrowTypes<Box<T>, _s> {
  using ArrayType = typename ArrowTypes<std::remove_cvref_t<T>, _s>::ArrayType;
  using BuilderType =
      typename ArrowTypes<std::remove_cvref_t<T>, _s>::BuilderType;

  /**
   * @brief Returns the Arrow data type for Box T.
   * @return The Arrow data type.
   */
  static auto data_type() { return ArrowTypes<T, _s>::data_type(); }

  /**
   * @brief Adds a Box value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto& _val, BuilderType* _builder) {
    ArrowTypes<T, _s>::add_to_builder(*_val, _builder);
  }

  /**
   * @brief Returns the array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return ArrowTypes<T, _s>::get_array(_arr);
  }

  /**
   * @brief Returns the Box value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The Box value.
   */
  static auto get_value(const Ref<ArrayType>& _chunk, const int64_t _ix) {
    return ArrowTypes<std::remove_cvref_t<T>, _s>::get_value(_chunk, _ix)
        .transform([](const auto& _v) { return Box<T>::make(_v); });
  }

  /**
   * @brief Creates a builder for arrays of Box T.
   * @return The builder.
   */
  static auto make_builder() { return ArrowTypes<T, _s>::make_builder(); }
};

template <class T, SerializationType _s>
struct ArrowTypes<Ref<T>, _s> {
  using ArrayType = typename ArrowTypes<std::remove_cvref_t<T>, _s>::ArrayType;
  using BuilderType =
      typename ArrowTypes<std::remove_cvref_t<T>, _s>::BuilderType;

  /**
   * @brief Returns the Arrow data type for Ref T.
   * @return The Arrow data type.
   */
  static auto data_type() { return ArrowTypes<T, _s>::data_type(); }

  /**
   * @brief Adds a Ref value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto& _val, BuilderType* _builder) {
    ArrowTypes<T, _s>::add_to_builder(*_val, _builder);
  }

  /**
   * @brief Returns the array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return ArrowTypes<T, _s>::get_array(_arr);
  }

  /**
   * @brief Returns the Ref value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The Ref value.
   */
  static auto get_value(const Ref<ArrayType>& _chunk, const int64_t _ix) {
    return ArrowTypes<std::remove_cvref_t<T>, _s>::get_value(_chunk, _ix)
        .transform([](const auto& _v) { return Ref<T>::make(_v); });
  }

  /**
   * @brief Creates a builder for arrays of Ref T.
   * @return The builder.
   */
  static auto make_builder() { return ArrowTypes<T, _s>::make_builder(); }
};

template <internal::StringLiteral _name, class T, SerializationType _s>
struct ArrowTypes<Rename<_name, T>, _s> {
  using ArrayType = typename ArrowTypes<std::remove_cvref_t<T>, _s>::ArrayType;
  using BuilderType =
      typename ArrowTypes<std::remove_cvref_t<T>, _s>::BuilderType;

  /**
   * @brief Returns the Arrow data type for Rename T.
   * @return The Arrow data type.
   */
  static auto data_type() { return ArrowTypes<T, _s>::data_type(); }

  /**
   * @brief Adds a Rename value to the builder.
   * @param _val The value to add.
   * @param _builder The builder to add to.
   */
  static void add_to_builder(const auto& _val, BuilderType* _builder) {
    ArrowTypes<T, _s>::add_to_builder(_val.value(), _builder);
  }

  /**
   * @brief Returns the array from the shared pointer.
   * @param _arr The shared pointer to the arrow array.
   * @return The array or an error.
   */
  static Result<Ref<ArrayType>> get_array(
      const std::shared_ptr<arrow::Array>& _arr) {
    return ArrowTypes<T, _s>::get_array(_arr);
  }

  /**
   * @brief Returns the Rename value from the array at the given index.
   * @param _chunk The array to get the value from.
   * @param _ix The index of the value.
   * @return The Rename value.
   */
  static auto get_value(const Ref<ArrayType>& _chunk, const int64_t _ix) {
    return ArrowTypes<std::remove_cvref_t<T>, _s>::get_value(_chunk, _ix)
        .transform([](const auto& _v) { return Rename<_name, T>(_v); });
  }

  /**
   * @brief Creates a builder for arrays of Rename T.
   * @return The builder.
   */
  static auto make_builder() { return ArrowTypes<T, _s>::make_builder(); }
};

/**
 * @brief Implementation of numerical array transformation.
 * @tparam T The target type.
 * @tparam _s The serialization type.
 * @tparam SourceArrayType The type of the source array.
 * @param _arr The source array.
 * @return The transformed array or an error.
 */
template <class T, SerializationType _s, class SourceArrayType>
Result<Ref<typename ArrowTypes<T, _s>::ArrayType>>
transform_numerical_array_impl(
    const std::shared_ptr<SourceArrayType>& _arr) noexcept {
  if (!_arr) {
    return error(
        "transform_numerical_array_impl: std::shared_ptr not set. This is a "
        "bug, please report.");
  }

  auto builder = ArrowTypes<T, _s>::make_builder();

  for (int64_t i = 0; i < _arr->length(); ++i) {
    if (_arr->IsNull(i)) {
      const auto status = builder.AppendNull();
      if (!status.ok()) {
        return error(status.message());
      }
    } else {
      const auto status = builder.Append(static_cast<T>(_arr->Value(i)));
      if (!status.ok()) {
        return error(status.message());
      }
    }
  }

  using TargetArrayType = typename ArrowTypes<T, _s>::ArrayType;

  std::shared_ptr<arrow::Array> res;
  const auto status = builder.Finish(&res);
  return Ref<TargetArrayType>::make(
      std::static_pointer_cast<TargetArrayType>(res));
}

/**
 * @brief Transforms a numerical array to the desired type T.
 * @tparam T The desired type.
 * @tparam _s The serialization type.
 * @param _arr The array to transform.
 * @return The transformed array or an error.
 */
template <class T, SerializationType _s>
Result<Ref<typename ArrowTypes<T, _s>::ArrayType>> transform_numerical_array(
    const std::shared_ptr<arrow::Array>& _arr) noexcept {
  if (!_arr) {
    return error(
        "Could not transform the numerical array. std::shared_ptr not set.");
  }

  using ArrayType = typename ArrowTypes<T, _s>::ArrayType;

  if (_arr->type()->Equals(ArrowTypes<T, _s>::data_type())) {
    return Ref<ArrayType>::make(std::static_pointer_cast<ArrayType>(_arr));

  } else if (_arr->type()->Equals(ArrowTypes<uint8_t, _s>::data_type())) {
    return transform_numerical_array_impl<T, _s>(
        std::static_pointer_cast<typename ArrowTypes<uint8_t, _s>::ArrayType>(
            _arr));

  } else if (_arr->type()->Equals(ArrowTypes<uint16_t, _s>::data_type())) {
    return transform_numerical_array_impl<T, _s>(
        std::static_pointer_cast<typename ArrowTypes<uint16_t, _s>::ArrayType>(
            _arr));

  } else if (_arr->type()->Equals(ArrowTypes<uint32_t, _s>::data_type())) {
    return transform_numerical_array_impl<T, _s>(
        std::static_pointer_cast<typename ArrowTypes<uint32_t, _s>::ArrayType>(
            _arr));

  } else if (_arr->type()->Equals(ArrowTypes<uint64_t, _s>::data_type())) {
    return transform_numerical_array_impl<T, _s>(
        std::static_pointer_cast<typename ArrowTypes<uint64_t, _s>::ArrayType>(
            _arr));

  } else if (_arr->type()->Equals(ArrowTypes<int8_t, _s>::data_type())) {
    return transform_numerical_array_impl<T, _s>(
        std::static_pointer_cast<typename ArrowTypes<int8_t, _s>::ArrayType>(
            _arr));

  } else if (_arr->type()->Equals(ArrowTypes<int16_t, _s>::data_type())) {
    return transform_numerical_array_impl<T, _s>(
        std::static_pointer_cast<typename ArrowTypes<int16_t, _s>::ArrayType>(
            _arr));

  } else if (_arr->type()->Equals(ArrowTypes<int32_t, _s>::data_type())) {
    return transform_numerical_array_impl<T, _s>(
        std::static_pointer_cast<typename ArrowTypes<int32_t, _s>::ArrayType>(
            _arr));

  } else if (_arr->type()->Equals(ArrowTypes<int64_t, _s>::data_type())) {
    return transform_numerical_array_impl<T, _s>(
        std::static_pointer_cast<typename ArrowTypes<int64_t, _s>::ArrayType>(
            _arr));

  } else if (_arr->type()->Equals(ArrowTypes<float, _s>::data_type())) {
    return transform_numerical_array_impl<T, _s>(
        std::static_pointer_cast<typename ArrowTypes<float, _s>::ArrayType>(
            _arr));

  } else if (_arr->type()->Equals(ArrowTypes<double, _s>::data_type())) {
    return transform_numerical_array_impl<T, _s>(
        std::static_pointer_cast<typename ArrowTypes<double, _s>::ArrayType>(
            _arr));

  } else {
    return error("Expected numerical array, got " + _arr->type()->ToString() +
                 ".");
  }
}

}  // namespace rfl::parsing::tabular

#endif
