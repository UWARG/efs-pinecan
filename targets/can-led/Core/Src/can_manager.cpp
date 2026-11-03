#include "can_manager.hpp"
#include "can.h"

// Pure decode/forwarding core (no PineCAN/HAL state); see Lighting/Lighting.md (Decode).

// Forwards to Can_Service().
void CAN_Manager::Service() {
    Can_Service();
}

// Forwards to Can_Get_Latest_Vehicle_State(); no mapping.
uint64_t CAN_Manager::Get_Latest_Vehicle_State() {
    return Can_Get_Latest_Vehicle_State();
}

// True if raw_vehicle_state differs from the previous call; first call returns false (baseline).
uint64_t CAN_Manager::prev_data_ = 0;
bool     CAN_Manager::have_prev_ = false;

bool CAN_Manager::Vehicle_State_Changed(uint64_t raw_vehicle_state) {
    if (!have_prev_) {
        prev_data_ = raw_vehicle_state;
        have_prev_ = true;
        return false;
    }

    if (raw_vehicle_state != prev_data_) {
        prev_data_ = raw_vehicle_state;
        return true;
    }
    return false;
}

// Pure map from a 64-bit vehicle_state bitmask to a LightingStateTransition (bits/precedence in Lighting.md).
uint8_t CAN_Manager::Map_Vehicle_State(uint64_t vehicle_state) {
    if (vehicle_state == 0) {
        return static_cast<uint8_t>(TRANSITION_GROUND);
    }

    constexpr uint64_t mapped_bits =
        (1ULL << 23) | (1ULL << 22) | (1ULL << 2) | (1ULL << 1) | (1ULL << 0);

    if ((vehicle_state & mapped_bits) == 0) {
        return unrecognized_state;
    }

    if (vehicle_state & (1ULL << 23)) return static_cast<uint8_t>(TRANSITION_TAKEOFF);
    if (vehicle_state & (1ULL << 22)) return static_cast<uint8_t>(TRANSITION_LANDING);
    if (vehicle_state & (1ULL <<  2)) return static_cast<uint8_t>(TRANSITION_FLIGHT);
    if (vehicle_state & (1ULL <<  1)) return static_cast<uint8_t>(TRANSITION_STANDBY);
    if (vehicle_state & (1ULL <<  0)) return static_cast<uint8_t>(TRANSITION_STARTUP);

    return unrecognized_state;
}

// Thin delegator to Map_Vehicle_State; call when Vehicle_State_Changed() returns true.
uint8_t Interpret_Vehicle_State(uint64_t vehicle_state) {
    return CAN_Manager::Map_Vehicle_State(vehicle_state);
}
