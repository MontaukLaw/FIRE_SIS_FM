import argparse
import collections
import sys

import matplotlib.animation as animation
import matplotlib.pyplot as plt
import serial


HEADER = b"\xA5\x5A"
TYPE_RAW = 0x01
TAIL = b"\r\n"
RAW_FRAME_SIZE = 11


def parse_args():
    parser = argparse.ArgumentParser(description="Plot GSENSOR raw/delta data and event result from UART.")
    parser.add_argument("--port", required=True, help="Serial port, e.g. COM5")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    parser.add_argument("--samples", type=int, default=80, help="Points kept on screen")
    parser.add_argument("--ylim", type=int, default=512, help="Y-axis limit in raw LSB")
    parser.add_argument("--dylim", type=int, default=100, help="Y-axis limit in delta LSB")
    return parser.parse_args()


def extract_frames(buffer):
    raw_frames = []

    while True:
        start = buffer.find(HEADER)
        if start < 0:
            if len(buffer) > 0 and buffer[-1] == HEADER[0]:
                buffer[:] = buffer[-1:]
            else:
                buffer.clear()
            break

        if start > 0:
            del buffer[:start]

        if len(buffer) < RAW_FRAME_SIZE:
            break

        frame = bytes(buffer[:RAW_FRAME_SIZE])

        if frame[0:2] != HEADER or frame[9:11] != TAIL:
            del buffer[0]
            continue

        if frame[2] != TYPE_RAW:
            del buffer[:RAW_FRAME_SIZE]
            continue

        x = int.from_bytes(frame[3:5], byteorder="little", signed=True)
        y = int.from_bytes(frame[5:7], byteorder="little", signed=True)
        z = int.from_bytes(frame[7:9], byteorder="little", signed=True)
        raw_frames.append((x, y, z))
        del buffer[:RAW_FRAME_SIZE]

    return raw_frames


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
    latest_event_text = "RAW frame mode"
    prev_xyz = None

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
        nonlocal sample_idx, latest_event_text, prev_xyz

        chunk = ser.read(ser.in_waiting or RAW_FRAME_SIZE)
        if chunk:
            buffer.extend(chunk)
            raw_frames = extract_frames(buffer)

            for x, y, z in raw_frames:
                if prev_xyz is None:
                    dx = dy = dz = 0
                else:
                    dx = x - prev_xyz[0]
                    dy = y - prev_xyz[1]
                    dz = z - prev_xyz[2]

                prev_xyz = (x, y, z)
                ts.append(sample_idx)
                xs.append(x)
                ys.append(y)
                zs.append(z)
                dxs.append(dx)
                dys.append(dy)
                dzs.append(dz)
                sample_idx += 1

            latest_event_text = f"RAW frame mode | samples={sample_idx}"

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
