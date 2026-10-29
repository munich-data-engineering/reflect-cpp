#ifndef RFL_PARSING_SCHEMA_TYPE_HPP_
#define RFL_PARSING_SCHEMA_TYPE_HPP_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../../Generic.hpp"
#include "../../Object.hpp"
#include "../../Ref.hpp"
#include "../../Variant.hpp"
#include "../../common.hpp"
#include "ValidationType.hpp"

namespace rfl::parsing::schema {

struct RFL_API Type {
  struct Boolean {};

  struct Null {};

  struct Bytestring {};

  struct Vectorstring {};

  struct Int32 {};

  struct Int64 {};

  struct UInt32 {};

  struct UInt64 {};

  struct Integer {};

  struct Float {};

  struct Double {};

  struct String {};

  struct AnyOf {
    std::vector<Type> types_;
  };

  struct Deprecated {
    std::string deprecation_message_;
    std::string description_;
    Ref<Type> type_;
  };

  struct Description {
    std::string description_;
    Ref<Type> type_;
  };

  struct FixedSizeTypedArray {
    size_t size_;
    Ref<Type> type_;
  };

  /// All values are assumed to be required unless explicitly stated otherwise
  /// using this or the Optional wrapper.
  struct DefaultVal {
    Ref<Type> type_;
    /// Empty when no default could be produced (distinct from "null").
    std::optional<Generic> default_value_;
  };

  struct DescribedLiteral {
    struct ValueWithDescription {
      std::string value_;
      std::string description_;
    };
    std::vector<ValueWithDescription> values_;
  };

  struct Literal {
    std::vector<std::string> values_;
  };

  struct Object {
    rfl::Object<Type> types_;
    rfl::Variant<std::shared_ptr<Type>, bool> additional_properties_;
  };

  /// All values are assumed to be required unless explicitly stated otherwise
  /// using this wrapper or the DefaultVal wrapper.
  struct Optional {
    Ref<Type> type_;
  };

  /// The is necessary to resolve circular definitions. Refers to something in
  /// Definitions.
  struct Reference {
    std::string name_;
  };

  // A map with key type string.
  struct StringMap {
    Ref<Type> value_type_;
  };

  struct Tuple {
    std::vector<Type> types_;
  };

  struct TypedArray {
    Ref<Type> type_;
  };

  struct Validated {
    Ref<Type> type_;
    ValidationType validation_;
  };

  using VariantType =
      rfl::Variant<Boolean, Null, Bytestring, Vectorstring, Int32, Int64,
                   UInt32, UInt64, Integer, Float, Double, String, AnyOf,
                   DefaultVal, Deprecated, Description, DescribedLiteral,
                   FixedSizeTypedArray, Literal, Object, Optional, Reference,
                   StringMap, Tuple, TypedArray, Validated>;

  /**
   * @brief Default constructor.
   */
  Type();

  /**
   * @brief Constructor.
   *
   * @param _variant The variant to use.
   */
  Type(const VariantType& _variant);

  /**
   * @brief Destructor.
   */
  ~Type();

  /// A type can be determined to be any of the above.
  VariantType variant_;
};

}  // namespace rfl::parsing::schema

#endif
