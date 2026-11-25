module;

#ifdef REFLECTCPP_CSV
#include "rfl/csv.hpp"
#endif

export module rfl:csv;

export namespace rfl::csv {
    #ifdef REFLECTCPP_CSV
    using rfl::csv::Settings;

    using rfl::csv::load;
    using rfl::csv::read;
    using rfl::csv::save;
    using rfl::csv::write;
    #endif
}
