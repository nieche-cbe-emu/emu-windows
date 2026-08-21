# emu-windows

尼彩 CBE 模拟器的 Windows 外壳。只用标准库 tkinter，不需要额外的界面依赖。

## 运行

```
pip install unicorn capstone
python nieche_win.py
```

需要 [emu-core](https://github.com/nieche-cbe-emu/emu-core) 的 `emu/` 与
`cbelib/` 放在同级或上一级目录。

## 打包成单文件 exe

```
pip install pyinstaller
pyinstaller --onefile --windowed --add-data "emu;emu" --add-data "cbelib;cbelib" nieche_win.py
```

## 操作

左边游戏库、中间画面、右边键盘。WASD 或方向键移动，J/空格/回车确定，
K 左软键，L 右软键，Esc 挂断，数字键直通。画面可直接点击——
软键和对话框按钮多数只吃触摸。

画面转换用一张 65536 项的查表把 RGB565 转成 PPM，纯 Python 也能跑到 60fps。
