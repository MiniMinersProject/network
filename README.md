# MINI Network

**Made for beginners. Open to everyone. Built for low power.**

MINI is a low-power cryptocurrency project designed for people who have always wanted to mine cryptocurrency but did not know how or did not want expensive, power-hungry mining hardware.

MINI is being developed around inexpensive and efficient devices such as ESP32 microcontrollers and Raspberry Pis while remaining open to CPUs, GPUs, custom miners, and other compatible hardware.

This repository is the public technical home for MINI network information, protocol documentation, reference mining software, and developer resources.

## Project Status

MINI is currently operating a live **Closed Testnet** with real ESP32 and CPU miners performing MiniPOW work against the development mining pool.

Development is progressing through three major stages:

**Closed Testnet** — Prove the mining technology, firmware, MiniPOW, pool infrastructure, adaptive difficulty, rewards, and hardware operation.

**MINI LIVENET** — Open MINI to public participation in a centrally operated blockchain training and proving environment. LIVENET is designed to look, feel, and act like a cryptocurrency network while remaining fixable as real users expose bugs, attacks, scaling problems, economic problems, and unexpected behavior.

**MINI Mainnet** — Use what survives LIVENET testing to launch the independent MINI blockchain.

LIVENET is intentionally **not represented as a decentralized blockchain**. Its purpose is to discover and correct serious problems before those rules become permanent Mainnet consensus.

## Low Power by Design

Low-power mining is not a secondary feature of MINI.

**It is one of the primary reasons MINI exists.**

Current Closed Testnet development includes ESP32-class miners operating at approximately **6.9 MiniPOW/s**, along with experimental CPU mining.

MINI uses adaptive difficulty so hardware with dramatically different performance can participate without raw computing speed automatically translating into proportionally greater per-miner rewards.

The long-term goal extends beyond mining. Raspberry Pi-class full-node operation is an explicit Mainnet design target.

## Open Mining

Purchasing Mini Miners hardware or proprietary software is **not required to participate in MINI**.

Public reference mining software and protocol documentation will allow developers, hobbyists, and miners to build and modify compatible miners.

Planned public reference implementations include:

- ESP32 miner
- CPU miner
- MiniPOW reference implementation
- MiniPOW verification tools
- Mining protocol documentation
- Test vectors and developer documentation

MINI welcomes experimentation.

Build a miner. Modify one. Port MiniPOW to different hardware. Measure it. Improve it. Find problems.

## Public Reference Software vs. Mini Miners Products

The MINI mining protocol and public reference miners are intended to provide everything necessary for independent participation in the network.

Mini Miners also develops separate commercial hardware and proprietary software containing additional convenience and management features, including automatic update infrastructure and plug-and-mine functionality.

Those commercial features are not required for network participation.

The objective is simple:

**Anyone should be able to mine MINI without buying something from us.**

## MiniPOW

MINI mining uses **MiniPOW1**, a proof-of-work design developed for low-resource mining hardware.

The current MiniPOW1 design uses SHA-256 with a **64 KiB per-attempt scratchpad** and **512 state-dependent mixing rounds**.

Faster hardware receives more difficult work through adaptive difficulty.

Technical MiniPOW documentation and reference implementations will be added to this repository as development continues.

## Fair Participation

MINI is open to ESP32s, CPUs, GPUs, Raspberry Pis, and other compatible hardware.

Adaptive difficulty addresses **vertical scaling**: making one miner dramatically faster should not produce a proportional increase in its per-miner reward.

LIVENET registration and probation address **horizontal scaling**: one powerful computer, cloud system, or botnet should not be able to instantly present itself as hundreds or thousands of independently rewarded miners.

Current LIVENET planning begins verified accounts with 2 miners and progressively permits additional miners through sustained participation.

A maximum of **50 registered miners per account** is currently planned for LIVENET as a security and issuance safeguard.

That is a LIVENET rule and does not necessarily represent a permanent Mainnet limitation.

## No Premine. No Founder Allocation.

MINI is not intended to begin with a special founder allocation or founder premine.

The founder participates under the same applicable mining, registration, probation, adaptive-difficulty, and reward rules as other participants.

There is no planned privileged founder mining class.

## Open Pool Future

The official Mini Miners pool provides the initial development infrastructure.

MINI is being designed toward a future in which independent third parties can operate compatible mining pools.

Pool operators are intended to receive a defined percentage for providing legitimate mining infrastructure. The final Mainnet compensation and pool-verification architecture remains under development.

## Repository Roadmap

This repository will grow alongside MINI development.

Planned additions include:

- MiniPOW1 technical specification
- Mining protocol specification
- ESP32 reference miner
- Experimental CPU reference miner
- MiniPOW test vectors
- Adaptive difficulty documentation
- LIVENET architecture
- Security documentation
- Network architecture
- Pool documentation
- Developer examples

Expect frequent changes while MINI moves toward LIVENET.

## Current Network Configuration

Current pool and network discovery information is maintained in:

`pools.json`

The contents of that file may change as Closed Testnet and LIVENET infrastructure evolves.

## Learn More

**New to MINI or cryptocurrency?**

Visit **MiniMiners.org**

https://miniminers.org

**Developer, miner builder, researcher, or technical user?**

Visit **MiniMiners.net**

https://miniminers.net

Technical documentation:

https://miniminers.net/livenet.html

https://miniminers.net/power.html

## Development Philosophy

MINI is being built in public.

Implemented features will be identified as implemented.

Planned features will be identified as planned.

Unresolved design decisions will be identified as unresolved.

LIVENET exists specifically so that real public use can expose problems before the independent blockchain makes those decisions difficult to change.

**Build it. Test it. Break it. Fix it. Then make it permanent.**
