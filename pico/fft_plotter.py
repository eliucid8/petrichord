#!/usr/bin/env python3
"""
pico_fft_bins_plotter.py

Real-time plot of frequency bin amplitudes from a Raspberry Pi Pico
using the pico_fft library.

EXPECTED SERIAL FORMAT FROM PICO (one line per FFT frame):

    BINS a0 a1 a2 ... aN

where a0..aN are amplitudes (floats or ints) for each pitch bin.

Usage:
    python pico_fft_bins_plotter.py --port /dev/tty.usbmodemXXXX
    # or on Windows:
    python pico_fft_bins_plotter.py --port COM3
"""

import argparse
import sys
import time

import numpy as np
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation


def parse_args():
    parser = argparse.ArgumentParser(description="Plot Pico FFT bin amplitudes in real time.")
    parser.add_argument(
        "--port",
        required=True,
        help="Serial port for the Pico (e.g. /dev/tty.usbmodemXXXX, /dev/ttyACM0, COM3)"
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="Baud rate (default: 115200)"
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=1.0,
        help="Serial read timeout in seconds (default: 1.0)"
    )
    return parser.parse_args()


def open_serial(port: str, baud: int, timeout: float) -> serial.Serial:
    try:
        ser = serial.Serial(port, baudrate=baud, timeout=timeout)
        # Give the Pico a moment after opening the port
        time.sleep(2.0)
        return ser
    except serial.SerialException as e:
        print(f"ERROR: Could not open serial port {port}: {e}")
        sys.exit(1)


def read_bins_from_serial(ser: serial.Serial):
    """
    Waits for a line starting with 'BINS' and returns a list of amplitudes (floats).

    Line format:
        BINS a0 a1 a2 ... aN
    """
    while True:
        line_bytes = ser.readline()
        if not line_bytes:
            # timeout with no data
            continue

        try:
            line = line_bytes.decode("utf-8", errors="ignore").strip()
        except UnicodeDecodeError:
            continue

        if not line:
            continue

        # Only care about lines starting with BINS
        if not line.startswith("BINS"):
            continue

        parts = line.split()
        if len(parts) <= 1:
            # No amplitudes
            continue

        try:
            values = [float(x) for x in parts[1:]]
            return values
        except ValueError:
            # Malformed line; skip and try again
            continue


def main():
    args = parse_args()
    ser = open_serial(args.port, args.baud, args.timeout)

    print(f"Opened {args.port} at {args.baud} baud.")
    print("Waiting for first BINS line from Pico...")

    # Get one frame to determine number of bins
    first_bins = read_bins_from_serial(ser)
    num_bins = len(first_bins)
    print(f"Detected {num_bins} bins.")

    # --- Matplotlib setup ---
    fig, ax = plt.subplots()
    x = np.arange(num_bins)  # bin indices; you can relabel later with note names if you want
    bars = ax.bar(x, first_bins)

    ax.set_xlabel("Bin index")
    ax.set_ylabel("Amplitude")
    ax.set_title("Pico FFT Bin Amplitudes")
    ax.set_xlim(-0.5, num_bins - 0.5)

    # Set a reasonable initial y-limit
    max_amp = max(first_bins) if first_bins else 1.0
    if max_amp <= 0:
        max_amp = 1.0
    ax.set_ylim(0, max_amp * 1.2)

    # Optional: grid
    ax.grid(True, axis="y", linestyle="--", alpha=0.3)

    def update(frame):
        """Animation callback: read one BINS line and update the bar heights."""
        bins = read_bins_from_serial(ser)
        if len(bins) != num_bins:
            # If the count changes unexpectedly, just ignore this frame
            return bars

        max_val = 0.0
        for bar, val in zip(bars, bins):
            bar.set_height(val)
            if val > max_val:
                max_val = val

        # Adjust y-axis dynamically so we can see peaks clearly
        if max_val <= 0:
            max_val = 1.0
        ax.set_ylim(0, max_val * 1.2)

        return bars

    ani = animation.FuncAnimation(
        fig,
        update,
        interval=50,     # ms between updates; tune as you like
        blit=False
    )

    try:
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()
        print("Serial port closed.")


if __name__ == "__main__":
    main()
