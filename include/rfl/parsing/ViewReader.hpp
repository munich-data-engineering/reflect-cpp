#ifndef RFL_PARSING_VIEWREADER_HPP_
#define RFL_PARSING_VIEWREADER_HPP_

#include <array>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Tuple.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/is_array.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "FieldErrors.hpp"
#include "Parser_base.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

template <class R, class W, class _ViewType, class ProcessorsType>
class ViewReader {
 public:
  using InputVarType = typename R::InputVarType;
  using ViewType = _ViewType;

  static constexpr size_t size_ = ViewType::size();

  /**
   * @brief Constructor.
   *
   * @param _r The reader to use.
   * @param _view The view to read into.
   * @param _found A boolean array indicating which fields have been found.
   * @param _set A boolean array indicating which fields have been successfully
   * set.
   * @param _errors The field error collector.
   */
  ViewReader(const R* _r, ViewType* _view, std::array<bool, size_>* _found,
             std::array<bool, size_>* _set, FieldErrors<ProcessorsType>* _errors)
      : r_(_r), view_(_view), found_(_found), set_(_set), errors_(_errors) {}

  ~ViewReader() = default;

  /**
   * @brief Reads a single field into the view.
   *
   * @param _name The name of the field.
   * @param _var The input variable to read from.
   */
  void read(const std::string_view& _name, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    assign_to_matching_field(*r_, _name, _var, view_, errors_, found_, set_,
                             std::make_integer_sequence<int, size_>());
  }

  /**
   * @brief Reads a single field into the view.
   *
   * @param _index The index of the field.
   * @param _var The input variable to read from.
   */
  void read(const int _index, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    assign_to_matching_field(*r_, _index, _var, view_, errors_, found_, set_,
                             std::make_integer_sequence<int, size_>());
  }

  // Positional readers already know the field at compile time.
  template <int i>
  void read(std::integral_constant<int, i>, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    assign_to_matching_field(*r_, i, _var, view_, errors_, found_, set_,
                             std::integer_sequence<int, i>());
  }

  /**
   * @brief Returns the size of the view.
   *
   * @return The size of the view.
   */
  static constexpr size_t size() { return size_; }

 private:
  template <int i, class FieldType>
  static bool is_matching(const int _current_index) {
    return _current_index == i;
  }

  template <int i, class FieldType>
  static bool is_matching(const std::string_view& _current_name) {
    return _current_name == FieldType::name();
  }

  static ErrorDetail::PathSegment to_path_segment(const int _current_index) {
    return static_cast<size_t>(_current_index);
  }

  static ErrorDetail::PathSegment to_path_segment(
      const std::string_view& _current_name) {
    return std::string(_current_name);
  }

  template <int i>
  static RFL_ALWAYS_INLINE void assign_if_field_matches(
      const R& _r, const auto _current_name_or_index, const auto& _var,
      auto* _view, auto* _errors, auto* _found, auto* _set,
      bool* _already_assigned) {
    using FieldType = tuple_element_t<i, typename ViewType::Fields>;
    using OriginalType = typename FieldType::Type;
    using T =
        std::remove_cvref_t<std::remove_pointer_t<typename FieldType::Type>>;
    if (!(*_already_assigned) && !std::get<i>(*_found) &&
        is_matching<i, FieldType>(_current_name_or_index)) {
      std::get<i>(*_found) = true;
      *_already_assigned = true;
      auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
      if (!res) [[unlikely]] {
        constexpr auto name = FieldType::name();
        // Reserve lazily to avoid error-vector reallocations while keeping
        // successful reads allocation-free.
        _errors->reserve(size_);
        _errors->emplace_back(wrap_field_error(name, std::move(res).error()));
        return;
      }
      if constexpr (std::is_pointer_v<OriginalType>) {
        move_to(rfl::get<i>(*_view), &(*res));
      } else {
        rfl::get<i>(*_view) = std::move(*res);
      }
      std::get<i>(*_set) = true;
    }
  }

  template <int _pos>
  static void assign_to_extra_fields(const R& _r,
                                     const std::string_view& _current_name,
                                     const auto& _var, auto* _view,
                                     auto* _errors, auto* _found, auto* _set) {
    auto* extra_fields = _view->template get<_pos>();
    using ExtraFieldsType =
        std::remove_cvref_t<std::remove_pointer_t<decltype(extra_fields)>>;
    using T = std::remove_cvref_t<
        std::remove_pointer_t<typename ExtraFieldsType::Type>>;
    if (!std::get<_pos>(*_set)) {
      ::new (extra_fields) ExtraFieldsType();
      std::get<_pos>(*_set) = true;
      std::get<_pos>(*_found) = true;
    }
    auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
    if (!res) [[unlikely]] {
      _errors->emplace_back(
          wrap_field_error(_current_name, std::move(res).error()));
      return;
    }
    extra_fields->emplace(std::string(_current_name), std::move(*res));
  }

  template <int... is>
  static void assign_to_matching_field(const R& _r,
                                       const auto _current_name_or_index,
                                       const auto& _var, auto* _view,
                                       auto* _errors, auto* _found, auto* _set,
                                       std::integer_sequence<int, is...>) {
    bool already_assigned = false;

    (assign_if_field_matches<is>(_r, _current_name_or_index, _var, _view,
                                 _errors, _found, _set, &already_assigned),
     ...);

    if constexpr (ViewType::pos_extra_fields() != -1) {
      static_assert(!schemaful::IsSchemafulReader<R>,
                    "rfl::ExtraFields are not supported for schemaful formats, "
                    "because schemaful formats cannot have extra fields.");
      constexpr int pos = ViewType::pos_extra_fields();
      if (!already_assigned) {
        assign_to_extra_fields<pos>(_r, _current_name_or_index, _var, _view,
                                    _errors, _found, _set);
      }
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      static_assert(
          !schemaful::IsSchemafulReader<R>,
          "Passing rfl::NoExtraFields to a schemaful format does not make "
          "sense, because schemaful formats cannot have extra fields.");
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << _current_name_or_index << "' not used.";
        _errors->emplace_back(make_nested_error(
            to_path_segment(_current_name_or_index), stream.str()));
      }
    }
  }

  template <class Target, class Source>
  static void move_to(Target* _t, Source* _s) {
    if constexpr (std::is_const_v<Target>) {
      return move_to(const_cast<std::remove_const_t<Target>*>(_t), _s);
    } else if constexpr (!rfl::internal::is_array_v<Source> &&
                         !std::is_array_v<Target>) {
      ::new (_t) Target(std::move(*_s));
    } else if constexpr (rfl::internal::is_array_v<Source>) {
      static_assert(std::is_array_v<Target>,
                    "Expected target to be a c-array.");
      for (size_t i = 0; i < _s->arr_.size(); ++i) {
        move_to(&((*_t)[i]), &(_s->arr_[i]));
      }
    } else {
      for (size_t i = 0; i < _s->size(); ++i) {
        move_to(&((*_t)[i]), &((*_s)[i]));
      }
    }
  }

 private:
  /// The underlying reader.
  const R* r_;

  /// The underlying view.
  ViewType* view_;

  /// Indicates that a certain field has been found.
  std::array<bool, size_>* found_;

  /// Indicates that a certain field has been successfully set - necessary,
  /// because we have to trigger the destructors manually.
  std::array<bool, size_>* set_;

  /// Stores field errors according to the selected processor policy.
  FieldErrors<ProcessorsType>* errors_;
};

}  // namespace rfl::parsing

#endif
