# Mini Miners Experimental Android Miner — Beta v0.2

Experimental Android reference miner for the Mini Miners network.

**Status:** Beta / experimental. This is not the polished consumer "Newb" miner.

## What it does

- Mines real MiniPOW on an Android device.
- Uses the same `MINIMINERS/0.3` mining protocol as the ESP32 reference miner.
- Discovers the current enabled pool from the network repository's `pools.json` and selects the highest-priority enabled pool.
- Caches the last successfully discovered pool and retains the legacy factory endpoint only as a final fallback.
- Uses a persistent P-256 device identity.
- Runs one continuous mining worker at Android background priority; it does not create a multi-core worker farm.
- Allows the screen to turn off normally while a partial wake lock keeps mining active.
- On Android 10+ pauses mining at Android `THERMAL_STATUS_SEVERE` or higher and resumes after the device cools.

## Beta v0.2 validation

Beta v0.2 was tested on a Samsung Galaxy A21. It successfully:

- discovered the current MINI pool,
- authenticated,
- completed the MiniPOW self-test,
- submitted accepted shares,
- received current reward accounting, and
- showed working unconfirmed rewards.

The MiniPOW implementation used in the earlier Android alpha was retained for v0.2 after successful share validation.

## Build and install

1. Install Android Studio.
2. Open this `android` folder as a project.
3. Allow Gradle to sync.
4. Enable Developer Options and USB debugging on the Android device.
5. Connect the device by USB and authorize debugging when prompted.
6. In Android Studio choose **Build > Make Project**.
7. Select the connected Android device and click **Run**.
8. Enter a registered MINI wallet and a miner name, then tap **START MINING**.

## Pool discovery

Pool discovery is read from:

`https://raw.githubusercontent.com/MiniMinersProject/network/main/pools.json`

The miner requires the discovery document protocol to match `MINIMINERS/0.3`. If discovery fails it first tries the last cached discovered pool, then the embedded factory fallback.

## Power and thermal behavior

This experimental miner intentionally uses a single continuous mining thread rather than attempting to maximize the phone's total CPU throughput. On Android 10+ it also checks Android's thermal status and pauses at `SEVERE` or higher.

Power use and thermals vary by phone. Early testing should be supervised until behavior on a device is understood.

## Project status

This code is a basic/open reference miner for experimentation and network participation. It is separate from Mini Miners' planned polished consumer/Newb software.
