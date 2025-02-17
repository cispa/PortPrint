import os
from typing import Tuple, List
import zstandard as zstd
import numpy as np
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor, as_completed
from copy import deepcopy
import matplotlib.pyplot as plt

from globals import *

def process_file(file_path, arch, alg, file_name, cutoff):
    num_samples = PREP_SAMPLES
    _, port = file_name.split("-")
    port = port.split(".")[0]

    with open(file_path, "rb") as compressed_file:
        dctx = zstd.ZstdDecompressor()
        with dctx.stream_reader(compressed_file) as reader:
            decompressed = reader.read()

    array = np.frombuffer(decompressed, dtype=np.float64)
    array = array.reshape(num_samples, -1)
    array = np.mean(array, axis=1)

    return array, arch, alg, port

def load_zstd_arrays(base_path, algs=None, archs=None, ports=None, cutoff=0):
    # Nested defaultdict to store arrays: dict[arch][alg][port]
    arrays_dict = defaultdict(lambda: defaultdict(dict))

    tasks = []
    with ProcessPoolExecutor() as executor:
        for arch in os.listdir(base_path):
            if archs and arch not in archs:
                continue

            arch_path = os.path.join(base_path, arch)
            if not os.path.isdir(arch_path):
                continue

            for alg in os.listdir(arch_path):
                if algs and alg not in algs:
                    continue

                alg_path = os.path.join(arch_path, alg)
                if not os.path.isdir(alg_path):
                    continue

                for file_name in os.listdir(alg_path):
                    # Check if the file follows the <arch>-<port>-4096of1.bin pattern
                    if file_name.endswith(".bin") and file_name.startswith(f"{arch}-"):
                        _, port = file_name.split("-")
                        port = port.split(".")[0]
                        if ports and port not in ports:
                            continue
                        file_path = os.path.join(alg_path, file_name)
                        tasks.append(executor.submit(process_file, deepcopy(file_path), deepcopy(arch), deepcopy(alg), deepcopy(file_name), cutoff))

        for future in as_completed(tasks):
            result = future.result()
            if result:
                array, arch, alg, port = result
                arrays_dict[arch][alg][port] = array

    return dict(arrays_dict)

def plot_list(measurements: List[Tuple[str, int, int, List[Tuple[str, np.array]]]]):
    fig, axs = plt.subplots(len(measurements), figsize=(8, 6))
    for i, (title, windup, num, vals) in enumerate(measurements):
        ax = axs[i]
        for (port, means) in sorted(vals, key=lambda v: v[0]):
            offset = windup
            means = means[offset:offset+num]
            ax.plot(np.arange(means.size) * PREP_SAMPLE_INTERVAL, means - np.min(means), label=port)
        ax.set_title(title)
        ax.grid()
        ax.legend()
    plt.tight_layout()
    plt.show()

def cyc_to_samples(i :int) -> int:
    return int(i / PREP_SAMPLE_INTERVAL)

def plot_cs1(measurements, alg_a, alg_b):
    plot_cycles = 12000
    plot_samples = cyc_to_samples(plot_cycles)

    arch = list(measurements.keys())[0]

    plots = [
        (f"{alg_a} ({arch})", cyc_to_samples(300), plot_samples, list(measurements[arch][alg_a].items())),
        (f"{alg_b} ({arch})", cyc_to_samples(300), plot_samples, list(measurements[arch][alg_b].items())),
    ]
    plot_list(plots)


if __name__ == "__main__":
    print("Loading measurements ... ", end="", flush=True)

    alg_a = "AES-NI-OpenSSL"
    alg_b = "AES-NI-Linux"
    measurements = load_zstd_arrays("./processed", algs=[alg_a, alg_b])
    print("done")

    plot_cs1(measurements, alg_a, alg_b)
