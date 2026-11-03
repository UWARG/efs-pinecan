# PineCAN LED README

## list of relevant files for the PR

These are the files most relevant when reviewing changes to the lighting
program. The remaining `Core` files are primarily STM32-generated peripheral,
startup, and support code.

- `Core/Src/main.cpp` — Application entry point. Initializes CAN, DMA, and the
  LED timer, services PineCAN, detects vehicle-state changes, selects a lighting
  pattern, and generates/pushes an LED frame every 20 ms.
- `Core/Inc/can_manager.hpp` and `Core/Src/can_manager.cpp` — The
  transport-independent CAN decoding layer. It forwards CAN servicing and
  cached-state reads to `can.c`, detects state changes, and maps the raw
  NotifyState bitmask to a lighting state.
- `Core/Src/can/can.c` (with `Core/Inc/can/can.h`) — CAN peripheral and PineCAN
  integration. It initializes PineCAN, runs its periodic service, decodes
  NotifyState messages in the receive handler, and safely caches the latest raw
  vehicle state for the main loop.
- `Lighting/Src/lighting_controller.cpp` (with
  `Lighting/Inc/lighting_controller.hpp`) — LED output engine. It owns the
  animation state and LED buffers, expands zones into physical LED colours,
  encodes RGB/RGBW data, and streams it through PWM/DMA.
- `Lighting/Src/pattern_table.cpp` (with
  `Lighting/Inc/pattern_table.hpp`) — Lighting definitions for each vehicle
  state. This is the main file to edit for colours, brightness, active zones,
  and animations.
- `Lighting/Inc/pattern_types.hpp` — Data structures and enums used to
  describe solid, breathing, and strobing zone appearances.
- `Lighting/Inc/conversions.hpp` and `Lighting/Src/conversions.cpp` — Shared
  colour values plus the lighting-state and control-domain enums used by both
  CAN decoding and the LED engine.
- `rev4/Inc/rev4_config.hpp` and `rev5/Inc/rev5_config.hpp` —
  Board-specific LED counts, chip types, zone-to-LED mappings, buffer sizes, and
  timer settings. The build selects the appropriate revision.
- `../../pinecan/common/pinecanCommon.c` and `../../pinecan/CMakeLists.txt` —
  Shared PineCAN fixes identified while debugging this target: the libcanard
  memory pool is explicitly aligned to 8 bytes, and runtime assertions are now
  opt-in rather than enabled automatically for Debug builds.

## Build and flash setup

The lighting target is a CMake project inside the larger `efs-pinecan`
repository. It depends on the shared `pinecan/` and `dsdl/` directories, so keep
the complete repository checked out and initialize its submodules before the
first build:

```powershell
git submodule update --init --recursive
```

### Shared DSDL files

Lighting uses the repository's `dsdl/generated/include` and
`dsdl/generated/src` through the `pinecan` CMake target. Its
`PINECAN_DSDL_SOURCES` list selects `ardupilot.indication.NotifyState.c`,
`uavcan.protocol.GetNodeInfo_res.c`, and `uavcan.protocol.NodeStatus.c`.
Supporting message headers also come from the shared directory.

Local generated files with shared equivalents have been removed.
`Core/dsdlc_generated` temporarily retains only four files that have no shared
equivalent: `include/cuav.equipment.power.CBAT.h`,
`src/cuav.equipment.power.CBAT.c`, `include/fred.MyPacket.h`, and
`include/fred.SetControlState.h`. These are backups pending hardware testing;
they are not build inputs or include paths. Remove them once testing confirms
they are unnecessary. Any future required message should live in the shared
DSDL directory rather than adding a lighting-specific copy.

The available configure and build presets are:

| Board | Debug | Release |
|---|---|---|
| Revision 4 | `rev4-debug` | `rev4-release` |
| Revision 5 | `rev5-debug` | `rev5-release` |

The configure preset and build preset must always match.

### VS Code

Install these VS Code extensions:

- **CMake Tools** by Microsoft (`ms-vscode.cmake-tools`)
- **STM32CubeIDE for Visual Studio Code** by STMicroelectronics

Either open the repository root (`efs-pinecan`) or open
`targets/can-led` directly with **File → Open Folder**. Both folders
include VS Code settings and tasks for the lighting project. Both use the same
presets and build outputs under `targets/can-led/build/`, so switching
the open folder does not require a separate build.

For the default **revision 5 Debug** workflow, run **Tasks: Run Task → Flash**.
This configures, builds, and flashes the board through ST-LINK. **Ctrl+Shift+B**
runs the default **Build** task without flashing. These tasks always use
`rev5-debug`; use the CMake Tools workflow below to choose another preset.

After opening the workspace:

1. Run **CMake: Select Configure Preset** from the Command Palette and choose the
   required board/build combination, for example `rev5-debug`.
2. Run **CMake: Configure**.
3. Run **CMake: Select Build Preset** and choose the same preset selected in
   step 1, for example `rev5-debug`.
4. Run **CMake: Build**. Use the CMake Tools command, not the STM32 extension's
   Build button.
5. To program the board, run **CMake: Build Target** and select `flash`.

For `rev5-debug`, the build produces:

```text
build/rev5-debug/can-led.elf
build/rev5-debug/can-led.hex
build/rev5-debug/can-led.bin
```

The other presets use the corresponding directory under `build/`.

### Command line

Run CMake from the lighting target directory:

```powershell
cd targets/can-led
cmake --preset rev5-debug
cmake --build --preset rev5-debug
```

Replace `rev5-debug` in both commands with the required preset. To build and
flash through ST-LINK, run:

```powershell
cmake --build --preset rev5-debug --target flash
```

Both workflows require CMake, Ninja, and the Arm GNU toolchain
(`arm-none-eabi-gcc`) to be available. Flashing also requires
`STM32_Programmer_CLI` on `PATH`.

This folder contains the data-driven lighting engine for the PineCAN LED board.
It replaces the old per-LED class hierarchy (`led`/`sk6812`/`ws2812`/
`lighting_controller`/`*_control_state_classes`) with three moving parts:

| File | Role |
|------|------|
| `lighting_controller.{hpp,cpp}` | The engine. Owns the PWM/DMA output, the animation loop, and the four entry points (`Led_Init`, `Select_Pattern`, `Generate_Leds`, `Push_Leds`). You rarely touch this. |
| `pattern_table.{hpp,cpp}` | **The one file to edit for colours, brightness, and animations.** One row per lighting state, one column per zone. |
| `pattern_types.hpp` | The `Zone_Appearance` / `AnimType` / `Anim_Params` data types the table is built from. |
| `rev4/Inc/rev4_config.hpp`, `rev5/Inc/rev5_config.hpp` | Per-board facts (LED count, chip types, zone→LED mapping, buffer sizes, timer). One is selected at compile time. |
| `rev_Config_Template.hpp` | Comment-only template. Copy it to add a new board revision. |
| `conversions.hpp` | Shared colour palette (`red`, `purple`, …) and the `ControlDomain` / `LightingStateTransition` enums. Also used by `can_manager.hpp`, so treat it as shared. |

## How a frame is produced

1. `main.cpp` polls the CAN cache and calls `Interpret_Vehicle_State()` to turn the
   raw `ardupilot.indication.NotifyState` bitmask into a state number (0–8).
2. On a state change, `Select_Pattern(state)` copies that row of
   `pattern_table` into the engine's `current_appearance[]`.
3. Every ~20 ms the inner loop calls `Generate_Leds()` (advances animations,
   expands active zones into a per-LED colour buffer) then `Push_Leds()`
   (encodes the buffer into the DMA-streamed PWM signal).

---

## PineCAN (CAN receive pipeline)

`Core/Src/can.c` is the **single owner** of all PineCAN interaction for this
target, mirroring the efs-pinecan single-servo driver reference. It owns the
`CanardInstance`, the `NodeStatus`, the RX handler body, and the async
`vehicle_state` cache. `CAN_Manager` (see Decode below) owns none of this — it
only forwards to `can.c`.

### Handler registration — `pinecan_handlers.h`

Header-only, no accompanying `.c`. PineCAN's `pinecanCommon.c` includes it and
registers handlers at **compile time** via the `RX_HANDLER_LIST` macro, so no
edits to the PineCAN source tree are needed:

```c
REGISTER_RX_HANDLER(TYPE, HANDLER, TRANSFER_KIND)
//   TYPE          — base name of the DSDL type (TYPE_ID + TYPE_SIGNATURE must exist)
//   HANDLER       — function called when a matching transfer arrives
//   TRANSFER_KIND — REQUEST | RESPONSE | BROADCAST
```

This target registers exactly one: `ARDUPILOT_INDICATION_NOTIFYSTATE` (BROADCAST)
→ `Handle_Notify_State`.

### Functions (all in `can.c`)

| Function | What it does |
|----------|--------------|
| `Init_Can()` | Builds `PinecanInit` from the CubeMX `hcan1`, sets `NodeStatus` defaults, calls `pinecanInit` once. On failure leaves `pinecan_initialized` false so `Can_Service()` never calls into PineCAN. |
| `Can_Service()` | Called every main-loop iteration; gates `pinecan1ms()` to fire once per millisecond (TX-queue drain + stale-transfer cleanup + NodeStatus). No-op until `Init_Can()` succeeds. |
| `Handle_Notify_State()` | Dispatched by PineCAN's `onTransferReceived` on a complete NotifyState broadcast (**interrupt context**). Thin: decode payload → cache `vehicle_state` on success, discard silently on decode failure. No mapping. |
| `Can_Get_Latest_Vehicle_State()` | Parameterless getter polled by the main loop. Returns the last cached raw bitmask, or 0 before any message. No mapping. |

### Async cache

`latest_vehicle_state` is written in interrupt context (`Handle_Notify_State`) and
read in the main loop (`Can_Get_Latest_Vehicle_State`). It's `volatile` so the poll
read isn't hoisted out of the loop, and because 64-bit access is not atomic on
Cortex-M4, reads/writes are bracketed with a brief IRQ critical section
(`__disable_irq`/`__enable_irq`) to avoid torn values.

---

## Decode (state mapping)

`CAN_Manager` (`can_manager.{hpp,cpp}`) is the pure, transport-agnostic, testable
core. It holds **no** PineCAN or HAL state — `Service()` and
`Get_Latest_Vehicle_State()` are thin forwarders to `can.c`, and the mapping is a
pure function safe to call directly in unit tests.

| Function | What it does |
|----------|--------------|
| `CAN_Manager::Service()` | Forwards to `Can_Service()`. |
| `CAN_Manager::Get_Latest_Vehicle_State()` | Forwards to `Can_Get_Latest_Vehicle_State()`. No mapping. |
| `CAN_Manager::Vehicle_State_Changed(raw)` | True if `raw` differs from the previous call. First call returns false to establish the baseline, so the initial cached 0 isn't delivered as a spurious change. |
| `CAN_Manager::Map_Vehicle_State(bitmask)` | Pure (no globals/HAL/PineCAN). Maps a 64-bit `vehicle_state` bitmask to a `LightingStateTransition`. |
| `Interpret_Vehicle_State(bitmask)` | Free function; thin delegator to `Map_Vehicle_State`. Call from the main loop when `Vehicle_State_Changed()` returns true. |

Bit mapping and precedence (highest wins) are in the
[Which 6 are actually used](#which-6-are-actually-used) table below.

---

## The 9 patterns (states)

`pattern_table` in `pattern_table.cpp` has 9 rows, indexed directly by the
`LightingStateTransition` enum (`conversions.hpp`). Each row lists which of the 8
zones (`ControlDomain`) are lit, their colour, brightness (0–100 **percent**),
and animation.

Colours: **MAIN** is always `purple @ 5%` (a dim "powered" glow). Animations are
`SOLID` unless noted. Only two animated cells exist in the whole table:
GROUND's beacon (breathe) and every active Strobe (double-flash, 800 ms).

| # | State | Active zones (colour @ brightness) | Reachable? |
|---|-------|------------------------------------|:----------:|
| 0 | `GROUND`  | MAIN purple@5 · BEACON red **breathe** (0→50%, 2 s) | ✅ |
| 1 | `STANDBY` | MAIN purple@5 · BEACON red@99 · STROBE orange@99 **strobe** | ✅ |
| 2 | `TAXI`    | MAIN purple@5 · TAXI white@99 · BEACON red@99 · BRAKE orange@99 | ❌ unused |
| 3 | `TAKEOFF` | MAIN purple@5 · BEACON red@99 | ✅ |
| 4 | `FLIGHT`  | MAIN purple@5 · NAV blue@99 · BEACON red@99 · STROBE orange@99 **strobe** | ✅ |
| 5 | `BRAKE`   | MAIN purple@5 · BRAKE orange@99 | ❌ unused |
| 6 | `LANDING` | MAIN purple@5 · LANDING white@99 · NAV blue@99 · BEACON red@99 · STROBE orange@99 **strobe** | ✅ |
| 7 | `SEARCH`  | MAIN purple@5 · SEARCH white@99 | ❌ unused |
| 8 | `STARTUP` | MAIN purple@5 | ✅ |

### Which 6 are actually used

Only **6 of the 9** rows can be reached today, because
`ardupilot.indication.NotifyState` has no bit that maps to taxiing, braking, or
comms-lost. The mapping lives in `CAN_Manager::Map_Vehicle_State` (`can_manager.hpp`):

| NotifyState bit | → State |
|-----------------|---------|
| (bitmask == 0)      | `GROUND` (0) |
| bit 1 `ARMED`       | `STANDBY` (1) |
| bit 23 `IS_TAKING_OFF` | `TAKEOFF` (3) |
| bit 2 `FLYING`      | `FLIGHT` (4) |
| bit 22 `IS_LANDING` | `LANDING` (6) |
| bit 0 `INITIALISING`| `STARTUP` (8) |

Precedence (highest wins): `IS_TAKING_OFF > IS_LANDING > FLYING > ARMED > INITIALISING`.
A non-zero state with no mapped bit returns `unrecognized_state` (0xFF), which
`Select_Pattern` ignores (holds the previous appearance).

**Used:** GROUND, STANDBY, TAKEOFF, FLIGHT, LANDING, STARTUP.
**Defined but unreachable:** TAXI (2), BRAKE (5), SEARCH (7).

The three unused rows are kept on purpose so `pattern_table[state]` stays a
direct index with no remapping layer. If a future NotifyState (or a different
message) ever exposes taxi/brake/comms-lost bits, add them to
`Map_Vehicle_State` and the rows are already there.

### Editing a pattern

Edit only `pattern_table.cpp`. Use the helpers at the top of the file:

- `Solid(colour, brightness)` — static colour.
- `Breathe(colour, period_ms, min_pct, max_pct)` — brightness ramps up/down.
- `Strobe(colour, brightness, period_ms)` — double-flash then idle.
- `Off()` — zone dark. (Always use `Off()`, never `brightness = 0`.)

Colours come from the palette in `conversions.hpp`. Brightness is a **percentage
(0–100)**, not 0–255.

---

## Board config files

Each revision config lives in `rev<N>/Inc/rev<N>_config.hpp`.

Everything that differs between physical boards lives in a single
`revN_config.hpp`. The engine and the pattern table are board-agnostic.

Exactly one config is compiled in, selected by a `REV<N>` preprocessor define.
The switch is in `lighting_controller.hpp`:

```cpp
#if defined(REV5)
#include "rev5_config.hpp"
#elif defined(REV4)
#include "rev4_config.hpp"
#endif
```

`REV4` / `REV5` are **not** set in source — they come from the build
configuration's compiler defines in `.cproject` ("REV4 Debug/Release",
"REV5 Debug/Release"). Pick the matching IDE build configuration for your board.

### Fields in a config file

| Field | Meaning |
|-------|---------|
| `BOARD_REV` | The numeric revision (documentation; matches the `REV<N>` define). |
| `num_leds` | Count of physically addressable LEDs, in wiring (data-line) order. |
| `led_types[num_leds]` | Per-LED chip protocol: `CHIP_RGB` (3 ch / 24 bit) or `CHIP_RGBW` (4 ch / 32 bit). |
| `num_leds_padding` / `padding_size` | Reset/latch gap prepended to the bank buffer. `padding_size = num_leds_padding * 32`. |
| `bank_output_buffer_size` | Bytes for one bank = Σ(each LED's bit width: 24 or 32) + `padding_size`. Get this wrong and the DMA chain truncates/overruns. |
| `dma_output_buffer_size` | `bank_output_buffer_size * 2` (double-buffered). |
| `zone_leds[CD_LENGTH]` | **Global** zone→LED bitmask (bit N set = LED N belongs to that zone). One fixed set per board, not per state. Must fit within `num_leds` bits. |
| `LED_TIM` / `LED_TIM_CHANNEL` | HAL timer handle + channel driving the LED PWM output. |

> ⚠️ Naming caveat: `CHIP_RGB` maps to the legacy class `SK6812` and `CHIP_RGBW`
> to legacy `WS2812` — **inverted from real-world part naming** in this repo.
> Use the `CHIP_*` names in all new code; don't reintroduce the legacy names.

### Current boards

- **rev4** — 10 LEDs, all `CHIP_RGBW`. Zones: corners `{0,2,3,5}`, sides
  `{6,7,8,9}`, E/W `{1,4}`.
- **rev5** — 12 LEDs, mixed (`{0,5,6,11}` are `CHIP_RGB`, the rest `CHIP_RGBW`).
  Three concentric rings of four: SIDE, OUTER, INNER.

---

## Adding a new board revision

A new PCB should require a **new config file only** — no engine, pattern, or
`main.cpp` changes.

1. Copy `Lighting/Inc/rev_Config_Template.hpp` to `rev<N>/Inc/rev<N>_config.hpp`.
   Add `rev<N>/Inc` to the CMake and editor include paths.
2. Uncomment and fill in **every** field (include guard, `#include`s,
   `BOARD_REV`, `num_leds`, `led_types[]`, padding, buffer sizes, `zone_leds[]`,
   `LED_TIM`, `LED_TIM_CHANNEL`). Follow the per-field notes in the template.
3. Double-check `bank_output_buffer_size` by hand against `led_types[]`
   (24 bits per `CHIP_RGB`, 32 per `CHIP_RGBW`, plus padding). A copy-pasted
   value from another rev is the most common bug here.
4. Add a branch in `lighting_controller.hpp`:

   ```cpp
   #if defined(REV5)
   #include "rev5_config.hpp"
   #elif defined(REV4)
   #include "rev4_config.hpp"
   #elif defined(REV<N>)
   #include "rev<N>_config.hpp"
   #endif
   ```

5. Add a `REV<N>` build configuration in the IDE (define `REV<N>` in that
   config's preprocessor symbols, alongside the existing rev configs in
   `.cproject`).

That's it — the pattern table and engine pick up the new board automatically
because they only reference the names the config provides.
