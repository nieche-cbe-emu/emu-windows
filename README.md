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

## 键位映射：尚未定论

模拟器目前把 **bit12 当左软键、bit13 当右软键**，挂断键不映射任何位。

依据来自孤岛的过场文本页：屏幕左下角画「加速」、右下角画「跳过」——软键标签的
标准位置——按 bit13 触发的正是「跳过」，按 bit12/确定 触发「加速」；进游戏后
底部左「菜单」右「商城」，bit12 打开的就是菜单。众神之战同样 bit12 开菜单。

**但这不足以当作结论，右软键的行为对不上它自己的标签：**

- 众神之战右下角画「任务」，按 bit13 弹的却是「是否退出游戏？」
- 孤岛游戏内右下角画「商城」，按 bit13 弹的却是「确定要回到标题界面？」

两个游戏都没有用右软键打开标签上写的那个功能。可能是这些标签本来就只吃触摸
（众神之战的「任务」，32 个位逐个长按 40 帧都打不开，游戏也从不调用
`Get_CurKeyDownState` 读原始键状态字），也可能是映射本身还不对。

挂断键不映射任何位，同样只是推断：没有任何模块轮询过一个"挂断位"，
真机上它由手机系统直接终止应用。

固件里 `CurKeyDownState` 这个全局是间接寻址的，literal pool 里搜不到引用，
所以还没能从固件侧读出手机键码到位的翻译表。这条线索还没走完。

键位在模拟器里可以自己改。**欢迎带着实机对照的结果开 issue。**

## 关于 Windows on ARM

目前只出 x64 版。原生 ARM64 卡在上游：unicorn 的构建里有汇编，走的是 MSVC 的
`masm.targets`，而 ARM64 工具链不支持 MASM（`MASM not supported on this platform`），
换 ClangCL 工具集也一样。capstone 本身能编出 ARM64。

实际影响不大：**Windows on ARM 自带 x64 模拟，x64 版可以直接跑**，只是性能有折损。
工作流里 arm64 那一路仍然保留（标了 `continue-on-error`），等上游能编了就能出产物。
