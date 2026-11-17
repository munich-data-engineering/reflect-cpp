#include "rfl/internal/DeferredDefaults.hpp"

namespace rfl::internal {

DeferredDefaults*& DeferredDefaults::active() noexcept {
  static thread_local DeferredDefaults* deferred = nullptr;
  return deferred;
}

}  // namespace rfl::internal
