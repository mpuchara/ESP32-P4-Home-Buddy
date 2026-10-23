# Vendored BSP

Source: https://github.com/ultramcu/guition-jc4880p4-bsp
Commit: 324970bade0d1f4e52880fe8016580368bc1e06e
License: MIT, copyright 2026 ultramcu (LICENSE retained).

Files:
- src/board_p4.c blob 5da3fceaf228afb82e68416e752b55eaf8db1bb3
- src/board_p4.h
- src/board_p4_pins.h
- src/board_p4_st7701_init.h

Upstream comments such as VERIFIED and claims about visible operation refer to
the upstream author's hardware, not Home Buddy acceptance. See docs/HARDWARE.md.
Only this source set is reused; there is no Arduino dependency.

Local changes and build fixes are recorded here as they are applied.

* Fix nested comment marker rejected by IDF -Werror=comment.
* Treat PPA registration failure as display startup failure.
* Reject failed touch reads and use esp_lcd_touch_get_data (1.2.1 API), avoiding
  stale coordinates and the deprecated get_coordinates API.
* Landscape-only path validates dimensions and waits for DPI refresh completion
  before reusing the former front buffer. A refresh failure freezes buffer writes
  until reboot instead of writing into a possibly scanned framebuffer.
