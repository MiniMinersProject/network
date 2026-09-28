# MINI CPU Reference Miner

**Status:** Experimental — MINI Closed Testnet

This directory contains the public Python CPU reference miner for MINI.

The reference miner is intended to demonstrate that MINI mining is open to general-purpose computers and that participation does not require purchasing Mini Miners hardware or proprietary software.

## What It Does

The reference miner:

- Implements MiniPOW1
- Performs a MiniPOW1 self-test at startup
- Generates a persistent P-256 miner identity
- Stores the private identity key locally
- Authenticates to the MINI mining pool
- Requests mining jobs
- Performs MiniPOW work
- Submits valid shares
- Displays accepted rewards
- Reports MiniPOW/s
- Reconnects automatically after connection failures

## Requirements

- Python 3
- `cryptography` Python package
- Internet connection

Install the required Python package with:

```bash
pip install cryptography
```

## Setup

Download or clone this repository.

The CPU miner is:

```text
miner.py
```

Before running it, open `miner.py` and configure the miner information near the top of the file.

At minimum, set:

```python
USERNAME = "your-account-or-testnet-name"
MINER_NAME = "your-miner-name"
```

Do not use the same miner identity directory for multiple independently registered miners.

## Run

From the CPU miner directory:

```bash
python miner.py
```

On Windows, depending on your Python installation:

```text
py miner.py
```

The miner will first run the built-in MiniPOW1 known-answer test.

If the test passes, it will load or create a persistent miner identity and connect to the MINI Closed Testnet mining pool.

## Persistent Miner Identity

The first time the miner runs, it creates a P-256 private key in:

```text
identity/miner_private_key.pem
```

That private key represents the persistent identity of that miner.

**Keep it private.**

Do not publish it, upload it to GitHub, send it to another person, or include it when sharing the miner source.

Only the corresponding public key is sent to the MINI mining infrastructure.

Deleting the identity directory causes the miner to create a new identity the next time it starts.

## MiniPOW1 Self-Test

The miner contains a known MiniPOW1 test vector.

Before connecting to the pool, it computes MiniPOW1 using the known challenge and nonce and compares the result with the expected digest.

If the result does not match, the miner stops.

This helps detect incompatible changes or broken MiniPOW implementations before invalid work is submitted to the network.

## Performance

Performance depends heavily on CPU architecture, Python version and system conditions.

One MINI development system using an AMD Ryzen 9 6900HX has produced approximately 258–288 MiniPOW/s during experimental CPU mining.

That result is a development measurement, not a guaranteed performance level.

Current power and benchmark information is published at:

https://miniminers.net/power.html

## Adaptive Difficulty

A faster miner does not simply receive the same work as a slower miner.

MINI uses adaptive difficulty to assign work according to miner performance.

This allows dramatically different devices — including ESP32-class microcontrollers and general-purpose CPUs — to participate in the same mining environment.

## Reference Software vs. Commercial Software

This is public reference mining software.

It is intended to provide the functionality necessary to participate in MINI mining and to give developers working code they can inspect, modify and port.

Mini Miners also develops separate proprietary software and commercial device firmware containing additional convenience and management features.

Those proprietary features are not required to mine MINI.

## Closed Testnet Warning

MINI is currently under active development.

Protocol details, pool addresses, authentication behavior and other network parameters may change as the project moves toward LIVENET.

Expect updates.

## Technical Documentation

MINI technical site:

https://miniminers.net

MiniPOW1:

https://github.com/MiniMinersProject/network/blob/main/docs/MINIPOW1.md

LIVENET:

https://github.com/MiniMinersProject/network/blob/main/docs/LIVENET.md

Roadmap:

https://github.com/MiniMinersProject/network/blob/main/docs/ROADMAP.md

Beginner MINI site:

https://miniminers.org

---

**MINI — Made for beginners. Open to everyone. Built for low power.**
