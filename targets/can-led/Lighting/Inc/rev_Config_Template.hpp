/*
 * rev_Config_Template.hpp
 *
 * TEMPLATE ONLY -- comments only, zero executable code. Copy this file to
 * revN_config.hpp when a new PCB revision appears, fill in every field
 * below, then add an #elifdef REVN branch pointing at it in
 * lighting_controller.hpp. See LIGHTING_MIGRATION_SPEC.md §8.
 *
 * A new PCB should require a new config file ONLY -- no pattern logic, no
 * controller code, no main.cpp changes.
 *
 *  Author: Avi
 */

#ifndef INC_NEWREV_CONFIG_TEMPLATE_HPP_
#define INC_NEWREV_CONFIG_TEMPLATE_HPP_

// #include <cstdint>
// #include "conversions.hpp"

// #define BOARD_REV <N>
//   The numeric board revision this file describes. Matches the REV<N>
//   preprocessor define set per build config in the IDE project settings
//   (see .cproject) -- NOT defined here, only documented.

// static constexpr uint8_t num_leds = <count>;
//   Total number of physically addressable LEDs on this board, in wiring
//   (data-line) order. This is the array length for led_types[] and the
//   upper bound for every LED index used elsewhere in this file.

// static constexpr ChipType led_types[num_leds] = { ... };
//   One entry per physical LED, in the same order as num_leds, naming which
//   chip protocol that position uses:
//     CHIP_RGB  -- 3 channels, 24 bits/LED  (legacy class name: SK6812)
//     CHIP_RGBW -- 4 channels, 32 bits/LED  (legacy class name: WS2812)
//   Do NOT reuse the legacy class names anywhere in new code -- they are
//   inverted from real-world part naming in this repo (see
//   LIGHTING_MIGRATION_SPEC.md's READ FIRST section).

// static constexpr uint8_t num_leds_padding = <count>;
//   Number of "dummy" 32-bit-wide LED slots prepended to the bank buffer as
//   a reset/latch gap before real LED data. Board-specific; taken from the
//   legacy value for this revision unless a datasheet says otherwise.

// static constexpr uint16_t padding_size = num_leds_padding * 32;
//   Padding region size in PWM output bytes (one byte per bit of the
//   padding region, all zero). Derived -- do not hand-tune independently of
//   num_leds_padding.

// static constexpr uint16_t bank_output_buffer_size = <sum of each LED's bit
//         width (24 for CHIP_RGB, 32 for CHIP_RGBW) + padding_size>;
//   Total size, in PWM output bytes, of one full bank (one DMA half). Get
//   this wrong and the DMA chain truncates or overruns -- verify against
//   led_types[] before trusting a copy-pasted value. (rev4's legacy value
//   was wrong for exactly this reason -- see spec §11.4.)

// static constexpr uint16_t dma_output_buffer_size = bank_output_buffer_size * 2;
//   Two banks, double-buffered so DMA always streams a stable copy while the
//   next frame is being written. Derived -- do not hand-tune.

// static constexpr uint16_t zone_leds[CD_LENGTH] = { ... };
//   GLOBAL zone -> LED membership, one bitmask per ControlDomain (bit N set
//   == LED index N belongs to that zone), fixed for the whole board -- NOT
//   per lighting state. Legacy varied zone membership by state; that
//   variation is intentionally not carried forward (spec §8). Every zone's
//   bitmask must fit within num_leds bits.

// #define LED_TIM          <htimN>
//   The HAL timer handle (declared in Core/Inc/tim.h) driving the LED PWM
//   output for this board.

// #define LED_TIM_CHANNEL  <TIM_CHANNEL_N>
//   The timer channel on LED_TIM used for LED PWM output.

#endif /* INC_NEWREV_CONFIG_TEMPLATE_HPP_ */
