cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# ======== Pre-build setup ========
# Default config to use from BuildConfigs.json. This can be overridden from command line.
# IMPORATANT: If you modify this value (or any CACHE value), you must do a clean re-configure of CMake
set(PINECAN_CONFIG_KEY "SSD_rev1"
    CACHE STRING "Key in BuildConfigs.json to use")

# Repo root path. Adjust path to match your layout.
get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

# Load BuildConfig.json to get config parameters
set(PINECAN_CONFIG_FILE "${ROOT_DIR}/BuildConfigs.json")
if(NOT EXISTS "${PINECAN_CONFIG_FILE}")
    message(FATAL_ERROR "Config file not found: ${PINECAN_CONFIG_FILE}")
endif()

file(READ "${PINECAN_CONFIG_FILE}" _configs_json)

# Ensure the requested key exists
string(JSON _has_key ERROR_VARIABLE _json_err
       GET "${_configs_json}" "${PINECAN_CONFIG_KEY}")

if(_json_err)
    message(FATAL_ERROR
        "Config key '${PINECAN_CONFIG_KEY}' not found in BuildConfigs.json")
endif()

string(JSON _cfg GET "${_configs_json}" "${PINECAN_CONFIG_KEY}")

# Extract board
string(JSON PINECAN_BOARD GET "${_cfg}" board)

# Get all keys under "symbols"
# Extract the 'symbols' object only
string(JSON _symbols_obj GET "${_cfg}" symbols)

# How many members does 'symbols' have?
string(JSON _sym_count LENGTH "${_symbols_obj}")
math(EXPR _sym_last "${_sym_count} - 1")

set(PINECAN_SYMBOL_DEFS "")

# Iterate members by index and get their names with MEMBER
foreach(_i RANGE 0 ${_sym_last})
    # Get the name of the _i-th member key
    string(JSON _key MEMBER "${_symbols_obj}" ${_i})

    # Value under that key
    string(JSON _val  GET  "${_symbols_obj}" "${_key}")
    string(JSON _type TYPE "${_symbols_obj}" "${_key}")

    # Decide quoting based on JSON type
    if(_type STREQUAL "STRING")
        # Strings need quotes in C preprocessor
        list(APPEND PINECAN_SYMBOL_DEFS "${_key}=\"${_val}\"")
    else()
        # NUMBER / BOOLEAN / NULL / OBJECT / ARRAY -> no quotes
        list(APPEND PINECAN_SYMBOL_DEFS "${_key}=${_val}")
    endif()
endforeach()

message("Using BuildConfig: ${PINECAN_CONFIG_KEY}")
message("  board: ${PINECAN_BOARD}")
message("  symbols: ${PINECAN_SYMBOL_DEFS}")


# ======= Build setup ========
# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME servo)


# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Add pinecan sources
add_subdirectory(${ROOT_DIR}/pinecan ${CMAKE_BINARY_DIR}/pinecan)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ./Core/Src/can/can.c
    ./Core/Src/servo/servo.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ./Core/Inc/can
    ./Core/Inc/servo
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Pinecan symbols from BuildConfig.json
    ${PINECAN_SYMBOL_DEFS}
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    # Pinecan libraries
    pinecan # Brings in libcanard + dsdl as well
    # Add user defined libraries
)
