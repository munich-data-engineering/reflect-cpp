module;

#ifdef REFLECTCPP_FLEXBUFFERS
#include "rfl/flexbuf.hpp"
#endif

export module rfl:flexbuf;

export namespace rfl::flexbuf {
    #ifdef REFLECTCPP_FLEXBUFFERS
    using rfl::flexbuf::Parser;
    using rfl::flexbuf::Reader;
    using rfl::flexbuf::Writer;

    using rfl::flexbuf::load;
    using rfl::flexbuf::read;
    using rfl::flexbuf::save;
    using rfl::flexbuf::to_buffer;
    using rfl::flexbuf::write;
    #endif
}
