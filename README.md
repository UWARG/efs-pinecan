# efs-pinecan

PineCAN is our lightweight DroneCAN protocol stack for embedded peripherals on WARG drones. It exists on top of libcanard and allows target firmware to communicate over CAN bus without dealing with protocol-level concerns directly.

It also serves as a monorepo for our CAN code: the repo houses both the board-agnostic CAN library and the target firmware projects that use it.

For getting set up for the first time, see [Development Setup](https://uwarg-docs.atlassian.net/wiki/spaces/efs/pages/3908403220/Development+Setup)

For more detailed documentation see the [Pinecan Docs](https://uwarg-docs.atlassian.net/wiki/spaces/efs/pages/3740401671/PineCAN)
