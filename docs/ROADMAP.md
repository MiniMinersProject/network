# MINI Development Roadmap

**Current Stage: Closed Testnet**

MINI is being developed in three major public stages:

**Closed Testnet → MINI LIVENET → MINI Mainnet**

This roadmap distinguishes between technology that exists today, work required before LIVENET, and longer-term Mainnet engineering.

Dates are intentionally not fixed yet. MINI will move between stages when the required systems are ready rather than launching an unfinished stage to meet an arbitrary date.

---

## Stage 1 — Closed Testnet

### Purpose

Prove that the fundamental MINI mining system works on real hardware.

### Working Today

Current Closed Testnet development includes:

- MiniPOW1 mining
- Real ESP32 miners
- Experimental CPU mining
- Pool job distribution
- Pool-side share verification
- Accepted-share accounting
- Adaptive difficulty
- Per-device identity
- Persistent miner configuration
- Wi-Fi setup and reconnection
- OLED miner status
- Development OTA updating
- Live miner monitoring/dashboard
- Long-duration miner stability testing
- Multiple ESP32 hardware variants under test

The Closed Testnet is actively being used to expose firmware, networking, mining and infrastructure problems before public LIVENET participation.

---

# Stage 2 — MINI LIVENET

## Purpose

Open MINI to real public participation while the system remains centrally operated and fixable.

LIVENET is intended to look, feel and act like a cryptocurrency network to participants without falsely representing its centralized backend as a decentralized blockchain.

It is both:

- a cryptocurrency learning environment;
- and a public proving ground for MINI Mainnet.

## Major LIVENET Launch Work

Several development tracks can proceed in parallel.

### Mining and Pool Hardening

Planned work includes:

- Secure public mining transport
- Authentication/session hardening
- Job/challenge expiry
- Replay protection
- Invalid-share throttling
- Connection and abuse rate limiting
- Load testing
- Malformed-client testing
- Pool recovery testing
- Anomaly detection
- Immediate high-priority administrator alerts

MiniPOW and adaptive difficulty already operate on the Closed Testnet and are not being redesigned merely for LIVENET unless testing exposes a reason to change them.

### OTA and Device Security

Commercial Mini Miners devices require hardened update infrastructure before public deployment.

Planned work includes:

- Signed firmware releases
- Firmware integrity verification
- Interrupted-update testing
- Power-loss testing
- Network-loss testing
- Corrupt-image rejection
- Failed-candidate recovery
- Rollback/recovery behavior
- Preservation of miner identity and customer configuration
- Audit to ensure personal/test credentials can never enter production firmware

The proprietary Mini Miners OTA/device-management system is separate from the public reference miner.

### Accounts and Miner Registration

Planned LIVENET account functionality includes:

- Account creation
- Anti-bot controls
- Verified email
- Wallet/address assignment for the LIVENET environment
- Miner registration
- Probation tracking
- Sustained-activity qualification
- Progressive miner allowance
- LIVENET miner ceiling

Current planned miner progression:

**2 → 7 → 12 → 17 → 22 → continued earned expansion**

Current planned LIVENET maximum:

**50 registered miners per account**

The precise qualifying-activity calculation remains to be finalized and published.

### LIVENET Economy

Before public LIVENET balances become economically meaningful, MINI must define and publish the applicable LIVENET issuance rules.

Work includes:

- Network issuance model
- Mining reward accounting
- Supply accounting
- Rules preventing arbitrary hidden issuance
- Transfer accounting
- Economic monitoring
- Exploit response principles

The long-term issuance policy is still under active design.

### Wallets and Transactions

LIVENET is intended to teach real cryptocurrency concepts through actual use.

Planned functionality includes:

- Wallet addresses
- Balances
- Mining rewards
- Sending MINI
- Receiving MINI
- Transaction IDs
- Transaction history
- Person-to-person transfers
- Economic activity
- Buying/selling functionality as development permits
- Merchant use where independently supported

LIVENET wallets are training wallets for the centrally operated LIVENET environment.

Final Mainnet self-custody and recovery rules will be different.

### LIVENET Explorer

The explorer is considered part of the LIVENET experience, not an optional later feature.

Planned explorer functionality includes:

- Wallet/address lookup
- Balances
- Mining rewards
- Transactions
- Transaction IDs
- Issuance
- Total supply
- Mining statistics
- Pool statistics
- Network activity
- Persistent ledger history

MINI intends for LIVENET economic activity to leave a running public record rather than appearing for the first time as a private database snapshot at Mainnet transition.

Tamper-evident and append-only accounting mechanisms are being evaluated.

### Public Developer Resources

MINI intends to provide enough public information and software for participation without purchasing Mini Miners hardware or proprietary software.

Planned public resources include:

- MiniPOW1 specification
- MiniPOW reference implementation
- MiniPOW verifier
- Known-answer test vectors
- ESP32 reference miner
- CPU reference miner
- Mining protocol specification
- Adaptive-difficulty documentation
- Developer examples

Public reference miners will contain the functionality necessary to participate in MINI mining.

Mini Miners commercial firmware/software may contain separate proprietary features such as automatic update infrastructure, device-management features and simplified plug-and-mine functionality.

---

# LIVENET Operation

Once LIVENET opens, development does not stop.

LIVENET itself becomes part of the engineering process.

The project expects to study:

- Real-world miner behavior
- Sybil attempts
- Account abuse
- Parallel miner identities
- Bot activity
- Pool attacks
- Protocol abuse
- Replay attempts
- Denial-of-service attempts
- Hardware optimization
- CPU/GPU implementations
- Network failures
- Scaling bottlenecks
- Wallet mistakes
- Transaction edge cases
- Economic behavior
- Issuance behavior
- Pool economics
- User experience
- Security failures
- Unexpected behavior

Problems discovered during LIVENET may cause LIVENET rules or implementation details to change.

That ability to fix serious problems is one of the reasons LIVENET remains centrally operated.

---

# Stage 3 — Alpha Block Transition

When the independent MINI blockchain is ready and LIVENET has completed its proving role, LIVENET mining and transactions are intended to freeze.

The LIVENET Explorer will remain available in read-only form.

Eligible LIVENET balances are intended to transition **1:1** into MINI Mainnet through the Alpha Block.

Detailed transition rules are not yet final.

Before the transition, MINI intends to publish rules covering:

- Eligibility
- Audit procedure
- Exploit handling
- Final LIVENET state
- Balance verification
- Public transition data
- Alpha Block construction
- Independent verification of migrated balances

The objective is for participants to be able to inspect their final LIVENET history and compare it with their opening Mainnet balance.

---

# Stage 4 — MINI Mainnet

## Purpose

Launch the independent MINI blockchain using the lessons and proven systems developed through Closed Testnet and LIVENET.

### Mainnet Design Targets

Current long-term goals include:

- Independent MINI blockchain
- Low-power network operation
- Raspberry Pi-class full-node target
- Multiple independent nodes
- Multiple independent mining pools
- Third-party pool operation
- Pool compensation
- Public protocol specifications
- Self-custody wallets
- No founder premine
- No privileged founder mining class
- Continued beginner education
- Open expert/developer participation

### Mainnet Engineering Still To Be Determined

Important Mainnet decisions remain intentionally unresolved, including:

- Final consensus architecture
- Final block timing
- Final monetary/emission policy
- Final total-supply policy
- Pool-to-chain work accounting
- Pool compensation percentage
- Node synchronization architecture
- Peer discovery
- Final storage requirements
- Final node resource limits
- Governance mechanisms
- Staking/masternode design, if any
- Transaction-fee architecture
- Long-term attack resistance

These are not being represented as solved before they have actually been engineered and tested.

---

# Ongoing Low-Power Research

Low power remains a permanent MINI engineering goal.

Testing will continue across:

- ESP32 variants
- Raspberry Pi
- CPUs
- GPUs
- Custom miners
- Battery operation
- Solar operation
- MiniPOW/s per watt
- Daily energy consumption
- Annual energy consumption
- Thermal behavior
- Long-duration stability

Results will be published as measurements become available.

See:

https://miniminers.net/power.html

---

# Current Development Principle

MINI does not need to pretend unfinished work is finished.

The project will distinguish between:

**WORKING NOW**

**IN ACTIVE DEVELOPMENT**

**PLANNED**

**TBD**

The purpose of the roadmap is not to promise dates.

It is to show where MINI is today, what is being built next, and which important decisions still need evidence
