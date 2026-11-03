/*
 * can_manager.hpp
 *
 * Pure, transport-agnostic decode core for the CAN receive pipeline.
 * See Lighting/Lighting.md (PineCAN + Decode) for the full description.
 */

#ifndef INC_CAN_MANAGER_HPP_
#define INC_CAN_MANAGER_HPP_

#include <cstdint>
#include "conversions.hpp"

class CAN_Manager {
public:
    /// Returned by Map_Vehicle_State when no bit maps to a state.
    static constexpr uint8_t unrecognized_state = 0xFF;

    /// Pure map from a 64-bit vehicle_state bitmask to a LightingStateTransition.
    static uint8_t Map_Vehicle_State(uint64_t vehicle_state);

    /// Forwards to Can_Get_Latest_Vehicle_State(); no mapping.
    static uint64_t Get_Latest_Vehicle_State();

    /// True if raw_vehicle_state differs from the previous call; first call returns false (baseline).
    static bool Vehicle_State_Changed(uint64_t raw_vehicle_state);

    /// Forwards to Can_Service().
    static void Service();

private:
    static uint64_t prev_data_;
    static bool     have_prev_;
};

/// Thin delegator to CAN_Manager::Map_Vehicle_State; call when Vehicle_State_Changed() returns true.
uint8_t Interpret_Vehicle_State(uint64_t vehicle_state);

#endif /* INC_CAN_MANAGER_HPP_ */
