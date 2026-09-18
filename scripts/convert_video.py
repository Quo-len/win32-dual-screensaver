import os
import sys
import argparse
import struct
import shutil
import cv2
import numpy as np

def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert any video file into a compressed binary format (.bin) for the ASCII screensaver."
    )
    parser.add_argument(
        "-i", "--input",
        type=str,
        default=None,
        help="Path to input video file (e.g. bad-apple.mp4). If not specified, automatically finds the first video in the current folder."
    )
    parser.add_argument(
        "-o", "--output",
        type=str,
        default="bad_apple.bin",
        help="Name or path of the output binary file (default: bad_apple.bin)."
    )
    parser.add_argument(
        "-w", "--width",
        type=int,
        default=160,
        help="Horizontal resolution in characters (default: 160)."
    )
    parser.add_argument(
        "-H", "--height",
        type=int,
        default=60,
        help="Vertical resolution in characters (default: 60)."
    )
    parser.add_argument(
        "--fps",
        type=float,
        default=30.0,
        help="Target framerate (default: 30.0)."
    )
    parser.add_argument(
        "--levels",
        type=int,
        default=16,
        help="Grayscale quantization levels (default: 16, i.e. 0-15)."
    )
    parser.add_argument(
        "--invert",
        action="store_true",
        help="Invert video brightness."
    )
    parser.add_argument(
        "--auto-deploy",
        action="store_true",
        default=True,
        help="Automatically copy the output .bin to dual-screensaver build and project directories."
    )
    return parser.parse_args()

def find_default_video(search_dir):
    extensions = [".mp4", ".mkv", ".avi", ".mov", ".webm", ".flv", ".wmv"]
    # Check for bad-apple first
    for f in os.listdir(search_dir):
        if "bad" in f.lower() and "apple" in f.lower() and any(f.lower().endswith(ext) for ext in extensions):
            return os.path.join(search_dir, f)
    # Check for any video
    for f in os.listdir(search_dir):
        if any(f.lower().endswith(ext) for ext in extensions):
            return os.path.join(search_dir, f)
    return None

def main():
    args = parse_args()
    script_dir = os.path.dirname(os.path.abspath(__file__))

    video_path = args.input
    if not video_path:
        video_path = find_default_video(script_dir)
        if not video_path:
            video_path = find_default_video(os.path.dirname(script_dir))

    if not video_path or not os.path.exists(video_path):
        print(f"Error: Video file not found. Specify with -i <path_to_video>")
        sys.exit(1)

    print(f"==================================================")
    print(f" ASCII Video Converter for Dual-Screensaver")
    print(f"==================================================")
    print(f"Input video : {video_path}")
    print(f"Resolution  : {args.width}x{args.height} characters")
    print(f"Grayscale   : {args.levels} levels")
    print(f"Invert      : {args.invert}")

    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print(f"Error: Could not open video file {video_path}")
        sys.exit(1)

    src_fps = cap.get(cv2.CAP_PROP_FPS) or 30.0
    src_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT) or 0)
    target_fps = args.fps if args.fps > 0 else src_fps

    print(f"Source FPS  : {src_fps:.2f} (Target FPS: {target_fps:.2f})")
    print(f"Source Total: {src_frames} frames")
    print(f"Extracting & compressing frames...")

    w, h = args.width, args.height
    levels = max(2, min(256, args.levels))
    divisor = 256 // levels

    # Step ratio if target_fps differs from src_fps
    frame_step = src_fps / target_fps if target_fps > 0 else 1.0

    rle_data = bytearray()
    frame_offsets = []

    src_idx = 0.0
    processed_frames = 0

    while True:
        target_src_frame = int(src_idx)
        if src_frames > 0 and target_src_frame >= src_frames:
            break

        # Fast sequential reading when step == 1.0
        if abs(frame_step - 1.0) < 0.001:
            ret, frame = cap.read()
        else:
            cap.set(cv2.CAP_PROP_POS_FRAMES, target_src_frame)
            ret, frame = cap.read()

        if not ret:
            break

        # Convert to grayscale
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        if args.invert:
            gray = 255 - gray

        # Resize to character dimensions
        resized = cv2.resize(gray, (w, h), interpolation=cv2.INTER_AREA)

        # Quantize to 0 .. (levels - 1)
        quantized = np.clip(resized // divisor, 0, levels - 1).astype(np.uint8).flatten()

        # Run-length encode (RLE) the frame
        frame_offsets.append(len(rle_data))

        last_val = int(quantized[0])
        run_len = 1
        for val in quantized[1:]:
            v = int(val)
            if v == last_val and run_len < 255:
                run_len += 1
            else:
                rle_data.extend([run_len, last_val])
                last_val = v
                run_len = 1
        rle_data.extend([run_len, last_val])

        processed_frames += 1
        src_idx += frame_step

        if processed_frames % 500 == 0:
            print(f"  Processed {processed_frames} frames ({len(rle_data) / 1024 / 1024:.2f} MB compressed)...")

    cap.release()
    print(f"Extraction complete. Total frames processed: {processed_frames}")

    # Build binary file
    # Header:
    # 0x00: magic 'BAP1' (4 bytes)
    # 0x04: uint32 total_frames
    # 0x08: uint16 width
    # 0x0A: uint16 height
    # 0x0C: uint16 fps
    # 0x0E: uint16 levels
    # 0x10: uint32 data_offset (offset from start of file to rle_data)
    # 0x14: reserved 12 bytes
    # 0x20: uint32 frame_offsets[total_frames]
    # Followed immediately by rle_data

    header_size = 32
    offsets_size = processed_frames * 4
    data_offset = header_size + offsets_size

    out_path = args.output
    if not os.path.isabs(out_path):
        out_path = os.path.join(script_dir, out_path)

    print(f"Writing binary file to: {out_path} ...")
    with open(out_path, "wb") as f:
        header = struct.pack(
            "<4sIHHHHI12s",
            b"BAP1",
            processed_frames,
            w,
            h,
            int(round(target_fps)),
            levels,
            data_offset,
            b"\x00" * 12
        )
        assert len(header) == 32, f"Header size is {len(header)}, expected 32"
        f.write(header)

        # Write frame offsets table
        for offset in frame_offsets:
            f.write(struct.pack("<I", offset))

        # Write RLE stream
        f.write(rle_data)

    final_size_mb = os.path.getsize(out_path) / (1024 * 1024)
    print(f"Done! Final file size: {final_size_mb:.2f} MB")

    if args.auto_deploy:
        repo_root = os.path.abspath(os.path.join(script_dir, ".."))
        deploy_dirs = [
            os.path.join(repo_root, "dual-screensaver"),
            os.path.join(repo_root, "dual-screensaver", "x64", "Debug"),
            os.path.join(repo_root, "dual-screensaver", "x64", "Release"),
        ]
        for d in deploy_dirs:
            if os.path.exists(d):
                dest = os.path.join(d, os.path.basename(out_path))
                shutil.copy2(out_path, dest)
                print(f"  -> Deployed to {dest}")

    print("\nConversion successfully completed!")

if __name__ == "__main__":
    main()
