# FreeRTOS Kernel V11.1.0

> Vendored at: `third_party/freertos/`
> Version: **V11.1.0** (see `include/task.h` → `tskKERNEL_VERSION_NUMBER`)
> License: **MIT** (see LICENSE)
> Upstream: https://github.com/FreeRTOS/FreeRTOS-Kernel
> Copyright: (c) 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.

## Usage in yuleASR

- `include/` — FreeRTOS kernel public headers (FreeRTOS.h, task.h, queue.h, …)
- `src/` — kernel implementation (tasks.c, queue.c, list.c, timers.c, …)
- `portable/` — port layer (posix port used by native host builds; arm_cm33
  port for the S32K312 target)

## License

Distributed under the MIT License. See `LICENSE` in this directory.

## SBOM

Declared in `sbom.json` as `SPDXRef-freertos` (MIT).
