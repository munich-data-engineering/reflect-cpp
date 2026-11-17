#ifndef RFL_INTERNAL_DEFERREDDEFAULTS_HPP_
#define RFL_INTERNAL_DEFERREDDEFAULTS_HPP_

#include <cstddef>
#include <optional>

#include "../Result.hpp"
#include "../common.hpp"

namespace rfl::internal {

/// Parser::read_struct_with_default default-constructs the struct it reads
/// into, to obtain the default values of the fields that are missing from the
/// input. If an rfl::Validator in it has an invalid default value, that
/// throws, even if the input contains the field. The parser then constructs
/// the struct again while a DeferredDefaults is active on this thread. Every
/// Validator that is default-constructed on this thread in the meantime keeps
/// its invalid default value instead of throwing, and is counted here. The
/// parser then checks with parsing::check_default_value that the struct holds
/// at least that many invalid Validators, so that none has been used up on
/// the way, and that none of them is in a field that is missing from the
/// input.
class DeferredDefaults {
 public:
  /// Makes _deferred the active DeferredDefaults of this thread for as long
  /// as the Scope lives. A null _deferred turns deferring off.
  class Scope {
   public:
    explicit Scope(DeferredDefaults* _deferred) noexcept : prev_(active()) {
      active() = _deferred;
    }

    ~Scope() { active() = prev_; }

    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

   private:
    DeferredDefaults* prev_;
  };

  /// Called by rfl::Validator() if its default value is invalid. Returns
  /// whether the Validator keeps the value instead of throwing.
  static bool defer(const Error& _error) {
    auto* deferred = active();
    if (!deferred) {
      return false;
    }
    if (!deferred->first_error_) {
      deferred->first_error_ = _error;
    }
    ++deferred->num_;
    return true;
  }

  /// Returns the current deferred count on this thread, or 0 when inactive.
  static size_t current_num() noexcept {
    auto* deferred = active();
    return deferred ? deferred->num_ : 0;
  }

  /// Resets the deferred count on this thread to _num when active.
  static void reset_num(size_t _num) noexcept {
    if (auto* deferred = active()) {
      deferred->num_ = _num;
    }
  }

  /// The number of Validators that have kept an invalid default value.
  size_t num() const noexcept { return num_; }

  /// Why the first of them is invalid.
  const std::optional<Error>& first_error() const noexcept {
    return first_error_;
  }

 private:
  RFL_API static DeferredDefaults*& active() noexcept;

 private:
  size_t num_ = 0;
  std::optional<Error> first_error_;
};

}  // namespace rfl::internal

#endif
