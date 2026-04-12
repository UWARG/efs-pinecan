cmake_minimum_required(VERSION 3.22)

add_custom_target(
    build_external
    COMMAND make # The command to run (e.g., 'make all' or 'mingw32-make' on Windows)
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} # The directory containing the Makefile
    COMMENT "Building the external library via Makefile" # Optional message
    # Add the ALL option if you want this target to run by default with your main build
    # ALL 
)

# dsdl as a static lib
add_library(dsdl STATIC)

file(GLOB DSDL_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/generated/src/*.c
)

target_sources(dsdl PRIVATE
    ${DSDL_SOURCES}
)

target_include_directories(dsdl SYSTEM PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/generated/include
)

target_link_libraries(dsdl PRIVATE libcanard)

# Validate that dsdl code is compatible with C standard
if(CMAKE_C_STANDARD LESS 11)
    message(ERROR "Generated code requires C11 or higher")
endif()
