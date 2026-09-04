add_library(common STATIC
    src/Descriptor.cpp
    include/Descriptor.hpp
)

target_include_directories(common
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
