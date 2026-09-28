# MINI ESP32 Reference Miner

**Status:** Experimental — MINI Closed Testnet

This directory contains the public ESP32 reference miner for MINI.

It provides the basic functionality necessary to configure a supported ESP32, connect to the MINI mining network, perform MiniPOW1, and submit mining shares.

**You do not need to purchase a Mini Miners device to mine MINI.**

## What This Reference Miner Includes

- MiniPOW1
- MiniPOW1 startup self-test
- Persistent P-256 miner identity
- MINI pool discovery
- Pool authentication
- Mining job handling
- Share submission
- Wi-Fi configuration
- Persistent configuration
- Automatic mining reconnection
- Mining watchdog/recovery
- Onboard LED status indication
- Accepted-share LED indication

## What It Does Not Include

This public reference miner intentionally does not contain the proprietary Mini Miners commercial firmware features.

It does not include:

- Mini Miners automatic OTA firmware updating
- Commercial device-management features
- Commercial provisioning features
- OLED/display support

These features are not required to participate in MINI mining.

## Tested Hardware

The public MINI ESP32 reference miner has been successfully compiled, flashed and used to mine on the MINI Closed Testnet on:

- ESP32 DevKit-style boards
- NodeMCU-32S-style ESP32 boards
- **ESP32-C3 Super Mini**

### ESP32-C3 Super Mini

The ESP32-C3 Super Mini has been physically tested with the public reference miner using:

```text
Board: ESP32C3 Dev Module
Arduino ESP32 core: 3.3.11
USB CDC On Boot: Enabled
```

Four ESP32-C3 Super Mini devices have been successfully brought online during development testing.

Initial observed mining performance was approximately:

```text
12.63–12.65 MiniPOW/s
```

For comparison, the established standard ESP32 test fleet was simultaneously operating at approximately:

```text
6.93–6.94 MiniPOW/s
```

These are real-world Closed Testnet observations, not guaranteed performance specifications.

Performance can vary with hardware, compiler version, configuration, firmware changes and operating conditions.

The ESP32-C3 results are particularly interesting because the C3 is a single-core RISC-V device and is running the same MiniPOW1 algorithm used by the other reference miners.

### ESP32-C3 Flashing Note

When using the ESP32-C3 Super Mini with its native USB connection, serial output may require:

```text
Tools → USB CDC On Boot → Enabled
```

The Serial Monitor is currently configured for:

```text
115200 baud
```

If the C3 repeatedly connects and disconnects before flashing, manually entering its bootloader/download mode may be necessary.

Other ESP32-family boards may also work, but they should not be considered tested until they have been successfully compiled, flashed, connected and observed mining on the Closed Testnet.

## Real-World Testing

MINI publishes real-world hardware measurements as testing progresses.

Current testing includes:

- MiniPOW/s
- Adaptive difficulty behavior
- Device temperature
- USB power consumption
- Energy consumption over time
- MiniPOW/s per watt
- Long-duration stability

Preliminary testing has shown highly repeatable MiniPOW performance within each currently tested hardware class.

More precise power measurements are underway using dedicated inline USB measurement equipment.

Power and efficiency results will be published as measurements are validated.

Technical power/testing page:

https://miniminers.net/power.html

## Arduino Environment

The reference miner is intended to be compiled with the Arduino ESP32 environment.

Current MINI development has been performed with:

```text
Arduino ESP32 core 3.3.11
```

Because MINI is under active development, other versions may work but are not yet guaranteed.

## Source

The miner source is:

```text
miner.ino
```

Open this file in the Arduino IDE.

## Before Flashing

Review the configuration section near the beginning of `miner.ino`.

The public reference build does not contain personal Wi-Fi credentials or private Mini Miners production configuration.

The miner is designed to obtain its user configuration through its setup process and store persistent configuration locally.

## First Boot

On first boot, the miner does not have your Wi-Fi configuration.

The miner enters setup mode so that the required network and miner information can be configured.

After configuration is saved, the ESP32 stores the settings persistently and attempts to connect to the MINI mining network.

## Miner Identity

The miner creates and stores a persistent P-256 cryptographic identity.

That identity is used during MINI pool authentication.

The private identity remains on the miner.

The public identity is used by the mining infrastructure to recognize and authenticate the miner.

Reflashing or erasing persistent storage can create a new miner identity depending on the flashing procedure used.

## Status LED

The public reference miner retains onboard LED status indication.

The LED is used to provide simple feedback for states such as:

- Setup
- Connection activity
- Accepted mining shares

LED electrical behavior differs between some ESP32 boards. The included board profiles account for the currently supported DevKit and NodeMCU-32S configurations.

## Mining

Once configured and connected, the miner:

1. Authenticates using its persistent P-256 identity.
2. Requests a mining job.
3. Receives a challenge and adaptive target.
4. Searches for a qualifying MiniPOW1 nonce.
5. Submits the nonce to the pool.
6. Receives the pool's acceptance/rejection response.
7. Requests additional work.

The pool independently verifies submitted work.

## Adaptive Difficulty

MINI uses adaptive difficulty.

Different hardware can therefore receive different work targets according to performance.

A faster implementation does not need to pretend to be an ESP32.

MINI is intentionally open to different hardware architectures.

## Pool Discovery

Current MINI pool information is maintained publicly in the network repository:

```text
pools.json
```

The reference miner can use the published pool-discovery information rather than requiring a permanently hard-coded mining server.

## Closed Testnet Warning

MINI is currently under active development.

This is reference software for the **Closed Testnet**, not a final Mainnet firmware release.

Protocol behavior, configuration, pool infrastructure and other details may change as MINI approaches LIVENET.

## Public Reference vs. Plug-and-Mine Hardware

This source exists so anyone can build a compatible MINI miner without purchasing hardware or proprietary software from Mini Miners.

Mini Miners also plans inexpensive preconfigured devices for people who want a simpler plug-and-mine experience.

Those devices may include proprietary convenience features such as automatic firmware updates and managed configuration.

Purchasing one is not required to participate in MINI.

## Technical Documentation

MiniPOW1 specification:

`docs/MINIPOW1.md`

Mining protocol:

`docs/MINING-PROTOCOL.md`

LIVENET:

`docs/LIVENET.md`

Roadmap:

`docs/ROADMAP.md`

Technical MINI site:

https://miniminers.net

Beginner MINI site:

https://miniminers.org

---

**MINI — Made for beginners. Open to everyone. Built for low power.**
