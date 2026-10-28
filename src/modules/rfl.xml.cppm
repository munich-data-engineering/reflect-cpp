module;

#ifdef REFLECTCPP_XML
#include "rfl/xml.hpp"
#endif

export module rfl:xml;

export namespace rfl::xml {
    #ifdef REFLECTCPP_XML
    using rfl::xml::Parser;
    using rfl::xml::Reader;
    using rfl::xml::Writer;
    using rfl::xml::InputVarType;

    using rfl::xml::load;
    using rfl::xml::read;
    using rfl::xml::save;
    using rfl::xml::get_root_name;
    using rfl::xml::write;
    #endif
}
