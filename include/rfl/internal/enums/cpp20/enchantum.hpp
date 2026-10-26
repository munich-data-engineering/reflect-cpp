#ifndef RFL_INTERNAL_ENUMS_CPP20_ENCHANTUM_HPP_
#define RFL_INTERNAL_ENUMS_CPP20_ENCHANTUM_HPP_

#ifndef REFLECTCPP_USE_CPP26_REFLECTION

// Enum values must be greater than or equal to RFL_ENUM_RANGE_MIN.
// By default, RFL_ENUM_RANGE_MIN is set to -256.
// To change the default minimum range for all enum types, redefine the macro
// RFL_ENUM_RANGE_MIN.
#if !defined(RFL_ENUM_RANGE_MIN)
#define RFL_ENUM_RANGE_MIN -256
#endif

// Enum values must be less than or equal to RFL_ENUM_RANGE_MAX.
// By default, RFL_ENUM_RANGE_MAX is set to 256.
// To change the default maximum range for all enum types, redefine the macro
// RFL_ENUM_RANGE_MAX.
#if !defined(RFL_ENUM_RANGE_MAX)
#define RFL_ENUM_RANGE_MAX 256
#endif

#ifdef ENCHANTUM_MIN_RANGE
#undef ENCHANTUM_MIN_RANGE
#endif
#define ENCHANTUM_MIN_RANGE RFL_ENUM_RANGE_MIN

#ifdef ENCHANTUM_MAX_RANGE
#undef ENCHANTUM_MAX_RANGE
#endif
#define ENCHANTUM_MAX_RANGE RFL_ENUM_RANGE_MAX

#if __has_include(<enchantum/enchantum.hpp>)
#include <enchantum/enchantum.hpp>
#else
#include "../../../thirdparty/enchantum/enchantum.hpp"
#endif

#endif
#endif
