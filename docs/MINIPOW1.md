# MiniPOW1

**Status:** Closed Testnet / Development Specification

MiniPOW1 is the proof-of-work function currently used by the MINI Closed Testnet.

It is designed to allow useful participation by low-resource hardware, including ESP32-class microcontrollers, while remaining implementable on CPUs, GPUs and other hardware.

MINI does not require miners to use a particular processor or device.

## Design Goals

MiniPOW1 is designed around several goals:

- Operate on ESP32-class hardware.
- Require more than a trivial SHA-256 nonce loop.
- Remain independently verifiable by the mining pool.
- Allow implementations on different hardware architectures.
- Support frequent mining feedback for beginners.
- Work with MINI adaptive difficulty.
- Keep verification practical for network infrastructure.
- Encourage experimentation and independent miner implementations.

MiniPOW1 is not intended to prove that a miner is physically running on an ESP32.

CPUs, GPUs, Raspberry Pis and other compatible hardware are legitimate MINI miners.

## Current MiniPOW1 Parameters

The current Closed Testnet implementation uses:

| Parameter | Current value |
|---|---:|
| Cryptographic primitive | SHA-256 |
| Scratchpad | 64 KiB per attempt |
| Mixing rounds | 512 |
| Difficulty | Assigned per miner |
| Difficulty control | Adaptive |
| Verification | Pool/server independently verifies submitted work |

These parameters describe the current development implementation and may change before MINI Mainnet.

## Adaptive Difficulty

MINI separates proof-of-work performance from miner identity.

A faster implementation can perform more MiniPOW attempts per second, but the network can assign that miner more difficult work.

This is referred to throughout MINI documentation as **vertical scaling control**.

For example, current development testing has included approximately:

- ESP32-class miner: ~6.9 MiniPOW/s
- AMD Ryzen 9 6900HX development system: roughly ~258–288 MiniPOW/s during experimental CPU mining

The CPU is dramatically faster at MiniPOW execution.

That does not mean it should automatically receive a proportionally greater per-miner reward.

Adaptive difficulty is intended to absorb large differences in miner performance while maintaining useful share timing.

The exact adaptive-difficulty controller used by the public network will be documented separately.

## Horizontal Scaling Is a Different Problem

Adaptive difficulty does not prevent a powerful machine from attempting to present itself as many separate miners.

MINI therefore treats two scaling problems separately:

**Vertical scaling**

One miner becomes much faster.

Handled primarily through adaptive difficulty.

**Horizontal scaling**

One participant attempts to create large numbers of independently rewarded miner identities.

During LIVENET this is addressed through account verification, miner registration, probation, activity requirements, miner ceilings, monitoring and abuse controls.

These systems are intentionally separate from MiniPOW itself.

## Verification

Submitted MiniPOW results are independently checked by the mining infrastructure.

A miner does not receive credit merely because it claims to have performed work.

The verifier must be able to reproduce the required computation and determine whether the submitted result satisfies the assigned work target.

## Current Development Status

MiniPOW1 is already executing on real hardware connected to the MINI Closed Testnet.

Current development includes:

- ESP32 mining
- CPU mining
- Pool-side share verification
- Adaptive difficulty
- Accepted-share accounting
- Long-duration ESP32 testing

The public specification is being expanded as MINI approaches LIVENET.

## Specification Work Still To Come

This document currently describes the design and high-level parameters of MiniPOW1.

Before it is treated as a complete independent implementation specification, additional material will be published, including:

- Exact input/job serialization
- Scratchpad initialization procedure
- Exact 512-round mixing procedure
- Nonce handling
- Target calculation and comparison
- Endianness
- Share serialization
- Known-answer test vectors
- Reference verifier
- Reference miner implementation
- Adaptive-difficulty specification

Until those items are published, this document should **not** be treated as sufficient by itself to create a consensus-compatible independent MiniPOW1 implementation.

## Open Development

The purpose of publishing MiniPOW1 is not merely to describe MINI mining.

We want developers to implement it.

Independent implementations help expose ambiguity, portability problems, optimization opportunities and security assumptions.

As reference code and test vectors are published, developers will be encouraged to implement MiniPOW1 independently and test their results against the MINI network.

## Related Documentation

MINI technical home:

https://miniminers.net

LIVENET architecture:

https://miniminers.net/livenet.html

Power and real-world testing:

https://miniminers.net/power.html

Beginner MINI site:

https://miniminers.org

---

**MINI — Made for beginners. Open to everyone. Built for low power.**
