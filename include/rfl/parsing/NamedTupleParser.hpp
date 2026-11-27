#ifndef RFL_PARSING_NAMEDTUPLEPARSER_HPP_
#define RFL_PARSING_NAMEDTUPLEPARSER_HPP_

#include <array>
#include <exception>
#include <map>
#include <sstream>
#include <type_traits>
#include <utility>

#include "../NamedTuple.hpp"
#include "../Result.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/default_if_missing_v.hpp"
#include "../internal/has_default_val_v.hpp"
#include "../internal/has_reflection_method_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/is_attribute.hpp"
#include "../internal/is_default_val_v.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_skip.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "../internal/nth_element_t.hpp"
#include "../internal/ptr_cast.hpp"
#include "../num_fields.hpp"
#include "../to_view.hpp"
#include "AreReaderAndWriter.hpp"
#include "FieldErrors.hpp"
#include "Parent.hpp"
#include "Parser_base.hpp"
#include "ViewReader.hpp"
#include "ViewReaderWithDefault.hpp"
#include "ViewReaderWithDefaultAndStrippedFieldNames.hpp"
#include "ViewReaderWithStrippedFieldNames.hpp"
#include "call_destructors_where_necessary.hpp"
#include "is_empty.hpp"
#include "is_required.hpp"
#include "schema/Type.hpp"
#include "to_single_error_message.hpp"
#include "wrap_error.hpp"

namespace rfl {

/// forward declaration
template <class... Ps>
Generic to_generic(const auto& _t);

namespace internal {

/// Satisfied by a type that carries a nested `ReflectionType` and can be
/// unwrapped by decomposing it, like `rfl::Validator`.
template <class T>
concept IsReflectionTypeWrapper =
    // We use `std::is_aggregate_v` and `num_fields` rather than `requires`
    // because `requires`, well, requires an expression, but checking this via
    // syntax would require a statement `const auto& [_] = _t`, which is not
    // allowed in a `requires` clause.
    has_reflection_type_v<T> && !has_reflection_method_v<T> &&
    std::is_aggregate_v<T> && num_fields<T> == 1;

/// Satisfied when the generic writer can serialize `T`, i.e. when a schema
/// `default` can be produced for it. Every type is writable, except those that
/// can be constructed from `ReflectionType`, but not converted back to it.
template <class T>
concept CanWriteGeneric =
    !has_reflection_type_v<std::remove_cvref_t<T>> ||
    has_reflection_method_v<std::remove_cvref_t<T>> ||
    IsReflectionTypeWrapper<std::remove_cvref_t<T>>;

/// Satisfied when a schema `default` can be emitted for the field type `T`:
/// it must be a `DefaultVal` that satisfies `CanWriteGeneric`.
template <class T>
concept CanEmitDefault =
    is_default_val_v<T> && CanWriteGeneric<typename T::Type>;

}  // namespace internal
}  // namespace rfl

namespace rfl::parsing {

template <class R, class W, bool _ignore_empty_containers, bool _all_required,
          bool _no_field_names, class ProcessorsType, class... FieldTypes>
  requires AreReaderAndWriter<R, W, NamedTuple<FieldTypes...>>
struct NamedTupleParser {
  using InputVarType = typename R::InputVarType;

  using ParentType = Parent<W>;

  using NamedTupleType = NamedTuple<FieldTypes...>;

  using ViewReaderType = std::conditional_t<
      _no_field_names,
      ViewReaderWithStrippedFieldNames<R, W, NamedTupleType, ProcessorsType>,
      ViewReader<R, W, NamedTupleType, ProcessorsType>>;

  using ViewReaderWithDefaultType = std::conditional_t<
      _no_field_names,
      ViewReaderWithDefaultAndStrippedFieldNames<R, W, NamedTupleType,
                                                 ProcessorsType>,
      ViewReaderWithDefault<R, W, NamedTupleType, ProcessorsType>>;

  using InputObjectOrArrayType =
      std::conditional_t<_no_field_names, typename R::InputArrayType,
                         typename R::InputObjectType>;
  using OutputObjectOrArrayType =
      std::conditional_t<_no_field_names, typename W::OutputArrayType,
                         typename W::OutputObjectType>;

  using SchemaType = std::conditional_t<_no_field_names, schema::Type::Tuple,
                                        schema::Type::Object>;

  static constexpr size_t size_ = NamedTupleType::size();

  static_assert(NamedTupleType::pos_extra_fields() == -1 || !_no_field_names,
                "You cannot use the rfl::NoFieldNames processor if you are "
                "including rfl::ExtraFields.");

 public:
  /**
   * @brief Reads a named tuple from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed named tuple or an error.
   */
  static Result<NamedTuple<FieldTypes...>> read(
      const R& _r, const InputVarType& _var) noexcept {
    alignas(NamedTuple<FieldTypes...>) unsigned char
        buf[sizeof(NamedTuple<FieldTypes...>)];
    auto ptr = internal::ptr_cast<NamedTuple<FieldTypes...>*>(&buf);
    auto view = rfl::to_view(*ptr);
    using ViewType = std::remove_cvref_t<decltype(view)>;
    // A NamedTuple has no in-class initializers, so a missing bare
    // rfl::DefaultVal field is value-initialized, which is what DefaultVal<T>
    // means without an initializer.
    auto [set, err] = Parser<R, W, ViewType, ProcessorsType>::read_view(
        _r, _var, &view, /*_value_initialize_default_vals=*/true);
    if (err) [[unlikely]] {
      call_destructors_where_necessary(set, &view);
      return error(std::move(*err));
    }
    auto res = Result<NamedTuple<FieldTypes...>>(std::move(*ptr));
    call_destructors_where_necessary(set, &view);
    return res;
  }

  /**
   * @brief Reads the data into a view assuming no default values.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @param _view The view to read into.
   * @param _value_initialize_default_vals Whether a missing rfl::DefaultVal
   *         field is value-initialized. Otherwise it is required, because
   *         the default value in the struct's in-class initializer is not
   *         available.
   * @return A pair containing a boolean array indicating which fields were
   *         found and an optional error.
   */
  static RFL_ALWAYS_INLINE
      std::pair<std::array<bool, NamedTupleType::size()>, std::optional<Error>>
      read_view(const R& _r, const InputVarType& _var,
                NamedTuple<FieldTypes...>* _view,
                const bool _value_initialize_default_vals = false) noexcept {
    if constexpr (_no_field_names) {
      auto arr = _r.to_array(_var);
      if (!arr) [[unlikely]] {
        auto set = std::array<bool, NamedTupleType::size()>{};
        return std::make_pair(set, arr.error());
      }
      return read_object_or_array(_r, *arr, _view,
                                  _value_initialize_default_vals);
    } else {
      auto obj = _r.to_object(_var);
      if (!obj) [[unlikely]] {
        auto set = std::array<bool, NamedTupleType::size()>{};
        return std::make_pair(set, obj.error());
      }
      return read_object_or_array(_r, *obj, _view,
                                  _value_initialize_default_vals);
    }
  }

  /**
   * @brief Reads the data into a view assuming default values.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @param _view The view to read into.
   * @return An optional error.
   */
  static std::optional<Error> read_view_with_default(
      const R& _r, const InputVarType& _var,
      NamedTuple<FieldTypes...>* _view) noexcept {
    if constexpr (_no_field_names) {
      auto arr = _r.to_array(_var);
      if (!arr) [[unlikely]] {
        return arr.error();
      }
      return read_object_or_array_with_default(_r, *arr, _view);
    } else {
      auto obj = _r.to_object(_var);
      if (!obj) [[unlikely]] {
        return obj.error();
      }
      return read_object_or_array_with_default(_r, *obj, _view);
    }
  }

  /**
   * @brief Writes a named tuple to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _tup The named tuple to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const NamedTuple<FieldTypes...>& _tup,
                    const P& _parent) {
    if constexpr (_no_field_names) {
      auto arr = ParentType::add_array(_w, _tup.num_fields(), _parent);
      build_object(_w, _tup, &arr, std::make_integer_sequence<int, size_>());
      _w.end_array(&arr);
    } else {
      auto obj = ParentType::add_object(_w, _tup.num_fields(), _parent);
      build_object(_w, _tup, &obj, std::make_integer_sequence<int, size_>());
      _w.end_object(&obj);
    }
  }

  /**
   * @brief Generates the schema for the named tuple.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  template <typename View = void>
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions,
      View* _view = nullptr) noexcept {
    SchemaType schema;
    build_schema(_definitions, &schema, _view,
                 std::make_integer_sequence<int, size_>());
    return schema::Type{schema};
  }

 private:
  template <int _i>
  static void add_field_to_object(const W& _w,
                                  const NamedTuple<FieldTypes...>& _tup,
                                  OutputObjectOrArrayType* _ptr) {
    using FieldType = internal::nth_element_t<_i, FieldTypes...>;
    using ValueType = std::remove_cvref_t<typename FieldType::Type>;
    const auto value = rfl::get<_i>(_tup);
    if constexpr (internal::is_extra_fields_v<ValueType>) {
      for (const auto& [k, v] : *value) {
        const auto new_parent = make_parent(k, _ptr);
        Parser<R, W, std::remove_cvref_t<decltype(v)>, ProcessorsType>::write(
            _w, v, new_parent);
      }
    } else if constexpr (!_all_required && !_no_field_names &&
                         !is_required<ValueType, _ignore_empty_containers>()) {
      constexpr auto name = FieldType::name_.string_view();
      const auto new_parent = make_parent(name, _ptr);
      if (!is_empty(value)) {
        if constexpr (internal::is_attribute_v<ValueType>) {
          Parser<R, W, ValueType, ProcessorsType>::write(
              _w, value, new_parent.as_attribute());
        } else {
          Parser<R, W, ValueType, ProcessorsType>::write(_w, value, new_parent);
        }
      }
    } else {
      constexpr auto name = FieldType::name_.string_view();
      const auto new_parent = make_parent(name, _ptr);
      if constexpr (internal::is_attribute_v<ValueType>) {
        Parser<R, W, ValueType, ProcessorsType>::write(
            _w, value, new_parent.as_attribute());
      } else {
        Parser<R, W, ValueType, ProcessorsType>::write(_w, value, new_parent);
      }
    }
  }

  template <typename View, size_t _i>
  static void add_field_to_schema(
      std::map<std::string, schema::Type>* _definitions,
      SchemaType* _schema,
      View* _view) noexcept {
    using F = internal::nth_element_t<_i, FieldTypes...>;
    using U = std::remove_cvref_t<typename F::Type>;
    if constexpr (!internal::is_skip_v<U> && !internal::is_extra_fields_v<U>) {
      // Add default value here
      auto s = Parser<R, W, U, ProcessorsType>::to_schema(_definitions);
      if constexpr (!std::is_same_v<View, void> && internal::CanEmitDefault<U>) {
        s.variant_.visit([&](auto& value) {
          if constexpr (std::is_same_v<std::remove_cvref_t<decltype(value)>,
                                       schema::Type::DefaultVal>) {
            value.default_value_ =
                rfl::to_generic<ProcessorsType>((*rfl::get<_i>(*_view)).get());
          }
        });
      }
      if constexpr (_no_field_names) {
        _schema->types_.emplace_back(std::move(s));
      } else {
        _schema->types_[std::string(F::name())] = std::move(s);
      }
    }
  }

  template <int... _is>
  static void build_object(const W& _w, const NamedTuple<FieldTypes...>& _tup,
                           OutputObjectOrArrayType* _ptr,
                           std::integer_sequence<int, _is...>) {
    (add_field_to_object<_is>(_w, _tup, _ptr), ...);
  }

  template <typename View,  int... _is>
  static void build_schema(std::map<std::string, schema::Type>* _definitions,
                           SchemaType* _schema,
                           View* _view,
                           std::integer_sequence<int, _is...>) noexcept {
    (add_field_to_schema<View, _is>(_definitions, _schema, _view), ...);

    if constexpr (NamedTupleType::pos_extra_fields() != -1) {
      using F = internal::nth_element_t<NamedTupleType::pos_extra_fields(),
                                        FieldTypes...>;
      using ExtraFieldsType = std::remove_cvref_t<typename F::Type>;
      using U = std::remove_cvref_t<typename ExtraFieldsType::Type>;
      _schema->additional_properties_ = std::make_shared<schema::Type>(
          Parser<R, W, U, ProcessorsType>::to_schema(_definitions));
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType> &&
                         !_no_field_names) {
      _schema->additional_properties_ = false;
    }
  }

  /// Generates error messages for when fields are missing.
  template <int _i>
  static RFL_ALWAYS_INLINE void handle_one_missing_field(
      const std::array<bool, size_>& _found, const NamedTupleType& _view,
      std::array<bool, size_>* _set, FieldErrors<ProcessorsType>* _errors,
      const bool _value_initialize_default_vals) noexcept {
    if (should_stop_on_field_error<ProcessorsType>(*_errors)) return;
    using FieldType = internal::nth_element_t<_i, FieldTypes...>;
    using ValueType = std::remove_reference_t<
        std::remove_pointer_t<typename FieldType::Type>>;

    if (!std::get<_i>(_found)) {
      constexpr bool is_default_val_field =
          internal::is_default_val_v<ValueType>;
      constexpr bool is_required_field =
          !is_default_val_field && !internal::is_extra_fields_v<ValueType> &&
          (_all_required || is_required<ValueType, _ignore_empty_containers>());

      // Without _set, the struct was default-constructed before it was read,
      // so the field already holds its default value. With _set, the fields
      // are constructed in raw memory. An rfl::NamedTuple has no in-class
      // initializers, so a missing bare rfl::DefaultVal field is
      // value-initialized there. Otherwise the field's default value is not
      // available, so the field is required.
      const bool defaults_constructed = _set == nullptr;

      if constexpr (is_default_val_field) {
        if constexpr (can_value_initialize_default_val<ValueType>()) {
          if (_value_initialize_default_vals && !defaults_constructed) {
            // The value's constructor can throw, for instance a Validator
            // whose zero value is invalid.
            try {
              ::new (rfl::get<_i>(_view)) ValueType();
            } catch (std::exception& e) {
              constexpr auto current_name =
                  internal::nth_element_t<_i, FieldTypes...>::name();
              _errors->emplace_back(make_nested_error(
                  std::string(current_name),
                  "Field named '" + std::string(current_name) +
                      "' not found, and its default value could not be "
                      "constructed: " +
                      e.what()));
              return;
            }
            std::get<_i>(*_set) = true;
            return;
          }
        }
      }

      if (is_required_field ||
          (is_default_val_field && !defaults_constructed)) {
        constexpr auto current_name =
            internal::nth_element_t<_i, FieldTypes...>::name();
        std::stringstream stream;
        stream << "Field named '" << std::string(current_name)
               << "' not found.";
        _errors->emplace_back(
            make_nested_error(std::string(current_name), stream.str()));
        return;
      }

      if constexpr (!is_required_field && !is_default_val_field) {
        if (defaults_constructed) {
          return;
        }
        using NonConstT = std::remove_const_t<ValueType>;
        const auto ptr = const_cast<NonConstT*>(rfl::get<_i>(_view));
        if constexpr (std::is_nothrow_default_constructible_v<NonConstT>) {
          ::new (ptr) NonConstT();
        } else {
          // The default value can throw, for instance a Validator whose
          // default value is invalid.
          try {
            ::new (ptr) NonConstT();
          } catch (std::exception& e) {
            constexpr auto current_name =
                internal::nth_element_t<_i, FieldTypes...>::name();
            _errors->emplace_back(make_nested_error(
                std::string(current_name),
                "Field named '" + std::string(current_name) +
                    "' not found, and its default value could not be "
                    "constructed: " +
                    e.what()));
            return;
          }
        }
        std::get<_i>(*_set) = true;
      }
    }
  }

  /// Whether a missing rfl::DefaultVal field can be value-initialized. Only
  /// an rfl::NamedTuple does this at runtime, and it cannot have const fields,
  /// so const fields of structs need no instantiation.
  template <class ValueType>
  static constexpr bool can_value_initialize_default_val() {
    using T = typename internal::default_val_t<ValueType>::Type;
    return !std::is_const_v<ValueType> && std::is_default_constructible_v<T> &&
           std::is_default_constructible_v<ValueType>;
  }

  /// Generates error messages for when fields are missing.
  template <int... _is>
  static RFL_ALWAYS_INLINE void handle_missing_fields(
      const std::array<bool, size_>& _found, const NamedTupleType& _view,
      std::array<bool, size_>* _set, FieldErrors<ProcessorsType>* _errors,
      const bool _value_initialize_default_vals,
      std::integer_sequence<int, _is...>) noexcept {
    (handle_one_missing_field<_is>(_found, _view, _set, _errors,
                                   _value_initialize_default_vals),
     ...);
  }

  static auto make_parent(const std::string_view& _name,
                          OutputObjectOrArrayType* _ptr) {
    if constexpr (_no_field_names) {
      return typename ParentType::Array{_ptr};
    } else {
      return typename ParentType::Object{_name, _ptr};
    }
  }

  static RFL_ALWAYS_INLINE
      std::pair<std::array<bool, NamedTupleType::size()>, std::optional<Error>>
      read_object_or_array(const R& _r,
                           const InputObjectOrArrayType& _obj_or_arr,
                           NamedTupleType* _view,
                           const bool _value_initialize_default_vals) noexcept {
    auto found = std::array<bool, NamedTupleType::size()>();
    found.fill(false);
    auto set = std::array<bool, NamedTupleType::size()>();
    set.fill(false);
    FieldErrors<ProcessorsType> errors;
    const auto reader = ViewReaderType(&_r, _view, &found, &set, &errors);
    if constexpr (_no_field_names) {
      auto err = _r.read_array(reader, _obj_or_arr);
      if (err) [[unlikely]] {
        return std::make_pair(set, std::move(err));
      }
    } else {
      auto err = _r.read_object(reader, _obj_or_arr);
      if (err) [[unlikely]] {
        return std::make_pair(set, std::move(err));
      }
    }
    if (!should_stop_on_field_error<ProcessorsType>(errors)) {
      handle_missing_fields(found, *_view, &set, &errors,
                            _value_initialize_default_vals,
                            std::make_integer_sequence<int, size_>());
    }
    if constexpr (internal::fail_fast_v<ProcessorsType>) {
      return std::make_pair(set, std::move(errors).error());
    } else {
      if (errors.size() != 0) [[unlikely]] {
        return std::make_pair(set, to_single_error(std::move(errors)));
      }
      return std::make_pair(set, std::optional<Error>());
    }
  }

  static std::optional<Error> read_object_or_array_with_default(
      const R& _r, const InputObjectOrArrayType& _obj_or_arr,
      NamedTupleType* _view) noexcept {
    FieldErrors<ProcessorsType> errors;
    const auto reader = ViewReaderWithDefaultType(&_r, _view, &errors);
    if constexpr (_no_field_names) {
      auto err = _r.read_array(reader, _obj_or_arr);
      if (err) [[unlikely]] {
        return err;
      }
    } else {
      auto err = _r.read_object(reader, _obj_or_arr);
      if (err) [[unlikely]] {
        return err;
      }
    }
    if constexpr (internal::has_default_val_v<NamedTupleType> &&
                  !internal::default_if_missing_v<ProcessorsType>) {
      if (!should_stop_on_field_error<ProcessorsType>(errors)) {
        handle_missing_fields(reader.found(), *_view, nullptr, &errors,
                              /*_value_initialize_default_vals=*/false,
                              std::make_integer_sequence<int, size_>());
      }
    }
    if constexpr (internal::fail_fast_v<ProcessorsType>) {
      return std::move(errors).error();
    } else {
      if (errors.size() != 0) [[unlikely]] {
        return to_single_error(std::move(errors));
      }
      return std::nullopt;
    }
  }
};

}  // namespace rfl::parsing

#endif
