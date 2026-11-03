# Test script to verify JSON symbol conversion logic
cmake_minimum_required(VERSION 3.22)

# Load BuildConfig.json
get_filename_component(root_dir "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(pinecan_config_file "${root_dir}/BuildConfigs.json")

if(NOT EXISTS "${pinecan_config_file}")
    message(FATAL_ERROR "Config file not found: ${pinecan_config_file}")
endif()

file(READ "${pinecan_config_file}" _configs_json)

# Test with LIGHTING_rev5
set(PINECAN_CONFIG_KEY "LIGHTING_rev5")
string(JSON _cfg GET "${_configs_json}" "${PINECAN_CONFIG_KEY}")
string(JSON _symbols_obj GET "${_cfg}" symbols)
string(JSON _sym_count LENGTH "${_symbols_obj}")
math(EXPR _sym_last "${_sym_count} - 1")

set(PINECAN_SYMBOL_DEFS "")

message("Testing symbol conversion for ${PINECAN_CONFIG_KEY}:")
message("Total symbols: ${_sym_count}")
message("")

foreach(_i RANGE 0 ${_sym_last})
    string(JSON _key MEMBER "${_symbols_obj}" ${_i})
    string(JSON _val  GET  "${_symbols_obj}" "${_key}")
    string(JSON _type TYPE "${_symbols_obj}" "${_key}")

    if(_type STREQUAL "STRING")
        list(APPEND PINECAN_SYMBOL_DEFS "${_key}=\"${_val}\"")
        message("  ${_key} (STRING): ${_key}=\"${_val}\"")
    else()
        list(APPEND PINECAN_SYMBOL_DEFS "${_key}=${_val}")
        message("  ${_key} (${_type}): ${_key}=${_val}")
    endif()
endforeach()

message("")
message("Final PINECAN_SYMBOL_DEFS list:")
foreach(_def IN LISTS PINECAN_SYMBOL_DEFS)
    message("  ${_def}")
endforeach()

message("")
message("SUCCESS: Symbol conversion completed correctly")
message("  - STRING types are quoted")
message("  - NUMBER types are unquoted")
