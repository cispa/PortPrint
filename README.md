# PortPrint

This repository contains a demonstration of the fingerprinting approach described in the paper "PortPrint: PortPrint: Identifying Inaccessible Code with Port Contention" by Hornetz and Schwarz.
It records the port contention profile of two AES implementations from Linux and OpenSSL, both using the AES-NI extensions.
The final output is a plot of the contention profiles, which should show clear differences between the two implementations.

## Prerequisites
To run this demo, you should have a CPU based on Intel's Skylake or Golden Cove architecture, or on AMD's Zen 3 architecture.
The demo is meant to run on Linux.

## Setup
#### Building the Recorder
To build the recorder, you will need cmake and libzstd. On Debian-based distributions, you can install them with the following command:
```shell
sudo apt-get install cmake libzstd-dev
```
Then, execute the following command to start building:
```shell
cmake -B build -S . && cmake --build build
```

#### Setting up the Python Virtual Environment
To avoid conflicts in the Python dependencies, you should set up a virtual environment with the package dependencies.
If you have the `python3-virtualenv` package installed, you can do this with following commands:
```shell
python -m virtualenv venv
source venv/bin/activate
pip install -r requirements.txt
```

## Usage
### Step 1: Recording Timing Traces
The first step is to record timing information by invoking the recorder with your CPU's architecture.
For example, `./recorder Skylake` will record timing information with contention primitives for Skylake.
Supported values are `Skylake`, `AlderLakeP` for CPU's based on Golden Cove, and `Zen3`.
The timing traces will be saved into the `measurements` folder in you working directory.

### Step 2: Pre-processing the traces
Before you can inspect the contention profile, the traces from the previous step must go through a pre-processing step.
You can start this by running the `preprocess.py` script, passing the path to the `measurements` as an argument.
The pre-processed traces will be stored to the `processed` folder.

### Step 3: Plotting the traces
Finally, you can plot the traces by running the `plot.py` script. If everything worked, there should be obvious differences between the plots of the Linux and OpenSSL implementations.

## Note
The files `src/victims/aesni-linux.S` and `src/victims/aesni-openssl.s` contain code taken from Linux and OpenSSL, and are published under their respective licenses.
