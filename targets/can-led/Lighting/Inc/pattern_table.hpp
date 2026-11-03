/*
 * pattern_table.hpp
 *
 * Declares the pattern table: one row per LightingStateTransition, one
 * column per ControlDomain (see LIGHTING_MIGRATION_SPEC.md §7). The table
 * itself — the only file meant to be edited when colours, brightness, or
 * animations change — lives in pattern_table.cpp.
 *
 *  Author: Avi
 */

#ifndef INC_PATTERN_TABLE_HPP_
#define INC_PATTERN_TABLE_HPP_

#include <cstdint>

#include "conversions.hpp"
#include "pattern_types.hpp"


// limit for Select_Pattern.
static constexpr uint8_t state_count = 9;

// One row per state. Index matches LightingStateTransition exactly.
extern const Zone_Appearance pattern_table[state_count][CD_LENGTH];

#endif
