cmake_minimum_required(VERSION 2.8.3)

project(moses_tokenizer)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I /usr/local/opt/icu4c/include")
endif ()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Ofast")
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

find_package(Boost 1.71 COMPONENTS program_options REQUIRED)

# compile executable into bin/
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# compile libraries into lib/
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# define where include files are
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
)

add_library(tokenizer_lib
    ${COMPILED_RESOURCES}
    src/tokenizer.cc
    src/regex.cc
    src/nonbreaking_prefix_set.cc
    data/perluniprops/IsN.txt.hex
    data/perluniprops/IsAlpha.txt.hex
    data/perluniprops/IsAlnum.txt.hex
    data/perluniprops/IsLower.txt.hex
    data/perluniprops/Lowercase_Letter.txt.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.ca.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.cs.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.de.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.el.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.en.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.es.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.fi.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.fr.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.ga.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.hu.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.is.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.it.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.lt.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.lv.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.nl.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.pl.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.pt.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.ro.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.ru.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.sk.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.sl.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.sv.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.ta.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.yue.hex
    data/nonbreaking_prefixes/nonbreaking_prefix.zh.hex
)

if (PREPROCESS_PATH)
    add_subdirectory(${PREPROCESS_PATH} preprocess)
    target_include_directories(tokenizer PUBLIC ${PREPROCESS_PATH})
else ()
    add_subdirectory(${CMAKE_SOURCE_DIR}/preprocess)
    target_include_directories(tokenizer PUBLIC ${CMAKE_SOURCE_DIR}/preprocess)
endif ()

find_program(RESOURCE_COMPILER xxd)
file(GLOB_RECURSE COMPILE_RESOURCES "data/*")
foreach(INPUT_FILE ${COMPILE_RESOURCES})
    set(OUTPUT_FILE ${INPUT_FILE}.hex)
    add_custom_command(
        PRE_BUILD
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${RESOURCE_COMPILER} -i < ${INPUT_FILE} > ${OUTPUT_FILE}
        COMMENT "Compiling ${INPUT_FILE} to header file"
        DEPENDS ${INPUT_FILE})
    list(APPEND COMPILED_RESOURCES ${OUTPUT_FILE})
endforeach()

target_link_libraries(tokenizer_lib
    preprocess_util
    ${Boost_LIBRARIES}
)

# define executables
add_executable(tokenizer tokenizer_main.cc)
target_link_libraries(tokenizer
    tokenizer
    ${Boost_LIBRARIES}
)

