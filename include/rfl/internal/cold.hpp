#ifndef RFL_INTERNAL_COLD_HPP_
#define RFL_INTERNAL_COLD_HPP_

/// Marks a function that is rarely called, such as one that only runs when
/// something went wrong. Such functions are kept out of the code of their
/// callers, so that they do not slow down the common case.
#if defined(_MSC_VER)
#define RFL_COLD __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define RFL_COLD __attribute__((noinline, cold))
#else
#define RFL_COLD
#endif

#endif
