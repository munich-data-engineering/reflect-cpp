#ifndef RFL_PARSING_WRAPERROR_HPP_
#define RFL_PARSING_WRAPERROR_HPP_

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "../internal/cold.hpp"

namespace rfl::parsing {

/**
 * @brief Wraps an error that occurred while reading a nested value, such as a
 * field or an array element.
 *
 * @param _prefix The text to put in front of the error message.
 * @param _segment The location of the nested value relative to its parent,
 * which is prepended to the path of every detail.
 * @param _err The error that occurred while reading the nested value.
 * @return The error as seen from the parent.
 */
RFL_COLD inline Error wrap_error(const std::string_view _prefix,
                                 const ErrorDetail::PathSegment& _segment,
                                 Error&& _err) {
  auto what = std::string(_prefix) + _err.what();
  auto details = std::move(_err).details();
  for (auto& detail : details) {
    detail.path.insert(detail.path.begin(), _segment);
  }
  return Error(std::move(what), std::move(details));
}

/**
 * @brief Wraps an error that occurred while reading a field.
 *
 * @param _name The name of the field.
 * @param _err The error that occurred while reading the field.
 * @return The error as seen from the object containing the field.
 */
RFL_COLD inline Error wrap_field_error(const std::string_view _name,
                                       Error&& _err) {
  auto name = std::string(_name);
  return wrap_error("Failed to parse field '" + name + "': ", name,
                    std::move(_err));
}

/**
 * @brief Adds context to an error without changing the location at which it
 * occurred.
 *
 * @param _prefix The text to put in front of the error message. It is also put
 * in front of the messages of all details at the root, because these describe
 * the same failure as the error message.
 * @param _err The error to add context to.
 * @return The error with the added context.
 */
RFL_COLD inline Error wrap_error(const std::string_view _prefix, Error&& _err) {
  auto what = std::string(_prefix) + _err.what();
  auto details = std::move(_err).details();
  for (auto& detail : details) {
    if (detail.path.empty()) {
      detail.message = std::string(_prefix) + detail.message;
    }
  }
  return Error(std::move(what), std::move(details));
}

/**
 * @brief Creates an error for a nested value that could not be read at all,
 * for instance because it is missing.
 *
 * @param _segment The location of the nested value relative to its parent.
 * @param _msg The error message.
 * @return The error as seen from the parent.
 */
RFL_COLD inline Error make_nested_error(
    const ErrorDetail::PathSegment& _segment, std::string _msg) {
  auto details = std::vector<ErrorDetail>(
      {ErrorDetail{.path = {_segment}, .message = _msg}});
  return Error(std::move(_msg), std::move(details));
}

}  // namespace rfl::parsing

#endif
