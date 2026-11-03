#ifndef RFL_RESULT_HPP_
#define RFL_RESULT_HPP_

#ifdef REFLECTCPP_USE_STD_EXPECTED
#include <expected>
#else
#include "internal/cpp20/Result.hpp"
#endif

#include "internal/Error.hpp"

namespace rfl {

using Error = rfl::internal::Error;
using ErrorDetail = rfl::internal::ErrorDetail;

/// To be returned when there is nothing to return, but there might be an error.
struct Nothing {};

/// This implementation is for cases where std::expected is defined
#ifdef REFLECTCPP_USE_STD_EXPECTED

template <class E>
using Unexpected = std::unexpected<E>;

template <class T>
using Result = std::expected<T, rfl::Error>;

/// This implementation is for cases where std::expected is not defined
#else  // REFLECTCPP_USE_STD_EXPECTED

template <class E>
using Unexpected = rfl::internal::cpp20::Unexpected<E>;

template <class T>
using Result = rfl::internal::cpp20::Result<T>;

#endif

/// Shorthand for unexpected error.
inline Unexpected<Error> error(const std::string& _what) {
  return Unexpected<Error>(Error(_what));
}

inline Unexpected<Error> error(std::string&& _what) {
  return Unexpected<Error>(Error(std::move(_what)));
}

/// Shorthand for unexpected error.
inline Unexpected<Error> error(const Error& _err) {
  return Unexpected<Error>(_err);
}

}  // namespace rfl

#ifdef REFLECTCPP_USE_STD_EXPECTED
template <>
class std::bad_expected_access<rfl::Error> : public bad_expected_access<void> {
 public:
  /// @brief Constructs a bad_expected_access exception with the given error.
  /// @param er The rfl::Error object to store in this exception.
  explicit constexpr bad_expected_access(rfl::Error er) : err_(std::move(er)) {}

  /// @brief Returns a C-string describing the error.
  /// @return A pointer to a null-terminated string with the error message.
  /// @note Overrides std::exception::what(). The string is owned by the
  /// underlying rfl::Error.
  const char* what() const noexcept override { return err_.what().c_str(); }

  /// @brief Returns a reference to the contained error object.
  /// @return Reference to the stored rfl::Error.
  [[nodiscard]] rfl::Error& error() & noexcept { return err_; }

  /// @brief Returns a const reference to the contained error object.
  /// @return Const reference to the stored rfl::Error.
  [[nodiscard]] const rfl::Error& error() const& noexcept { return err_; }

  /// @brief Moves and returns the contained error object.
  /// @return Rvalue reference to the stored rfl::Error.
  [[nodiscard]] rfl::Error&& error() && noexcept { return std::move(err_); }

  /// @brief Moves and returns the contained error object from a const rvalue.
  /// @return Const rvalue reference to the stored rfl::Error.
  [[nodiscard]] const rfl::Error&& error() const&& noexcept {
    return std::move(err_);
  }

 private:
  rfl::Error err_;
};
#endif

#endif
