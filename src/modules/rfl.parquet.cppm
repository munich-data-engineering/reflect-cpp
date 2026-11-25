module;

#ifdef REFLECTCPP_PARQUET
#include "rfl/parquet.hpp"
#endif

export module rfl:parquet;

export namespace rfl::parquet {
    #ifdef REFLECTCPP_PARQUET
    using rfl::parquet::Compression;
    using rfl::parquet::Settings;
    
    using rfl::parquet::load;
    using rfl::parquet::read;
    using rfl::parquet::save;
    using rfl::parquet::to_buffer;
    using rfl::parquet::write;
    #endif
}
