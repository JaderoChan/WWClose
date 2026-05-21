# WWClose (Win + W for Close)

[**[简体中文](README.md)** | **English**]

![Version](https://img.shields.io/badge/version-1.0.2-blue)
![Platform](https://img.shields.io/badge/platform-Windows-0078D6?logo=windows&logoColor=white)
![Language](https://img.shields.io/badge/language-C%2B%2B17-00599C?logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.26+-064F8C?logo=cmake&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-green)

A **Windows** system-tray application that replaces the default `Win + W` shortcut (which opens the Widgets panel) with **closing the currently focused window** — identical in behaviour to `Alt + F4`.

The goal is to provide a more ergonomic alternative to `Alt + F4`. Because the two keys are far apart, that shortcut is awkward to trigger one-handed. WWClose lets you close windows on **Windows** just as conveniently as `⌘ W` on **MacOS**.

## Features

- Intercepts `Win + W` and suppresses the system's default action
- Sends a close command to the focused window (equivalent to `Alt + F4`)
- Triggers the Windows shutdown dialog when pressed on the desktop
- Runs as a system-tray icon with minimal resource usage
- **Launch at startup** support

## Build

### Requirements

- Windows 10 / 11
- CMake ≥ 3.26

### Steps

```bash
git clone --recurse-submodules https://github.com/JaderoChan/WWClose.git
cd WWClose
cmake -B build
cmake --build build --config Release
```

The compiled executable will be at `build/bin/Release/WWClose.exe`.

## Dependencies

| Library | Description | License |
| ------- | ----------- | ------- |
| [easy_translate](https://github.com/JaderoChan/easy_translate) | Lightweight C++ i18n library | MIT |
| [keyboard_tools](https://github.com/JaderoChan/keyboard_tools) | Cross-platform keyboard hook & key utility library | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON parsing (used by easy_translate) | MIT |

## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT).

Copyright (c) 2026 JaderoChan
