cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH C:/Users/seva-/Desktop/protobuf/Protobuf)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto svg.proto map_renderer.proto graph.proto transport_router.proto)

set(TRANSPORTCATALOGUE_FILES 
    main.cpp
    geo.cpp geo.h 
    json.cpp json.h
    json_reader.cpp json_reader.h
    json_builder.cpp json_builder.h
    svg.cpp svg.h
    map_renderer.cpp map_renderer.h
    transport_catalogue.cpp transport_catalogue.h
    serialization.cpp serialization.h
    transport_router.cpp transport_router.h
    router.h
    ranges.h
    graph.h
    domain.h)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${TRANSPORTCATALOGUE_FILES})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

#string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
#string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue ${Protobuf_LIBRARY} Threads::Threads)