add_library(
        decoder
        Decoder.h
        Decoder.cpp
)

find_package(Boost 1.81 COMPONENTS md5 hex)
target_link_libraries(decoder ${Boost_LIBRARIES})