/*
 * pattern_types.hpp
 *
 * Keystone data types for the PineCAN-driven lighting pipeline (see
 * LIGHTING_MIGRATION_SPEC.md §6). A Zone_Appearance is the full description of
 * what one zone (ControlDomain) looks like: whether it's lit, its colour, its
 * brightness, and which animation (if any) drives it over time.
 *
 * "Off" is expressed by `active = false`
 *
 *  Author: Avi
 */

#ifndef INC_PATTERN_TYPES_HPP_
#define INC_PATTERN_TYPES_HPP_

#include <cstdint>

#include "conversions.hpp"

enum AnimType {
	ANIM_SOLID   = 0,   // static colour, no time dependence
	ANIM_BREATHE = 1,   // brightness ramps up and down
	ANIM_STROBE  = 2    // on/off flash sequence
};

struct Anim_Params {
	uint16_t period_ms;   
	uint8_t  min_pct;     
	uint8_t  max_pct;     
};

struct Zone_Appearance {
	bool          active;      
	RGB_colour_t  colour;     
	uint8_t       brightness;  // 0-100 PERCENT — not 0-255
	AnimType      animation;
	Anim_Params    params;
};

#endif 
