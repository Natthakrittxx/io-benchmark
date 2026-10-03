# HW1 charts: reads ex1.csv and ex2.csv, writes ex1_chart.png and ex2_chart.png.
# Usage: python3 plot.py   (needs matplotlib: pip install matplotlib)
import csv, statistics as st
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FixedLocator, FuncFormatter, NullLocator

HW = Path(__file__).resolve().parent
INK, INK2, MUTED, GRID, AXIS = "#0b0b0b", "#52514e", "#898781", "#e1e0d9", "#c3c2b7"
BLUE, ORANGE = "#2a78d6", "#eb6834"

# ---------------- data ----------------
rows = list(csv.DictReader(open(HW / "ex1.csv")))
files = sorted({r["file"] for r in rows})
blocks, avg, med = [], [], []
for f in files:
    v = [float(r["ms"]) for r in rows if r["file"] == f]
    blocks.append(int(next(r["block_bytes"] for r in rows if r["file"] == f)))
    avg.append(sum(v) / len(v))
    med.append(st.median(v))
ex2 = {r["test"]: (float(r["ms"]), float(r["MB_per_s"])) for r in csv.DictReader(open(HW / "ex2.csv"))}


def hsize(b):
    return f"{b} B" if b < 1024 else f"{b // 1024} KB"


# ---------------- charts ----------------
plt.rcParams.update({"font.family": ["Arial", "DejaVu Sans"], "font.size": 9, "text.color": INK2,
                     "axes.labelcolor": INK2, "xtick.color": INK2, "ytick.color": INK2,
                     "axes.edgecolor": AXIS, "axes.linewidth": 0.8})


def style_axes(ax):
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    ax.grid(axis="y", color=GRID, linewidth=0.6)
    ax.set_axisbelow(True)
    ax.tick_params(length=0)


# Experiment 1: write time vs block size, log-log.
fig, ax = plt.subplots(figsize=(7.2, 3.8), dpi=220)
ax.plot(blocks, avg, color=BLUE, lw=1.6, marker="o", ms=4.5, label=f"Average of {len(rows) // len(files)} phases")
ax.plot(blocks, med, color=ORANGE, lw=1.6, ls="--", marker="s", ms=4, label=f"Median of {len(rows) // len(files)} phases")
ax.set_xscale("log", base=2)
ax.set_yscale("log")
ax.xaxis.set_major_locator(FixedLocator(blocks))
ax.xaxis.set_minor_locator(NullLocator())
ax.xaxis.set_major_formatter(FuncFormatter(lambda x, _: hsize(int(round(x)))))
plt.setp(ax.get_xticklabels(), rotation=45, ha="right")
yt = [0.01, 0.1, 1, 10, 100, 1000]
ax.yaxis.set_major_locator(FixedLocator(yt))
ax.yaxis.set_minor_locator(NullLocator())
ax.yaxis.set_major_formatter(FuncFormatter(lambda y, _: f"{y:g}"))
ax.set_ylim(0.01, 1000)
ax.set_xlabel("Block size (log scale)")
ax.set_ylabel("Time to write 256 KB (ms, log scale)")
style_axes(ax)
ax.grid(axis="x", color=GRID, linewidth=0.4)
ax.annotate(f"{avg[0]:.0f} ms", (blocks[0], avg[0]), xytext=(8, 4), textcoords="offset points", color=INK, fontsize=9)
ax.annotate(f"{med[-1]:.3f} ms", (blocks[-1], med[-1]), xytext=(-10, -14), textcoords="offset points",
            color=INK, fontsize=9, ha="right")
# Label the block size whose average is pulled furthest above its median, if a slow phase did that.
i = max(range(len(blocks)), key=lambda k: avg[k] / med[k])
if avg[i] > 2 * med[i]:
    ax.annotate("one slow phase\n(outlier)", (blocks[i], avg[i]), xytext=(-70, 30), textcoords="offset points",
                color=INK2, fontsize=8, arrowprops=dict(arrowstyle="-", color=MUTED, lw=0.8))
ax.legend(frameon=False, loc="upper right")
fig.tight_layout()
fig.savefig(HW / "ex1_chart.png", facecolor="white")
plt.close(fig)

# Experiment 2: sequential vs random read time.
fig, ax = plt.subplots(figsize=(5.2, 3.4), dpi=220)
names = ["Sequential", "Random"]
ms = [ex2["sequential"][0], ex2["random"][0]]
bars = ax.bar(names, ms, width=0.45, color=BLUE)
for b, (m, mbs) in zip(bars, [ex2["sequential"], ex2["random"]]):
    ax.text(b.get_x() + b.get_width() / 2, m + max(ms) * 0.025, f"{m:.1f} ms\n{mbs:,.0f} MB/s", ha="center",
            va="bottom", color=INK, fontsize=9)
ax.set_ylim(0, max(ms) * 1.28)
ax.set_ylabel("Time to read 256 MB (ms)")
style_axes(ax)
fig.tight_layout()
fig.savefig(HW / "ex2_chart.png", facecolor="white")
plt.close(fig)
