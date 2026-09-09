# emu-windows

CoolBar `.cbe` 模拟器的 Windows 外壳。C++ / Qt 6，界面与
[emu-macos](https://github.com/nieche-cbe-emu/emu-macos) 一致。模拟核心是
[emu-core-rs](https://github.com/nieche-cbe-emu/emu-core-rs) 的 `nieche.dll`，
运行时动态加载。

## 特性

- 三栏布局：游戏库 / 画面 / 控制区（缩放、旋转、放大、声音、帧率、虚拟键盘、模块日志）
- 键盘与虚拟键盘输入，两者都支持按住；鼠标点击映射为触摸，画面旋转后坐标同步换算
- 帧率任意设定（1–240），并显示实测帧率
- 不含 Python，不需要安装运行时

## 环境要求

- Windows 10 1809 及以上，x64
- 构建需要：Qt 6.10、MSVC 2022（C++ 工具集）、CMake 3.21 及以上
- 运行需要 `nieche.dll` 与 Qt 运行库位于 exe 同级目录（发布包已包含）

## 安装

从 release 下载 zip，解压后运行 `NiecheEmu.exe`。不需要安装，也不需要额外运行时。

## 构建

```bash
cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH=<Qt 安装路径> -DNIECHE_DIR=<emu-core-rs 路径>
cmake --build build --config Release
windeployqt --release build\Release\NiecheEmu.exe
```

`NIECHE_DIR` 用于定位 `emuffi/nieche.h`，默认取同级的 `../rust`。
构建好的 `nieche.dll` 需要手动放到 exe 同级目录。

仓库内的 GitHub Actions 工作流 `build-qt.yml` 会完成上述全部步骤并上传发布包。

## 关键配置

| 项 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `NIECHE_HOME` | 环境变量（路径） | `%USERPROFILE%\.nieche-emu` | 数据根：游戏库与存档 |
| 帧率 | 整数 | `30` | 1–240，控制区按钮设定，保存在注册表 |
| 缩放 | 整数 / 适应窗口 | `2×` | 整数倍或按窗口等比放大 |
| 旋转 | 0 / 90 / 180 / 270 | `0°` | 触摸坐标随之换算 |

游戏库读取 `%NIECHE_HOME%\games` 下的 `.cbe` 文件。

## 帧率

模块的动画与计时按帧推进，帧率直接决定游戏快慢。原机运行这些模块约
10–15 fps。控制区可直接输入任意帧率，旁边显示实测值。

## 说明

本仓库只包含代码。游戏数据不在此处，也不提供。
