# emu-windows

CoolBar `.cbe` 模拟器的 Windows 外壳。界面只用标准库 tkinter；模拟核心是
[emu-core-rs](https://github.com/nieche-cbe-emu/emu-core-rs) 的 `nieche.dll`，
经 `nieche.py`（ctypes）调用。

## 特性

- 仅依赖标准库，无第三方界面依赖
- 键盘与虚拟键盘输入，鼠标点击映射为触摸
- 帧率可任意设定，并显示实测帧率
- 游戏库：记录用过的模块
- `--uctest` / `--coretest` 两个自检入口，用于诊断打包产物

## 环境要求

- Windows x64
- Python 3.11（带 tkinter）
- `nieche.py` 与 `nieche.dll` 位于同级或上一级目录

## 运行

```bash
python nieche_win.py
```

## 命令行参数

| 参数 | 说明 |
|---|---|
| `--uctest` | 让核心真跑几条 ARM 指令，验证 JIT 可用后退出 |
| `--coretest` | 加载核心并打印 ABI 版本后退出 |

## 环境变量

| 变量 | 默认值 | 说明 |
|---|---|---|
| `NIECHE_HOME` | `~/.nieche-emu` | 数据根：存档、游戏库与崩溃日志 |
| `NIECHE_LIB` | 自动查找 | `nieche.dll` 的路径 |

## 打包成 exe

仓库内的 GitHub Actions 工作流 `build-exe.yml` 会在 Windows runner 上编译
`nieche.dll`、打包三种形态并上传到 release。手动打包：

```bash
pip install pyinstaller
pyinstaller --onefile --windowed --add-binary "nieche.dll;." nieche_win.py
```

打包后必须关闭 Control Flow Guard 并加大栈，否则 JIT 执行时进程会以
`0xC0000409` 直接终止，且没有任何 Python 异常：

```bat
editbin /STACK:8388608 /GUARD:NO dist\NiecheEmu.exe
```

用 `NiecheEmu.exe --uctest` 验证：只加载 DLL 不足以覆盖该问题，需要真正执行到 JIT。

## 帧率

模块的动画与计时按帧推进，帧率直接决定游戏快慢。原机运行这些模块约
10–15 fps。画面下方可直接输入任意帧率（1–240，回车生效），旁边显示实测值。

## 说明

本仓库只包含代码。游戏数据不在此处，也不提供。
