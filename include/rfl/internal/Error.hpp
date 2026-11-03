#ifndef RFL_INTERNAL_ERROR_HPP_
#define RFL_INTERNAL_ERROR_HPP_

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "cold.hpp"

namespace rfl::internal {

/// Describes a single failure contained in an Error: where it occurred and
/// what went wrong.
struct ErrorDetail {
  /// A field name or an array index.
  using PathSegment = std::variant<std::string, size_t>;

  /// The location of the failure, starting from the root of the value being
  /// read. Field names are spelled as they appear in the input, that is, after
  /// processors such as rfl::SnakeCaseToCamelCase have been applied. An empty
  /// path refers to the root value itself.
  std::vector<PathSegment> path;

  /// The message describing the failure, without the "Failed to parse field
  /// '...': " prefixes that Error::what() contains for every level of nesting.
  std::string message;

  bool operator==(const ErrorDetail&) const = default;
};

/// Defines the error class to be returned when something went wrong.
/// This is used throughout reflect-cpp for error handling.
class Error {
 public:
  /// Default constructor - creates an empty error.
  Error() = default;

  /// Constructs an error with a message (copy).
  /// @param _what The error message describing what went wrong
  Error(const std::string& _what) : what_(_what) {}

  /// Constructs an error with a message (move).
  /// @param _what The error message describing what went wrong (will be moved)
  Error(std::string&& _what) : what_(std::move(_what)) {}

  /// Constructs an error with a message and the individual failures it
  /// summarizes.
  /// @param _what The error message describing what went wrong
  /// @param _details The individual failures, see details()
  Error(std::string _what, std::vector<ErrorDetail> _details)
      : what_(std::move(_what)),
        details_(_details.empty()
                     ? nullptr
                     : new std::vector<ErrorDetail>(std::move(_details))) {}

  /// Destructor.
  ~Error() = default;

  /// Copy constructor.
  /// @param e The error to copy from
  Error(const Error& e) : what_(e.what_), details_(copy_details(e.details_)) {}

  /// Move constructor.
  /// @param e The error to move from
  Error(Error&& e) noexcept = default;

  /// Copy assignment operator.
  /// @param _other The error to copy from
  /// @return Reference to this error
  Error& operator=(const Error& _other) {
    if (this != &_other) {
      what_ = _other.what_;
      details_ = copy_details(_other.details_);
    }
    return *this;
  }

  /// Move assignment operator.
  /// @param _other The error to move from
  /// @return Reference to this error
  Error& operator=(Error&& _other) noexcept = default;

  /// Returns the error message, equivalent to .what() in std::exception.
  const std::string& what() const& { return what_; }
  /// Moves the error message out of Error object and leaves what_ in a moved
  /// from state
  std::string what() && { return std::move(what_); }

  /// Returns the individual failures summarized by what(), each with the
  /// location at which it occurred. An error that was constructed from a
  /// message only is reported as a single failure at the root.
  std::vector<ErrorDetail> details() const& {
    if (!details_) {
      return {ErrorDetail{.path = {}, .message = what_}};
    }
    return *details_;
  }

  /// Moves the individual failures out of the Error object and leaves it in a
  /// moved from state.
  std::vector<ErrorDetail> details() && {
    if (!details_) {
      return {ErrorDetail{.path = {}, .message = std::move(what_)}};
    }
    return std::move(*details_);
  }

 private:
  /// Errors are created and destroyed in many places, mostly as part of
  /// rfl::Result. Deleting and copying the details out of line keeps that code
  /// small, which matters for the performance of successful reads.
  struct DeleteDetails {
    RFL_COLD void operator()(std::vector<ErrorDetail>* _ptr) const {
      delete _ptr;
    }
  };

  using DetailsPtr = std::unique_ptr<std::vector<ErrorDetail>, DeleteDetails>;

  RFL_COLD static DetailsPtr copy_details(const DetailsPtr& _ptr) {
    return _ptr ? DetailsPtr(new std::vector<ErrorDetail>(*_ptr)) : nullptr;
  }

 private:
  /// Documents what went wrong
  std::string what_;

  /// The individual failures summarized by what_. Null, unless what_ was
  /// assembled from failures at nested locations.
  DetailsPtr details_;
};

}  // namespace rfl::internal

#endif
