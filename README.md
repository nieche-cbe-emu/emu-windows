# emu-windows

尼彩 CBE 模拟器的 Windows 外壳。只用标准库 tkinter，不需要额外的界面依赖。

## 运行

```
python nieche_win.py
```

需要 [emu-core](https://github.com/nieche-cbe-emu/emu-core) 的 `emu/` 与
`cbelib/` 放在同级或上一级目录。

核心优先用 `nieche.dll`（emu-core 的 Rust 实现，经 ctypes 调用），
没有就回落到 Python 实现——那条路要 `pip install unicorn capstone`。
**状态栏会显示当前用的是哪个核心**。

## 打包成单文件 exe

仓库里的 GitHub Actions 工作流会在 Windows runner 上打好并传到 release。
本地打的话：

```
pip install pyinstaller
pyinstaller --onefile --noconsole nieche_win.py
```

打出来的 exe **必须关掉 Control Flow Guard**，否则 unicorn 的 JIT 一跑
整个进程会无声消失。工作流里已经处理了。

## 说明

本仓库只有代码。游戏数据不在这里，也不会提供。
