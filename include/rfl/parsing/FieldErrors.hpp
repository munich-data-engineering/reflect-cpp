#ifndef RFL_PARSING_FIELDERRORS_HPP_
#define RFL_PARSING_FIELDERRORS_HPP_

#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "../internal/fail_fast_v.hpp"

namespace rfl::parsing {

class FirstFieldError {
 public:
  bool should_stop() const { return error_.has_value(); }

  void reserve(std::size_t) {}

  void emplace_back(Error _err) {
    if (!error_) error_.emplace(std::move(_err));
  }

  std::optional<Error> error() && { return std::move(error_); }

 private:
  std::optional<Error> error_;
};

template <class ProcessorsType>
using FieldErrors = std::conditional_t<internal::fail_fast_v<ProcessorsType>,
                                       FirstFieldError, std::vector<Error>>;

template <class ProcessorsType>
bool should_stop_on_field_error(const FieldErrors<ProcessorsType>& errors) {
  if constexpr (internal::fail_fast_v<ProcessorsType>) {
    return errors.should_stop();
  } else {
    return false;
  }
}

}  // namespace rfl::parsing

#endif
