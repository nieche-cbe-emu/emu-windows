
"""尼彩 CBE 模拟器 —— Windows 外壳

只用标准库（tkinter），不需要装任何东西就能跑，也方便 PyInstaller 打成单文件 exe。
布局和 macOS 版一致：左边游戏库、中间画面、右边键盘。

画面转换是这里唯一的性能要害：客户机给的是 RGB565，Tk 的 PhotoImage 要的是
PPM(P6) 的 RGB888。逐像素算的话一帧 96000 次，纯 Python 扛不住 30fps，
所以预先建一张 65536 项的查表，每帧只做查表 + join。
"""
import array
import os
import sys
import time
import tkinter as tk
from tkinter import filedialog, messagebox

_HERE = os.path.dirname(os.path.abspath(__file__))

sys.path.insert(0, _HERE)
sys.path.insert(0, os.path.dirname(_HERE))

from emu.host import Session
from emu import paths

KEYS = {

    "lsk": 1 << 12, "rsk": 1 << 13, "call": 1 << 20, "end": 1 << 13,
    "up": (1 << 2) | (1 << 17), "down": (1 << 8) | (1 << 18),
    "left": (1 << 4) | (1 << 15), "right": (1 << 6) | (1 << 16),
    "ok": (1 << 5) | (1 << 14),
    "k1": 1 << 19, "k2": 1 << 18, "k3": 1 << 20, "k4": 1 << 15, "k5": 1 << 14,
    "k6": 1 << 16, "k7": 1 << 21, "k8": 1 << 17, "k9": 1 << 22,
    "star": 1 << 23, "k0": 1 << 24, "pound": 1 << 25,
}

LABEL = {
    "lsk": "左软键", "rsk": "右软键", "call": "呼叫", "end": "挂断",
    "up": "▲", "down": "▼", "left": "◀", "right": "▶", "ok": "OK",
    "star": "✱", "pound": "#",
}

KEYBOARD = {
    "w": "up", "s": "down", "a": "left", "d": "right",
    "Up": "up", "Down": "down", "Left": "left", "Right": "right",
    "j": "ok", "space": "ok", "Return": "ok",
    "k": "lsk", "l": "rsk", "u": "call", "Escape": "end",
    "1": "k1", "2": "k2", "3": "k3", "4": "k4", "5": "k5",
    "6": "k6", "7": "k7", "8": "k8", "9": "k9", "0": "k0",
}

def build_lut():
    lut = []
    for v in range(65536):
        r = (v >> 11) & 0x1F
        g = (v >> 5) & 0x3F
        b = v & 0x1F
        lut.append(bytes(((r * 255 + 15) // 31, (g * 255 + 31) // 63,
                          (b * 255 + 15) // 31)))
    return lut

LUT = build_lut()

def to_ppm(px, w, h):
    a = array.array("H")
    a.frombytes(px)
    if sys.byteorder != "little":
        a.byteswap()
    body = b"".join([LUT[v] for v in a])
    return b"P6\n%d %d\n255\n" % (w, h) + body

class App:
    def __init__(self, root):
        self.root = root
        root.title("尼彩 CBE 模拟器")
        root.configure(bg="#101014")
        icon = os.path.join(_HERE, "logo.png")
        if os.path.exists(icon):
            try:
                self._icon = tk.PhotoImage(file=icon)
                root.iconphoto(True, self._icon)
            except tk.TclError:
                pass
        self.session = None
        self.held = set()
        self.latched = 0
        self.mask = 0
        self.scale = 2
        self.fps = 30
        self.img = None
        self.running = False

        left = tk.Frame(root, bg="#101014")
        left.pack(side="left", fill="y", padx=8, pady=8)
        tk.Button(left, text="打开 .cbe", command=self.open_file).pack(fill="x")
        tk.Button(left, text="停止", command=self.stop).pack(fill="x", pady=(4, 8))
        tk.Label(left, text="游戏库", bg="#101014", fg="#9aa0a6").pack(anchor="w")
        self.listbox = tk.Listbox(left, width=26, height=22)
        self.listbox.pack(fill="both", expand=True)
        self.listbox.bind("<Double-Button-1>", self.run_selected)

        mid = tk.Frame(root, bg="#101014")
        mid.pack(side="left", padx=8, pady=8)
        self.canvas = tk.Canvas(mid, width=240 * self.scale, height=400 * self.scale,
                                bg="black", highlightthickness=0)
        self.canvas.pack()
        self.canvas.bind("<Button-1>", lambda e: self.touch(e, "down"))
        self.canvas.bind("<B1-Motion>", lambda e: self.touch(e, "move"))
        self.canvas.bind("<ButtonRelease-1>", lambda e: self.touch(e, "up"))
        self.status = tk.Label(mid, text="未加载模块", bg="#101014", fg="#9aa0a6")
        self.status.pack(anchor="w", pady=4)

        right = tk.Frame(root, bg="#101014")
        right.pack(side="left", fill="y", padx=8, pady=8)
        self.build_pad(right)

        root.bind("<KeyPress>", self.on_key_down)
        root.bind("<KeyRelease>", self.on_key_up)
        root.protocol("WM_DELETE_WINDOW", self.quit)
        self.refresh_library()

    def build_pad(self, parent):
        def key(p, kid, w=6):
            b = tk.Button(p, text=LABEL.get(kid, kid), width=w)
            b.bind("<ButtonPress-1>", lambda e: self.press(kid))
            b.bind("<ButtonRelease-1>", lambda e: self.release(kid))
            return b

        r = tk.Frame(parent, bg="#101014"); r.pack()
        key(r, "lsk", 10).pack(side="left", padx=2, pady=2)
        key(r, "rsk", 10).pack(side="left", padx=2, pady=2)
        for ids in (("", "up", ""), ("left", "ok", "right"), ("", "down", "")):
            r = tk.Frame(parent, bg="#101014"); r.pack()
            for kid in ids:
                if kid:
                    key(r, kid).pack(side="left", padx=2, pady=2)
                else:
                    tk.Frame(r, width=54, bg="#101014").pack(side="left")
        r = tk.Frame(parent, bg="#101014"); r.pack(pady=(6, 0))
        key(r, "call", 10).pack(side="left", padx=2)
        key(r, "end", 10).pack(side="left", padx=2)
        for ids in (("k1", "k2", "k3"), ("k4", "k5", "k6"),
                    ("k7", "k8", "k9"), ("star", "k0", "pound")):
            r = tk.Frame(parent, bg="#101014"); r.pack()
            for kid in ids:
                key(r, kid).pack(side="left", padx=2, pady=2)
        tk.Label(parent, bg="#101014", fg="#9aa0a6", justify="left",
                 text="键盘：WASD/方向键，J 或空格=OK\nK=左软键 L=右软键 U=呼叫 Esc=挂断\n"
                      "画面可直接点击，软键多数只吃触摸").pack(anchor="w", pady=8)

    def games_dir(self):
        d = os.path.join(paths.home(), "games")
        os.makedirs(d, exist_ok=True)
        return d

    def refresh_library(self):
        self.listbox.delete(0, "end")
        self.games = sorted(f for f in os.listdir(self.games_dir())
                            if f.lower().endswith(".cbe"))
        for g in self.games:
            self.listbox.insert("end", g)

    def open_file(self):
        p = filedialog.askopenfilename(filetypes=[("CBE 模块", "*.cbe *.CBE")])
        if not p:
            return
        dst = os.path.join(self.games_dir(), os.path.basename(p))
        if os.path.abspath(p) != os.path.abspath(dst):
            with open(p, "rb") as s, open(dst, "wb") as d:
                d.write(s.read())
        self.refresh_library()
        self.start(dst)

    def run_selected(self, _e=None):
        sel = self.listbox.curselection()
        if sel:
            self.start(os.path.join(self.games_dir(), self.games[sel[0]]))

    def start(self, path):
        self.stop()
        try:
            self.session = Session(path, audio=False).boot()
        except Exception as e:
            messagebox.showerror("启动失败", f"{type(e).__name__}: {e}")
            return
        w, h = self.session.size
        self.canvas.config(width=w * self.scale, height=h * self.scale)
        self.status.config(text=f"{os.path.basename(path)}  {w}x{h}")
        self.running = True
        self.tick()

    def stop(self):
        self.running = False
        if self.session:
            self.session.stop()
            self.session = None
        self.canvas.delete("all")
        self.img = None
        self.status.config(text="已停止")

    def press(self, kid):
        self.held.add(kid)
        self.sync_keys()

        if self.session and kid in ("lsk", "rsk"):
            self.session.soft_key("left" if kid == "lsk" else "right", True)

    def release(self, kid):
        self.held.discard(kid)
        self.sync_keys()

    def sync_keys(self):
        m = 0
        for kid in self.held:
            m |= KEYS.get(kid, 0)
        self.latched |= m & ~self.mask
        self.mask = m

    def on_key_down(self, e):
        kid = KEYBOARD.get(e.keysym)
        if kid:
            self.press(kid)

    def on_key_up(self, e):
        kid = KEYBOARD.get(e.keysym)
        if kid:
            self.release(kid)

    def touch(self, e, state):
        if self.session:
            self.session.set_touch(int(e.x / self.scale), int(e.y / self.scale), state)

    def tick(self):
        if not self.running or not self.session:
            return
        t0 = time.time()
        self.session.set_keys(self.mask | self.latched)
        self.latched = 0
        px = self.session.step()
        w, h = self.session.size
        self.img = tk.PhotoImage(data=to_ppm(px, w, h))
        if self.scale > 1:
            self.img = self.img.zoom(self.scale, self.scale)
        self.canvas.delete("all")
        self.canvas.create_image(0, 0, anchor="nw", image=self.img)
        for ev in self.session.take_events():
            if ev.get("kind") == "exit":
                self.stop()
                return
        delay = max(1, int(1000 / self.fps - (time.time() - t0) * 1000))
        self.root.after(delay, self.tick)

    def quit(self):
        self.stop()
        self.root.destroy()

def main():
    root = tk.Tk()
    app = App(root)
    if len(sys.argv) > 1:
        app.start(sys.argv[1])
    root.mainloop()

if __name__ == "__main__":
    main()
