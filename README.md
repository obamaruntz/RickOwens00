# RickOwens00 Driver

## discord: m2n5m35n63b5mbv56m34bm73v4bn3v5v

## Overview

RickOwens00 is a kernel driver that works for UM anticheats and EOS anticheat.

---

## IOCTL Definitions

| IOCTL Name        | Code        | Description                              |
|-------------------|-------------|------------------------------------------|
| `PRW_CODE`        | 0x2ec33     | Physical memory read/write               |
| `VRW_ATTACH_CODE` | 0x2ec34     | Attach to process for virtual memory RW  |
| `VRW_CODE`        | 0x2ec35     | Virtual memory read/write                |
| `BA_CODE`         | 0x2ec36     | Get process base address                 |
| `GR_CODE`         | 0x2ec37     | Get guarded memory region                |
| `HF_CODE`         | 0x2ec38     | Unlink a process from active list        |
| `SECURITY_CODE`   | 0x94c9e4bc3 | Required for all valid requests          |

---

## Functionality

Physical Memory Access
- Translates virtual to physical address using CR3 of target process.
- Reads or writes physical memory across page boundaries safely.

Virtual Memory Access
- Uses MmCopyVirtualMemory for safe memory transfer.
- Requires VRW_ATTACH_CODE to set a valid target context.

Process Base Address Retrieval
- Returns the image base of a specified process by PID.

Guarded Region Lookup
- Scans big pool allocations for a region tagged TnoC with size 0x200000.

Process Unlinking
- Unlinks a process from ActiveProcessLinks to hide it from typical enumeration.
