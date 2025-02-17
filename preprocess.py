import os
import sys
import zstandard as zstd
import numpy as np
from concurrent.futures import ProcessPoolExecutor, as_completed
from zstandard.backend_cffi import ZstdCompressor

from globals import *

out_dir = "processed"

def process_file(file_path, out_file):
    try:
        cycle_count = PREP_SAMPLES

        with open(file_path, "rb") as compressed_file:
            dctx = zstd.ZstdDecompressor()
            with dctx.stream_reader(compressed_file) as reader:
                decompressed = reader.read()

        raw = np.clip(np.frombuffer(decompressed, dtype=np.uint64), 0, CEIL)
        raw = raw[::-1]

        rounds = raw.reshape(-1, PROFILER_SAMPLES_PER_RUN)
        runs_per_port = len(rounds)

        cycle_progress = np.cumsum(rounds, axis=1)
        cycle_latency = np.zeros((cycle_count, runs_per_port))
        for i in range(runs_per_port):
            indices = np.searchsorted(cycle_progress[i], np.arange(cycle_count) * PREP_SAMPLE_INTERVAL, side='left')
            indices = np.clip(indices, 0, cycle_progress[i].size - 1)
            cycle_latency.T[i] = rounds[i][indices]

        directory = os.path.dirname(out_file)
        if directory and not os.path.exists(directory):
            os.makedirs(directory)

        cctx = ZstdCompressor(level=3, threads=1)
        with open(out_file, "wb") as f:
            f.write(cctx.compress(cycle_latency.tobytes()))

        return True
    except Exception as e:
        print(f"{file_path}: {e}")
    return False

def load_zstd_arrays(base_path):
    tasks = []
    with ProcessPoolExecutor() as executor:
        for arch in os.listdir(base_path):
            arch_path = os.path.join(base_path, arch)
            if not os.path.isdir(arch_path):
                continue

            for alg in os.listdir(arch_path):
                alg_path = os.path.join(arch_path, alg)
                if not os.path.isdir(alg_path):
                    continue

                for file_name in os.listdir(alg_path):
                    if file_name.endswith(f"sumsof1.bin") and file_name.startswith(f"{arch}-"):
                        file_path = os.path.join(alg_path, file_name)
                        _, port, _ = file_name.split("-")
                        tasks.append(executor.submit(process_file, file_path, f"{out_dir}/{arch}/{alg}/{arch}-{port}.bin"))

        for _future in as_completed(tasks):
            pass

    return True

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <path to measurements folder>")
        exit(0)

    print("Processing measurements ... ", end="", flush=True)
    measurements = load_zstd_arrays(sys.argv[1])
    print("done")

