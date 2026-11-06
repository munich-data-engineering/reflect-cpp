#ifndef RFL_BOX_HPP_
#define RFL_BOX_HPP_

#include <memory>

#include "Result.hpp"

namespace rfl {


/// Enum defining whether a Box is copyable or not.
/// Determines the availability of copy constructor and assignment operators.
enum class Copyability {
    COPYABLE,      ///< Box can be copied (creates a deep copy of the contained object)
    NON_COPYABLE   ///< Box cannot be copied (behaves like unique_ptr)
};

/// A smart pointer wrapper that is guaranteed to always contain a valid object.
/// The Box class behaves very similarly to unique_ptr, but unlike unique_ptr,
/// it is 100% guaranteed to be filled at all times (unless the user
/// tries to access it after calling std::move or does something else that is
/// clearly bad practice).
///
/// By default Box behaves like a unique_ptr in relation to copying, but it can be
/// configured to add copy constructor and assignment operators that call the
/// same function of the contained type T.
/// @tparam T The type of object to contain
/// @tparam C Copyability mode (defaults to NON_COPYABLE)
template <class T, Copyability C = Copyability::NON_COPYABLE>
class Box {
 public:
  /// The only way of creating new boxes is Box<T>::make(...).
  /// Constructs a new Box with the given arguments forwarded to T's constructor.
  /// @tparam Args Types of constructor arguments
  /// @param _args Arguments to forward to T's constructor
  /// @return A new Box containing the constructed object
  template <class... Args>
  static Box make(Args&&... _args) {
    return Box(std::make_unique<T>(std::forward<Args>(_args)...));
  }

  /// Creates a Box from a unique_ptr, returns an Error if the unique_ptr is null.
  /// @param _ptr The unique_ptr to convert to a Box
  /// @return Result containing the Box or an error if _ptr is nullptr
  static Result<Box> make(std::unique_ptr<T>&& _ptr) {
    if (!_ptr) {
      return error("std::unique_ptr was a nullptr.");
    }
    return Box(std::move(_ptr));
  }

  /// Default constructor - creates a Box containing a default-constructed T.
  Box() : ptr_(std::make_unique<T>()) {}

  /// Copy constructor - only available when C == Copyability::COPYABLE.
  /// Creates a deep copy of the contained object.
  /// @param _other The Box to copy from
  Box(const Box& _other) requires (C == Copyability::COPYABLE)
  {
    ptr_ = std::make_unique<T>(*_other);
  }

  /// Copy constructor deleted when not copyable.
  Box(const Box& _other) requires (C == Copyability::NON_COPYABLE) = delete;

  /// Move constructor.
  /// @param _other The Box to move from
  Box(Box&& _other) = default;

  /// Move constructor from a Box with different copyability mode.
  /// @tparam U Type convertible to T
  /// @tparam C2 Copyability mode of the other Box
  /// @param _other The Box to move from
  template <class U, Copyability C2>
  Box(Box<U, C2>&& _other) noexcept
      : ptr_(std::forward<std::unique_ptr<U>>(_other.ptr())) {}

  /// Destructor.
  ~Box() = default;

  /// Returns a pointer to the underlying object.
  /// @return Raw pointer to the contained object
  T* get() const { return ptr_.get(); }

  /// Copy assignment operator - only available when C == Copyability::COPYABLE.
  /// Replaces the contained object with a deep copy of the other one. The copy
  /// is made before the old object is destroyed, so the other Box may be part
  /// of the object it replaces.
  /// @param other The Box to copy from
  /// @return Reference to this Box
  Box& operator=(const Box& other) requires (C == Copyability::COPYABLE) {
    if(this != &other) {
      ptr_ = std::make_unique<T>(*other);
    }
    return *this;
  }

  /// Copy assignment operator deleted when not copyable.
  Box& operator=(const Box& _other) requires (C == Copyability::NON_COPYABLE)  = delete;

  /// Move assignment operator.
  /// @param _other The Box to move from
  /// @return Reference to this Box
  Box& operator=(Box&& _other) noexcept = default;

  /// Move assignment operator from Box with different type.
  /// @tparam U Type convertible to T
  /// @param _other The Box to move from
  /// @return Reference to this Box
  template <class U>
  Box& operator=(Box<U>&& _other) noexcept {
    ptr_ = std::forward<std::unique_ptr<U>>(_other.ptr());
    return *this;
  }

  /// Dereference operator - returns the underlying object.
  /// @return Reference to the contained object
  T& operator*() { return *ptr_; }

  /// Dereference operator (const) - returns the underlying object.
  /// @return Const reference to the contained object
  T& operator*() const { return *ptr_; }

  /// Arrow operator - provides access to the underlying object's members.
  /// @return Pointer to the contained object
  T* operator->() { return ptr_.get(); }

  /// Arrow operator (const) - provides access to the underlying object's members.
  /// @return Const pointer to the contained object
  T* operator->() const { return ptr_.get(); }

  /// Returns the underlying unique_ptr.
  /// @return Reference to the internal unique_ptr
  std::unique_ptr<T>& ptr() { return ptr_; }

  /// Returns the underlying unique_ptr (const).
  /// @return Const reference to the internal unique_ptr
  const std::unique_ptr<T>& ptr() const { return ptr_; }

 private:
  /// Only make() is allowed to use this constructor.
  /// @param _ptr The unique_ptr to wrap
  explicit Box(std::unique_ptr<T>&& _ptr) : ptr_(std::move(_ptr)) {}

 private:
  /// The underlying unique_ptr.
  std::unique_ptr<T> ptr_;
};

/// Generates a new Box<T> by forwarding arguments to T's constructor.
/// @tparam T The type to box
/// @tparam Args Types of constructor arguments
/// @param _args Arguments to forward to T's constructor
/// @return A new Box<T>
template <class T, class... Args>
auto make_box(Args&&... _args) {
  return Box<T>::make(std::forward<Args>(_args)...);
}

/// Template specialization for a Box that is copyable.
/// CopyableBox allows copying via the copy constructor and assignment operators.
/// @tparam T The type to box
template<typename T>
using CopyableBox = Box<T, Copyability::COPYABLE>;

/// Generates a new CopyableBox<T> by forwarding arguments to T's constructor.
/// @tparam T The type to box
/// @tparam Args Types of constructor arguments
/// @param _args Arguments to forward to T's constructor
/// @return A new CopyableBox<T>
template <class T, class... Args>
auto make_copyable_box(Args&&... _args) {
    return CopyableBox<T>::make(std::forward<Args>(_args)...);
}

/// Three-way comparison operator for Boxes.
/// @tparam T1 Type of the first Box
/// @tparam T2 Type of the second Box
/// @param _b1 The first Box to compare
/// @param _b2 The second Box to compare
/// @return The ordering relationship between the underlying pointers
template <class T1, class T2>
inline auto operator<=>(const Box<T1>& _b1, const Box<T2>& _b2) {
  return _b1.ptr() <=> _b2.ptr();
}

/// Stream insertion operator for Box.
/// @tparam CharT Character type
/// @tparam Traits Character traits
/// @tparam T Type contained in the Box
/// @param _os The output stream
/// @param _b The Box to output
/// @return The output stream
template <class CharT, class Traits, class T>
inline std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& _os, const Box<T>& _b) {
  _os << _b.get();
  return _os;
}

}  // namespace rfl

namespace std {

/// Specialization of std::hash for Box.
/// Allows Box to be used in unordered containers.
/// @tparam T The type contained in the Box
template <class T>
struct hash<rfl::Box<T>> {
  /// Computes hash of the Box by hashing its underlying unique_ptr.
  /// @param _b The Box to hash
  /// @return The hash value
  size_t operator()(const rfl::Box<T>& _b) const {
    return std::hash<std::unique_ptr<T>>{}(_b.ptr());
  }
};

/// Specialization of std::swap for Box.
/// @tparam T The type contained in the Box
/// @param _b1 The first Box to swap
/// @param _b2 The second Box to swap
template <class T>
inline void swap(rfl::Box<T>& _b1, rfl::Box<T>& _b2) {
  return swap(_b1.ptr(), _b2.ptr());
}

}  // namespace std

#endif
