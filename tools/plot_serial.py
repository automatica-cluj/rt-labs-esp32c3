#!/usr/bin/env python3
"""Plot the CSV block that periodic_jitter prints on key 'p'.

Save the serial output to a file first, for example by copying it out of the
serial monitor, or with

    arduino-cli monitor -p PORT --config baudrate=115200 | tee run.log

then

    python3 tools/plot_serial.py run.log            # opens a window
    python3 tools/plot_serial.py run.log out.png    # writes a PNG

If the log holds several CSV blocks, each one is plotted as its own run.
"""

import sys

import matplotlib.pyplot as plt


def read_blocks(path):
    blocks, cur = [], None
    with open(path, errors="replace") as f:
        for line in f:
            line = line.strip()
            if line.startswith("csv: k,"):
                cur = []
            elif line.startswith("csv: end"):
                if cur:
                    blocks.append(cur)
                cur = None
            elif cur is not None and line.count(",") == 2:
                k, wake, interval = line.split(",")
                cur.append((int(k), int(wake), int(interval)))
    return blocks


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    blocks = read_blocks(sys.argv[1])
    if not blocks:
        sys.exit("no 'csv:' block found; press p in the serial monitor after a run")

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 4))
    for i, b in enumerate(blocks):
        ks = [r[0] for r in b[1:]]
        iv = [r[2] for r in b[1:]]
        ax1.plot(ks, iv, ".", markersize=3, label=f"run {i + 1}")
        ax2.hist(iv, bins=50, alpha=0.6, label=f"run {i + 1}")
    ax1.set_xlabel("period k")
    ax1.set_ylabel("interval between wake-ups (us)")
    ax2.set_xlabel("interval (us)")
    ax2.set_ylabel("count")
    ax2.set_yscale("log")
    for ax in (ax1, ax2):
        ax.legend()
        ax.grid(alpha=0.3)
    fig.tight_layout()
    if len(sys.argv) > 2:
        fig.savefig(sys.argv[2], dpi=120)
        print("wrote", sys.argv[2])
    else:
        plt.show()


if __name__ == "__main__":
    main()
