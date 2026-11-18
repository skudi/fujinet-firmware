# lib/statusDisplay

A status screen for boards with a built-in LCD: WiFi network and IP, mounted disks, and bus
activity.

## How it fits
- `src/main.cpp` owns one `StatusDisplay` and calls `start()` at the end of setup.
- It reads state only: `fnWiFi`, `fnSystem.Net`, the disk slots of `SYSTEM_BUS.theFuji`, and
  `fnLedManager.last_on_us(LED_BUS)` for bus activity.
- A board opts in by defining `PIN_LCD_*` and the `LCD_*` geometry in its
  [include/pinmap/](../../include/pinmap/) header.

## Build
ESP: globbed by `src/CMakeLists.txt`; without `PIN_LCD_SCK`, `start()` does nothing. PC: not
compiled.

## Notes
`font8x16.h` is generated from the Spleen 8x16 BDF (https://github.com/fcambus/spleen), version
2.2.0, BSD-2-Clause, unmodified glyphs; the license is in the file.
