# emu-windows

尼彩 CBE 模拟器的 Windows 外壳。只用标准库 tkinter，不需要额外的界面依赖。

模拟核心是 [emu-core-rs](https://github.com/nieche-cbe-emu/emu-core-rs) 的
`nieche.dll`，经 `nieche.py`（ctypes）调用。
**没有回落**——不需要也不会去用 Python 实现。

## 运行

```
python nieche_win.py
```

需要 emu-core-rs 的 `python/nieche.py` 和构建好的 `nieche.dll`
放在同级或上一级目录。

## 打包成单文件 exe

仓库里的 GitHub Actions 工作流会在 Windows runner 上打好并传到 release，
包括在 runner 上现编 `nieche.dll`。本地打的话：

```
pip install pyinstaller
pyinstaller --onefile --noconsole --add-binary "nieche.dll;." nieche_win.py
```

打出来的 exe **必须关掉 Control Flow Guard**（`editbin /GUARD:NO`），
否则 unicorn 的 JIT 一跑整个进程会无声消失：退出码 0xC0000409，
没有任何异常也没有日志。`nieche_win.py --uctest` 就是用来验这件事的，
它会让核心真跑几条 ARM 指令——**只加载 DLL 是测不出来的**。

## 说明

本仓库只有代码。游戏数据不在这里，也不会提供。
