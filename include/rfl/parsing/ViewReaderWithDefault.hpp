#ifndef RFL_PARSING_VIEWREADERWITHDEFAULT_HPP_
#define RFL_PARSING_VIEWREADERWITHDEFAULT_HPP_

#include <array>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Ref.hpp"
#include "../Tuple.hpp"
#include "../internal/is_array.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "FieldErrors.hpp"
#include "Parser_base.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

template <class R, class W, class _ViewType, class ProcessorsType>
class ViewReaderWithDefault {
 public:
  using InputVarType = typename R::InputVarType;
  using ViewType = _ViewType;

  static constexpr size_t size_ = ViewType::size();

  /**
   * @brief Constructor.
   *
   * @param _r The reader to use.
   * @param _view The view to read into.
   * @param _errors The field error collector.
   */
  ViewReaderWithDefault(const R* _r, ViewType* _view,
                        FieldErrors<ProcessorsType>* _errors)
      : r_(_r), view_(_view), errors_(_errors) {
    found_->fill(false);
  }

  ~ViewReaderWithDefault() = default;

  /**
   * @brief Returns a boolean array indicating which fields were found.
   *
   * @return The boolean array.
   */
  const std::array<bool, size_>& found() const { return *found_; }

  /**
   * @brief Reads a single field into the view.
   *
   * @param _name The name of the field.
   * @param _var The input variable to read from.
   */
  void read(const std::string_view& _name, const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return;
    assign_to_matching_field(*r_, _name, _var, view_, errors_, found_.get(),
                             std::make_integer_sequence<int, size_>());
  }

  /**
   * @brief Returns the size of the view.
   *
   * @return The size of the view.
   */
  static constexpr size_t size() { return size_; }

 private:
  template <int i>
  static void assign_if_field_matches(const R& _r,
                                      const std::string_view& _current_name,
                                      const auto& _var, auto* _view,
                                      auto* _errors, auto* _found,
                                      bool* _already_assigned) {
    using FieldType = tuple_element_t<i, typename ViewType::Fields>;
    using OriginalType = typename FieldType::Type;
    using T =
        std::remove_cvref_t<std::remove_pointer_t<typename FieldType::Type>>;
    constexpr auto name = FieldType::name();
    if (!(*_already_assigned) && !std::get<i>(*_found) &&
        _current_name == name) {
      std::get<i>(*_found) = true;
      *_already_assigned = true;
      auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
      if (!res) {
        _errors->emplace_back(wrap_field_error(name, std::move(res).error()));
        return;
      }
      if constexpr (std::is_pointer_v<OriginalType>) {
        move_to(rfl::get<i>(*_view), &(*res));
      } else {
        rfl::get<i>(*_view) = std::move(*res);
      }
    }
  }

  template <int _pos>
  static void assign_to_extra_fields(const R& _r,
                                     const std::string_view& _current_name,
                                     const auto& _var, auto* _view,
                                     auto* _errors) {
    auto* extra_fields = _view->template get<_pos>();
    using ExtraFieldsType =
        std::remove_cvref_t<std::remove_pointer_t<decltype(extra_fields)>>;
    using T = std::remove_cvref_t<
        std::remove_pointer_t<typename ExtraFieldsType::Type>>;
    auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
    if (!res) {
      _errors->emplace_back(
          wrap_field_error(_current_name, std::move(res).error()));
      return;
    }
    extra_fields->emplace(std::string(_current_name), std::move(*res));
  }

  template <int... is>
  static void assign_to_matching_field(const R& _r,
                                       const std::string_view& _current_name,
                                       const auto& _var, auto* _view,
                                       auto* _errors, auto* _found,
                                       std::integer_sequence<int, is...>) {
    bool already_assigned = false;

    (assign_if_field_matches<is>(_r, _current_name, _var, _view, _errors,
                                 _found, &already_assigned),
     ...);

    if constexpr (ViewType::pos_extra_fields() != -1) {
      constexpr int pos = ViewType::pos_extra_fields();
      if (!already_assigned) {
        assign_to_extra_fields<pos>(_r, _current_name, _var, _view, _errors);
      }
    } else if constexpr (internal::no_extra_fields_v<ProcessorsType>) {
      if (!already_assigned) {
        std::stringstream stream;
        stream << "Value named '" << std::string(_current_name)
               << "' not used. Remove the rfl::NoExtraFields processor or add "
                  "rfl::ExtraFields to avoid this error message.";
        _errors->emplace_back(
            make_nested_error(std::string(_current_name), stream.str()));
      }
    }
  }

  template <class Target, class Source>
  static void move_to(Target* _t, Source* _s) {
    if constexpr (std::is_const_v<Target>) {
      return move_to(const_cast<std::remove_const_t<Target>*>(_t), _s);
    } else if constexpr (!rfl::internal::is_array_v<Source> &&
                         !std::is_array_v<Target>) {
      *_t = Target(std::move(*_s));
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
  rfl::Ref<std::array<bool, size_>> found_;

  /// Stores field errors according to the selected processor policy.
  FieldErrors<ProcessorsType>* errors_;
};

}  // namespace rfl::parsing

#endif
