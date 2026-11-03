# Test error handling in BuildConfigs.json parsing

cmake_minimum_required(VERSION 3.22)

get_filename_component(root_dir "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(pinecan_config_file "${root_dir}/BuildConfigs.json")
file(READ "${pinecan_config_file}" _configs_json)

# Test 1: Invalid config key
message("Test 1: Testing invalid config key...")
set(PINECAN_CONFIG_KEY "INVALID_KEY_DOES_NOT_EXIST")

string(JSON _has_key ERROR_VARIABLE _json_err
       GET "${_configs_json}" "${PINECAN_CONFIG_KEY}")

if(_json_err)
    message("Correctly detected missing config key: ${PINECAN_CONFIG_KEY}")
else()
    message(FATAL_ERROR "Failed to detect missing config key")
endif()

# Test 2: Wrong target location (use SSD_rev1 which has different target_location)
message("Test 2: Testing wrong target_location validation...")
set(PINECAN_CONFIG_KEY "SSD_rev1")

string(JSON _has_key ERROR_VARIABLE _json_err
       GET "${_configs_json}" "${PINECAN_CONFIG_KEY}")

if(_json_err)
    message(FATAL_ERROR "Config key '${PINECAN_CONFIG_KEY}' should exist")
endif()

string(JSON _cfg GET "${_configs_json}" "${PINECAN_CONFIG_KEY}")
string(JSON _target_location GET "${_cfg}" target_location)

if(NOT _target_location STREQUAL "targets/can-led")
    message("Correctly detected wrong target_location: '${_target_location}' != 'targets/can-led'")
else()
    message(FATAL_ERROR "Failed to detect wrong target_location")
endif()

message("All error handling tests passed!")
