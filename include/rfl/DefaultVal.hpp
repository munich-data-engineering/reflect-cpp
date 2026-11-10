#ifndef RFL_DEFAULTVAL_HPP_
#define RFL_DEFAULTVAL_HPP_

#include <type_traits>
#include <utility>

#include "default.hpp"

namespace rfl {

/// Wraps a field with a default value that will be used when the field is missing during parsing.
/// When combined with the DefaultIfMissing processor, fields wrapped in DefaultVal will use
/// their stored default value if not present in the input. This is more explicit than relying
/// on default constructors.
/// @tparam T The type of the field value
template <class T>
struct DefaultVal {
 public:
  using Type = std::remove_cvref_t<T>;

  /// Default constructor - initializes with default-constructed value.
  DefaultVal() : value_(Type()) {}

  /// Constructs from a const reference to the value.
  /// @param _value The value to store
  DefaultVal(const Type& _value) : value_(_value) {}

  /// Constructs from an rvalue reference to the value.
  /// @param _value The value to store (will be moved)
  DefaultVal(Type&& _value) noexcept : value_(std::move(_value)) {}

  /// Move constructor.
  /// @param _field The DefaultVal to move from
  DefaultVal(DefaultVal&& _field) noexcept = default;

  /// Copy constructor.
  /// @param _field The DefaultVal to copy from
  DefaultVal(const DefaultVal& _field) = default;

  /// Copy constructor from DefaultVal with compatible type.
  /// @tparam U Type compatible with T
  /// @param _field The DefaultVal to copy the value from
  template <class U>
  DefaultVal(const DefaultVal<U>& _field) : value_(_field.get()) {}

  /// Move constructor from DefaultVal with compatible type.
  /// @tparam U Type compatible with T
  /// @param _field The DefaultVal to move the value from
  template <class U>
  DefaultVal(DefaultVal<U>&& _field) noexcept(
      noexcept(Type(std::move(_field.value()))))
      : value_(std::move(_field.value())) {}

  /// Constructs from any type convertible to Type (copy).
  /// @tparam U Type convertible to T
  /// @param _value The value to convert and store
  template <class U>
    requires(std::is_convertible_v<U, Type>)
  DefaultVal(const U& _value) : value_(_value) {}

  /// Constructs from any type convertible to Type (move).
  /// @tparam U Type convertible to T
  /// @param _value The value to convert and store (will be moved)
  template <class U>
    requires(std::is_convertible_v<U, Type>)
  DefaultVal(U&& _value) noexcept(std::is_nothrow_constructible_v<Type, U&&>)
      : value_(std::forward<U>(_value)) {}

  /// Constructs from a DefaultVal with compatible type.
  /// @tparam U Type convertible to T
  /// @param _field The DefaultVal to copy the value from
  template <class U>
    requires(std::is_convertible_v<U, Type>)
  DefaultVal(const DefaultVal<U>& _field) : value_(_field.value()) {}

  /// Assigns the underlying object to its default value using the Default sentinel.
  /// @tparam U The type (must be default constructible)
  /// @param The default sentinel value
  template <class U = Type>
    requires(std::is_default_constructible_v<U>)
  DefaultVal(const Default&) : value_(Type()) {}

  /// Destructor.
  ~DefaultVal() = default;

  /// Returns the underlying value.
  /// @return Const reference to the stored value
  const Type& get() const noexcept { return value_; }

  /// Returns the underlying value.
  /// @return Reference to the stored value
  Type& get() noexcept { return value_; }

  /// Dereference operator - returns the underlying value.
  /// @return Reference to the stored value
  Type& operator*() noexcept { return value_; }

  /// Dereference operator (const) - returns the underlying value.
  /// @return Const reference to the stored value
  const Type& operator*() const noexcept { return value_; }

  /// Function call operator - returns the underlying value.
  /// @return Reference to the stored value
  Type& operator()() noexcept { return value_; }

  /// Function call operator (const) - returns the underlying value.
  /// @return Const reference to the stored value
  const Type& operator()() const noexcept { return value_; }

  /// Pointer to the underlying value.
  /// @return Pointer to the stored value
  Type* operator->() noexcept { return &value_; }

  /// Pointer to the underlying value (const).
  /// @return Const pointer to the stored value
  const Type* operator->() const noexcept { return &value_; }

  /// Assigns a new value.
  /// @param _value The value to assign
  /// @return Reference to this DefaultVal
  auto& operator=(const Type& _value) {
    value_ = _value;
    return *this;
  }

  /// Assigns a new value (move version).
  /// @param _value The value to assign (will be moved)
  /// @return Reference to this DefaultVal
  auto& operator=(Type&& _value) noexcept {
    value_ = std::move(_value);
    return *this;
  }

  /// Assigns from any type convertible to the underlying type.
  /// @tparam U Type convertible to T
  /// @param _value The value to convert and assign
  /// @return Reference to this DefaultVal
  template <class U>
    requires std::is_convertible_v<U, Type>
  auto& operator=(const U& _value) {
    value_ = _value;
    return *this;
  }

  /// Assigns the value to its default using the Default sentinel.
  /// @tparam U The type (must be default constructible)
  /// @param The default sentinel value
  /// @return Reference to this DefaultVal
  template <class U = Type>
    requires std::is_default_constructible_v<U>
  auto& operator=(const Default&) {
    value_ = Type();
    return *this;
  }

  /// Copy assignment operator.
  /// @param _field The DefaultVal to copy from
  /// @return Reference to this DefaultVal
  DefaultVal& operator=(const DefaultVal& _field) = default;

  /// Move assignment operator.
  /// @param _field The DefaultVal to move from
  /// @return Reference to this DefaultVal
  DefaultVal& operator=(DefaultVal&& _field) = default;

  /// Assigns from another DefaultVal with compatible type (copy).
  /// @tparam U Type compatible with T
  /// @param _field The DefaultVal to copy the value from
  /// @return Reference to this DefaultVal
  template <class U>
  auto& operator=(const DefaultVal<U>& _field) {
    value_ = _field.get();
    return *this;
  }

  /// Assigns from another DefaultVal with compatible type (move).
  /// @tparam U Type compatible with T
  /// @param _field The DefaultVal to move the value from
  /// @return Reference to this DefaultVal
  template <class U>
  auto& operator=(DefaultVal<U>&& _field) {
    value_ = std::move(_field.value_);
    return *this;
  }

  /// Sets the value (copy version).
  /// @param _value The value to set
  void set(const Type& _value) { value_ = _value; }

  /// Sets the value (move version).
  /// @param _value The value to set (will be moved)
  void set(Type&& _value) { value_ = std::move(_value); }

  /// Returns the underlying value.
  /// @return Reference to the stored value
  Type& value() noexcept { return value_; }

  /// Returns the underlying value (const).
  /// @return Const reference to the stored value
  const Type& value() const noexcept { return value_; }

  /// The underlying value.
  Type value_;
};

}  // namespace rfl

#endif
