/*
 * lighting_controller.cpp
 *
 * Implements the four entry points declared in lighting_controller.hpp
 * plus the internal animation engine (see LIGHTING_MIGRATION_SPEC.md)
 *
 *  Author: Avi
 */

#include <cstdint>
#include <cstring>

#include "lighting_controller.hpp"
#include "pattern_table.hpp"

namespace {

uint8_t bank_output_buffer[bank_output_buffer_size];
uint8_t dma_output_buffer[dma_output_buffer_size];

RGB_colour_t colour_buffer[num_leds];

Zone_Appearance current_appearance[CD_LENGTH];

// Per-zone animation runtime state (spec §9.3) -- reset by Select_Pattern on
// every state change so each animation starts fresh.
struct Anim_State {
	uint8_t ramp_pos;     // Breathe: current brightness within [min_pct, max_pct]
	int8_t  direction;    // Breathe: +1 rising, -1 falling
	uint8_t stage;        // Strobe: sub-stage counter, 0..STROBE_NUM_STAGES-1
	uint8_t stage_ticks;  // Strobe: calls elapsed within the current stage
};
Anim_State animation_state[CD_LENGTH];

constexpr uint8_t pwm_lo = 19;
constexpr uint8_t pwm_hi = 38;

// One rate-gated call is ~20ms (spec §9.6). Strobe's 800ms period splits
// into 8 stages of 100ms each, i.e. 5 calls per stage.
constexpr uint8_t strobe_stage_ticks = 5;
constexpr uint8_t strobe_num_stages  = 8;

RGB_colour_t Scale(RGB_colour_t colour, uint8_t brightness_pct) {
	return {
		static_cast<uint8_t>(colour.red   * brightness_pct / 100),
		static_cast<uint8_t>(colour.green * brightness_pct / 100),
		static_cast<uint8_t>(colour.blue  * brightness_pct / 100),
	};
}

void Reset_Animation_State() {
	for (int z = 0; z < CD_LENGTH; ++z) {
		animation_state[z].ramp_pos    = current_appearance[z].params.min_pct;
		animation_state[z].direction   = 1;
		animation_state[z].stage       = 0;
		animation_state[z].stage_ticks = 0;
	}
}

void Update_Animations(uint32_t tick) {
	(void) tick; 
	for (int z = 0; z < CD_LENGTH; ++z) {
		Zone_Appearance &appearance = current_appearance[z];
		Anim_State &state = animation_state[z];

		switch (appearance.animation) {
		case ANIM_SOLID:
			break;

		case ANIM_BREATHE: {
			uint8_t min_pct = appearance.params.min_pct;
			uint8_t max_pct = appearance.params.max_pct;
			if (state.ramp_pos <= min_pct) {
				state.ramp_pos  = min_pct;
				state.direction = 1;
			} else if (state.ramp_pos >= max_pct) {
				state.ramp_pos  = max_pct;
				state.direction = -1;
			}
			appearance.brightness = state.ramp_pos;
			state.ramp_pos = static_cast<uint8_t>(state.ramp_pos + state.direction);
			break;
		}

		case ANIM_STROBE:
			
			if (++state.stage_ticks >= strobe_stage_ticks) {
				state.stage_ticks = 0;
				state.stage = static_cast<uint8_t>((state.stage + 1) % strobe_num_stages);
			}
			appearance.active = (state.stage == 0) || (state.stage == 2);
			break;
		}
	}
}

} // namespace

void Led_Init() {
	// Padding region zeroed, data region = pwm_lo pattern -- all LEDs start
	// off until the first Generate_Leds/Push_Leds call updates them.
	for (uint16_t i = 0; i < bank_output_buffer_size; ++i) {
		bank_output_buffer[i] = (i < padding_size) ? 0 : pwm_lo;
	}

	std::memcpy(dma_output_buffer, bank_output_buffer, bank_output_buffer_size);
	std::memcpy(dma_output_buffer + bank_output_buffer_size, bank_output_buffer,
			bank_output_buffer_size);

	HAL_TIM_PWM_Start_DMA(&LED_TIM, LED_TIM_CHANNEL,
			reinterpret_cast<uint32_t *>(dma_output_buffer), dma_output_buffer_size);

	// Boot default: show a valid pattern before the first CAN message.
	Select_Pattern(TRANSITION_GROUND);
}

void Select_Pattern(uint8_t flight_state) {
	if (flight_state >= state_count) {
		// Covers 0xFF (CAN_Manager::unrecognized_state) and any other
		// out-of-range value: hold the previous appearance, ignore this call.
		return;
	}

	for (int z = 0; z < CD_LENGTH; ++z) {
		current_appearance[z] = pattern_table[flight_state][z];
	}
	Reset_Animation_State();
}

void Generate_Leds(uint32_t tick) {
	Update_Animations(tick);

	for (int i = 0; i < num_leds; ++i) {
		colour_buffer[i] = RGB_colour_t{0, 0, 0};
	}

	// Zone overlap: zones are applied in ascending index order, so a higher-numbered
	// zone overwrites a lower-numbered one on any shared LED.
	for (int z = 0; z < CD_LENGTH; ++z) {
		if (!current_appearance[z].active) {
			continue;
		}
		RGB_colour_t scaled = Scale(current_appearance[z].colour, current_appearance[z].brightness);
		for (int i = 0; i < num_leds; ++i) {
			if (zone_leds[z] & (1 << i)) {
				colour_buffer[i] = scaled;
			}
		}
	}
}

void Push_Leds() {
	uint16_t offset = padding_size;

	for (int i = 0; i < num_leds; ++i) {
		uint8_t r = colour_buffer[i].red;
		uint8_t g = colour_buffer[i].green;
		uint8_t b = colour_buffer[i].blue;

		if (led_types[i] == CHIP_RGBW) {
			uint8_t w = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
			r = static_cast<uint8_t>(r - w);
			g = static_cast<uint8_t>(g - w);
			b = static_cast<uint8_t>(b - w);

			// Wire order GRBW, MSB-first within each channel.
			for (int bit = 7; bit >= 0; --bit) {
				bank_output_buffer[offset++] = ((g >> bit) & 0x1) ? pwm_hi : pwm_lo;
			}
			for (int bit = 7; bit >= 0; --bit) {
				bank_output_buffer[offset++] = ((r >> bit) & 0x1) ? pwm_hi : pwm_lo;
			}
			for (int bit = 7; bit >= 0; --bit) {
				bank_output_buffer[offset++] = ((b >> bit) & 0x1) ? pwm_hi : pwm_lo;
			}
			for (int bit = 7; bit >= 0; --bit) {
				bank_output_buffer[offset++] = ((w >> bit) & 0x1) ? pwm_hi : pwm_lo;
			}
		} else { // CHIP_RGB
			// Wire order GRB, MSB-first within each channel.
			for (int bit = 7; bit >= 0; --bit) {
				bank_output_buffer[offset++] = ((g >> bit) & 0x1) ? pwm_hi : pwm_lo;
			}
			for (int bit = 7; bit >= 0; --bit) {
				bank_output_buffer[offset++] = ((r >> bit) & 0x1) ? pwm_hi : pwm_lo;
			}
			for (int bit = 7; bit >= 0; --bit) {
				bank_output_buffer[offset++] = ((b >> bit) & 0x1) ? pwm_hi : pwm_lo;
			}
		}
	}
}


void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
	(void) htim;
	// | BANK 1 | BANK 2 |
	//          ^ Current location -- update BANK 1
	std::memcpy(dma_output_buffer, bank_output_buffer, bank_output_buffer_size);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	(void) htim;
	// | BANK 1 | BANK 2 |
	//                   ^ Current location -- update BANK 2
	std::memcpy(dma_output_buffer + bank_output_buffer_size, bank_output_buffer,
			bank_output_buffer_size);
}
