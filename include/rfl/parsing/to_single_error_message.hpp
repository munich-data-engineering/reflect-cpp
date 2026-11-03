#ifndef RFL_PARSING_TOSINGLEERRORMESSAGE_HPP_
#define RFL_PARSING_TOSINGLEERRORMESSAGE_HPP_

#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "../internal/cold.hpp"
#include "../internal/strings/strings.hpp"

namespace rfl::parsing {

/**
 * @brief Combines a set of errors to a single, readable error message.
 *
 * @param _errors The vector of error messages.
 * @param _msg_prefix An optional prefix for the error message.
 * @param _err_limit The maximum number of errors to show.
 * @return The combined error message.
 */
inline std::string to_single_error_message(
    std::vector<std::string> _errors,
    std::optional<std::string> _msg_prefix = std::nullopt,
    size_t _err_limit = 10) {
  if (_errors.size() == 1) {
    return _errors[0];
  } else {
    std::stringstream stream;
    stream << (_msg_prefix
                   ? *_msg_prefix
                   : "Found " + std::to_string(_errors.size()) + " errors:");
    for (size_t i = 0; i < _errors.size() && i < _err_limit; ++i) {
      stream << "\n"
             << i + 1 << ") "
             << internal::strings::replace_all(_errors.at(i), "\n", "\n    ");
    }
    if (_errors.size() > _err_limit) {
      stream << "\n...\nMore than " << _err_limit
             << " errors occurred, but I am only showing the "
                "first "
             << _err_limit << ".";
    }
    return stream.str();
  }
}

/**
 * @brief Combines a set of errors to a single error.
 *
 * @param _errors The vector of errors.
 * @return An error whose message is the combined error message, as produced by
 * to_single_error_message, and whose details are the details of all errors.
 */
RFL_COLD inline Error to_single_error(std::vector<Error> _errors) {
  if (_errors.size() == 1) {
    return std::move(_errors[0]);
  }
  std::vector<std::string> messages;
  messages.reserve(_errors.size());
  std::vector<ErrorDetail> details;
  details.reserve(_errors.size());
  for (auto& err : _errors) {
    messages.emplace_back(err.what());
    for (auto& detail : std::move(err).details()) {
      details.emplace_back(std::move(detail));
    }
  }
  return Error(to_single_error_message(std::move(messages)),
               std::move(details));
}

}  // namespace rfl::parsing

#endif
