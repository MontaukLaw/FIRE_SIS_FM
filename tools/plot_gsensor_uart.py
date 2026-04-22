import argparse
import collections
import struct
import sys

import matplotlib.animation as animation
import matplotlib.pyplot as plt
import serial


HEADER = b"\xA5\x5A"
TYPE_RAW = 0x01
TYPE_EVENT = 0x02
TAIL = b"\r\n"
RAW_FRAME_SIZE = 17
EVENT_FRAME_SIZE = 13

FAIL_PEAK = 0x01
FAIL_ENERGY = 0x02
FAIL_ACTIVE = 0x04
FAIL_REBOUND = 0x08
FAIL_COOLDOWN = 0x10


def parse_args():
    parser = argparse.ArgumentParser(description="Plot GSENSOR raw/delta data and event result from UART.")
    parser.add_argument("--port", required=True, help="Serial port, e.g. COM5")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    parser.add_argument("--samples", type=int, default=80, help="Points kept on screen")
    parser.add_argument("--ylim", type=int, default=512, help="Y-axis limit in raw LSB")
    parser.add_argument("--dylim", type=int, default=100, help="Y-axis limit in delta LSB")
    return parser.parse_args()


def fail_mask_to_text(fail_mask):
    if fail_mask == 0:
        return "OK"

    reasons = []
    if fail_mask & FAIL_PEAK:
        reasons.append("peak")
    if fail_mask & FAIL_ENERGY:
        reasons.append("energy")
    if fail_mask & FAIL_ACTIVE:
        reasons.append("active")
    if fail_mask & FAIL_REBOUND:
        reasons.append("rebound")
    if fail_mask & FAIL_COOLDOWN:
        reasons.append("cooldown")
    return ",".join(reasons)


def extract_frames(buffer):
    raw_frames = []
    event_frames = []

    while len(buffer) >= 5:
        if buffer[0:2] != HEADER:
            del buffer[0]
            continue

        frame_type = buffer[2]

        if frame_type == TYPE_RAW:
            if len(buffer) < RAW_FRAME_SIZE:
                break
            if buffer[15:17] != TAIL:
                del buffer[0]
                continue
            raw_frames.append(struct.unpack("<hhhhhh", buffer[3:15]))
            del buffer[:RAW_FRAME_SIZE]
            continue

        if frame_type == TYPE_EVENT:
            if len(buffer) < EVENT_FRAME_SIZE:
                break
            if buffer[11:13] != TAIL:
                del buffer[0]
                continue
            success, fail_mask, peak_any, energy, active_samples, rebound = struct.unpack("<BBhHBB", buffer[3:11])
            event_frames.append((success, fail_mask, peak_any, energy, active_samples, rebound))
            del buffer[:EVENT_FRAME_SIZE]
            continue

        del buffer[0]

    return raw_frames, event_frames


def main():
    args = parse_args()

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.05)
    except serial.SerialException as exc:
        print(f"Open serial failed: {exc}", file=sys.stderr)
        return 1

    ts = collections.deque(maxlen=args.samples)
    xs = collections.deque(maxlen=args.samples)
    ys = collections.deque(maxlen=args.samples)
    zs = collections.deque(maxlen=args.samples)
    dxs = collections.deque(maxlen=args.samples)
    dys = collections.deque(maxlen=args.samples)
    dzs = collections.deque(maxlen=args.samples)
    buffer = bytearray()
    sample_idx = 0
    latest_event_text = "No event yet"

    fig, (ax_raw, ax_delta) = plt.subplots(2, 1, sharex=True)

    line_x, = ax_raw.plot([], [], label="X")
    line_y, = ax_raw.plot([], [], label="Y")
    line_z, = ax_raw.plot([], [], label="Z")
    line_dx, = ax_delta.plot([], [], label="dX")
    line_dy, = ax_delta.plot([], [], label="dY")
    line_dz, = ax_delta.plot([], [], label="dZ")

    event_text = ax_raw.text(0.01, 0.96, latest_event_text, transform=ax_raw.transAxes, va="top",
                             bbox={"boxstyle": "round", "facecolor": "white", "alpha": 0.8})

    ax_raw.set_title("GSENSOR Raw / Delta Waveform")
    ax_raw.set_ylabel("Raw")
    ax_raw.set_ylim(-args.ylim, args.ylim)
    ax_raw.grid(True, alpha=0.3)
    ax_raw.legend(loc="upper right")

    ax_delta.set_xlabel("Sample")
    ax_delta.set_ylabel("Delta")
    ax_delta.set_ylim(-args.dylim, args.dylim)
    ax_delta.grid(True, alpha=0.3)
    ax_delta.legend(loc="upper right")

    def update(_frame):
        nonlocal sample_idx, latest_event_text

        chunk = ser.read(ser.in_waiting or RAW_FRAME_SIZE)
        if chunk:
            buffer.extend(chunk)
            raw_frames, event_frames = extract_frames(buffer)

            for x, y, z, dx, dy, dz in raw_frames:
                ts.append(sample_idx)
                xs.append(x)
                ys.append(y)
                zs.append(z)
                dxs.append(dx)
                dys.append(dy)
                dzs.append(dz)
                sample_idx += 1

            for success, fail_mask, peak_any, energy, active_samples, rebound in event_frames:
                latest_event_text = (
                    f"{'PASS' if success else 'FAIL'} "
                    f"reason={fail_mask_to_text(fail_mask)} "
                    f"peak={peak_any} energy={energy} active={active_samples} rebound={rebound}"
                )
                print(latest_event_text)

        if ts:
            line_x.set_data(ts, xs)
            line_y.set_data(ts, ys)
            line_z.set_data(ts, zs)
            line_dx.set_data(ts, dxs)
            line_dy.set_data(ts, dys)
            line_dz.set_data(ts, dzs)
            x_end = ts[-1] if ts[-1] > ts[0] else ts[0] + 1
            ax_raw.set_xlim(ts[0], x_end)

        event_text.set_text(latest_event_text)
        return line_x, line_y, line_z, line_dx, line_dy, line_dz, event_text

    ani = animation.FuncAnimation(fig, update, interval=30, blit=False, cache_frame_data=False)
    try:
        plt.show()
    finally:
        del ani
        ser.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
