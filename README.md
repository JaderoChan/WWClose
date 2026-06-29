# WWClose (Win + W for Close)

[**简体中文** | **[English](README_EN.md)**]

![Version](https://img.shields.io/badge/版本-1.0.2-blue)
![Platform](https://img.shields.io/badge/平台-Windows-0078D6?logo=windows&logoColor=white)
![Language](https://img.shields.io/badge/语言-C%2B%2B17-00599C?logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.26+-064F8C?logo=cmake&logoColor=white)
![License](https://img.shields.io/badge/协议-MIT-green)

这是一个 **Windows** 系统托盘程序，将 `Win + W` 热键的原有功能（打开侧边组件）替换为 **关闭当前焦点窗口**，行为与 `Alt + F4` 完全一致。

此程序的目的是替代 `Alt + F4`。由于这两个键距离较远，使得该组合键难以单手操作；而本程序让 **Windows** 上的窗口关闭操作能够像 **macOS** 的 `⌘ Q` 一样便捷。

## 功能

- 拦截 `Win + W` 并屏蔽系统原有行为
- 向当前焦点窗口发送关闭命令（等同于 `Alt + F4`）
- 在桌面按下时，弹出 **Windows** 关机对话框
- 系统托盘常驻，低资源占用
- 支持 **开机自启动**

## 构建

### 依赖环境

- Windows 10 / 11
- CMake ≥ 3.26

### 步骤

```bash
git clone --recurse-submodules https://github.com/JaderoChan/WWClose.git
cd WWClose
cmake -B build
cmake --build build --config Release
```

编译产物位于 `build/bin/Release/WWClose.exe`。

## 依赖

| 库 | 说明 | 协议 |
| --- | --- | --- |
| [easy_translate](https://github.com/JaderoChan/easy_translate) | 轻量级 C++ 国际化库 | MIT |
| [keyboard_tools](https://github.com/JaderoChan/keyboard_tools) | 跨平台键盘钩子与按键工具库 | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON 解析（easy_translate 依赖） | MIT |

## 注意

为了使程序能够在高权限程序中正常使用（Windows 安全机制会阻止低权限进程的钩子接受高权限目标程序（使用管理员权限运行的进程）的键盘事件，导致快捷键失效），程序将默认以管理员权限运行。如果你需要关闭此行为，可以在 `cmake/templates/app.manifest.in` 中删除或注释相应的声明。

## 开源协议

本项目基于 [MIT License](https://opensource.org/licenses/MIT) 开源。

Copyright (c) 2026 JaderoChan
