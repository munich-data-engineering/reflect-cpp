#ifndef RFL_INTERNAL_DEFERREDDEFAULTS_HPP_
#define RFL_INTERNAL_DEFERREDDEFAULTS_HPP_

#include <cstddef>
#include <functional>
#include <vector>

#include "../Result.hpp"

namespace rfl::internal {

/// Parser::read_struct_with_default default-constructs the struct it reads
/// into, to obtain the default values of the fields that are missing from the
/// input. If an rfl::Validator in it has an invalid default value, that
/// throws, even if the input contains the field. The parser then constructs
/// the struct again while a DeferredDefaults is active on this thread. A
/// Validator inside the struct then keeps its invalid default value instead
/// of throwing and is recorded here, so that the parser can check after
/// reading that the value has been replaced by one from the input. Validators
/// anywhere else, such as temporaries or on the heap, throw as usual.
class DeferredDefaults {
 public:
  struct Record {
    /// The address of the Validator.
    const void* address;

    /// Why its default value is invalid.
    Error error;
  };

  /// Makes _deferred the active DeferredDefaults of this thread for as long
  /// as the Scope lives.
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

  /// _begin and _size describe the storage of the struct.
  DeferredDefaults(const void* _begin, size_t _size) noexcept
      : begin_(static_cast<const unsigned char*>(_begin)),
        end_(begin_ + _size) {}

  /// Called by rfl::Validator() if its default value is invalid. Returns
  /// whether the Validator keeps the value instead of throwing.
  static bool defer(const void* _address, const Error& _error) {
    auto* deferred = active();
    const auto* p = static_cast<const unsigned char*>(_address);
    const auto less = std::less<const unsigned char*>();
    if (!deferred || less(p, deferred->begin_) || !less(p, deferred->end_)) {
      return false;
    }
    deferred->records_.push_back(Record{_address, _error});
    return true;
  }

  const std::vector<Record>& records() const noexcept { return records_; }

 private:
  static DeferredDefaults*& active() noexcept {
    static thread_local DeferredDefaults* deferred = nullptr;
    return deferred;
  }

 private:
  const unsigned char* begin_;
  const unsigned char* end_;
  std::vector<Record> records_;
};

}  // namespace rfl::internal

#endif
