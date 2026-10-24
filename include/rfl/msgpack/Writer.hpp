#ifndef RFL_MSGPACK_WRITER_HPP_
#define RFL_MSGPACK_WRITER_HPP_

#include <msgpack.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include "../always_false.hpp"
#include "../common.hpp"
#include "../concepts.hpp"

namespace rfl::msgpack {

class RFL_API Writer {
 public:
  struct MsgpackOutputArray {};

  struct MsgpackOutputObject {};

  struct MsgpackOutputVar {};

  using OutputArrayType = MsgpackOutputArray;
  using OutputObjectType = MsgpackOutputObject;
  using OutputVarType = MsgpackOutputVar;

  /// A context for writing a typed map entry's key, then its value.
  struct OutputMapEntryType {};

  OutputMapEntryType map_entry(OutputObjectType*) const { return {}; }

  OutputArrayType add_array_to_map_entry(const size_t _size,
                                         OutputMapEntryType*) const {
    return array_as_root(_size);
  }

  OutputObjectType add_object_to_map_entry(const size_t _size,
                                           OutputMapEntryType*) const {
    return object_as_root(_size);
  }

  template <class T>
  OutputVarType add_value_to_map_entry(const T& _var,
                                       OutputMapEntryType*) const {
    return new_value(_var);
  }

  OutputVarType add_null_to_map_entry(OutputMapEntryType*) const {
    return null_as_root();
  }

  Writer(msgpack_packer* _pk);

  ~Writer();

  OutputArrayType array_as_root(const size_t _size) const;

  OutputObjectType object_as_root(const size_t _size) const;

  OutputVarType null_as_root() const;

  template <class T>
  OutputVarType value_as_root(const T& _var) const {
    return new_value(_var);
  }

  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* _parent) const;

  OutputArrayType add_array_to_object(const std::string_view& _name,
                                      const size_t _size,
                                      OutputObjectType* _parent) const;

  OutputObjectType add_object_to_array(const size_t _size,
                                       OutputArrayType* _parent) const;

  OutputObjectType add_object_to_object(const std::string_view& _name,
                                        const size_t _size,
                                        OutputObjectType* _parent) const;

  template <class T>
  OutputVarType add_value_to_array(const T& _var, OutputArrayType*) const {
    return new_value(_var);
  }

  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name,
                                    const T& _var, OutputObjectType*) const {
    auto err = msgpack_pack_str(pk_, _name.size());
    if (err) {
      throw std::runtime_error("Could not pack string.");
    }
    err = msgpack_pack_str_body(pk_, _name.data(), _name.size());
    if (err) {
      throw std::runtime_error("Could not pack string body.");
    }
    return new_value(_var);
  }

  OutputVarType add_null_to_array(OutputArrayType* _parent) const;

  OutputVarType add_null_to_object(const std::string_view& _name,
                                   OutputObjectType* _parent) const;

  void end_array(OutputArrayType* _arr) const noexcept;

  void end_object(OutputObjectType* _obj) const noexcept;

 private:
  OutputArrayType new_array(const size_t _size) const;

  OutputObjectType new_object(const size_t _size) const;

  template <class T>
  OutputVarType new_value(const T& _var) const {
    using Type = std::remove_cvref_t<T>;
    if constexpr (std::is_same<Type, std::string>()) {
      auto err = msgpack_pack_str(pk_, _var.size());
      if (err) {
        throw std::runtime_error("Could not pack string.");
      }
      err = msgpack_pack_str_body(pk_, _var.c_str(), _var.size());
      if (err) {
        throw std::runtime_error("Could not pack string body.");
      }

    } else if constexpr (concepts::MutableContiguousByteContainer<Type>) {
      auto err = msgpack_pack_bin(pk_, _var.size());
      if (err) {
        throw std::runtime_error("Could not pack binary string.");
      }
      err = msgpack_pack_bin_body(pk_, _var.data(), _var.size());
      if (err) {
        throw std::runtime_error("Could not pack binary string.");
      }

    } else if constexpr (std::is_same<Type, bool>()) {
      if (_var) {
        const auto err = msgpack_pack_true(pk_);
        if (err) {
          throw std::runtime_error("Could not pack boolean.");
        }
      } else {
        const auto err = msgpack_pack_false(pk_);
        if (err) {
          throw std::runtime_error("Could not pack boolean.");
        }
      }

    } else if constexpr (std::is_same<Type, float>()) {
      const auto err = msgpack_pack_float(pk_, _var);
      if (err) {
        throw std::runtime_error("Could not pack float.");
      }

    } else if constexpr (std::is_same<Type, double>()) {
      const auto err = msgpack_pack_double(pk_, _var);
      if (err) {
        throw std::runtime_error("Could not pack double.");
      }

    } else if constexpr (std::is_floating_point<Type>()) {
      // Preserve the existing narrowing behavior for floating-point types
      // other than float and double.
      const auto err = msgpack_pack_double(pk_, static_cast<double>(_var));
      if (err) {
        throw std::runtime_error("Could not pack floating-point value.");
      }

    } else if constexpr (std::is_unsigned<Type>()) {
      const auto err =
          msgpack_pack_uint64(pk_, static_cast<std::uint64_t>(_var));
      if (err) {
        throw std::runtime_error("Could not pack uint.");
      }

    } else if constexpr (std::is_integral<Type>()) {
      const auto err = msgpack_pack_int64(pk_, static_cast<std::int64_t>(_var));
      if (err) {
        throw std::runtime_error("Could not pack int.");
      }

    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
    return OutputVarType{};
  }

 private:
  /// The underlying packer.
  msgpack_packer* pk_;
};

}  // namespace rfl::msgpack

#endif
