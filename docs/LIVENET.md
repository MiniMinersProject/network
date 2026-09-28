# MINI LIVENET

**Status:** Planned / Active Development

MINI LIVENET is the public training, economic and proving environment that will operate between the current Closed Testnet and the independent MINI Mainnet blockchain.

## What LIVENET Is

LIVENET is designed to **look, feel and act like a cryptocurrency network in virtually every way visible to its users while remaining centrally operated behind the scenes.**

That centralization is deliberate.

LIVENET exists so MINI can expose its mining system, wallets, transactions, economy, pools, security systems and user experience to real public use while serious problems can still be corrected.

LIVENET is **not** the MINI blockchain, and MINI will not represent it as one.

The independent blockchain comes later.

## Development Path

### Closed Testnet

**Question:** Does the technology work?

Current Closed Testnet development tests:

- MiniPOW
- ESP32 mining
- CPU mining
- Pool communication
- Share verification
- Adaptive difficulty
- Reward accounting
- Miner identity
- Wi-Fi/reconnection behavior
- Firmware stability
- OTA/update systems

### MINI LIVENET

**Question:** What happens when the public actually uses, scales, experiments with and attacks it?

LIVENET introduces real public participation while retaining the ability to repair serious architectural mistakes.

### MINI Mainnet

**Question:** What survived testing strongly enough to make permanent?

Mainnet is the planned independent MINI blockchain.

## Blockchain Training Environment

LIVENET is intended to expose beginners to the same concepts they will encounter in cryptocurrency networks.

Planned LIVENET functionality includes:

- Wallet addresses
- Mining rewards
- Balances
- Sending MINI
- Receiving MINI
- Transaction IDs
- Transaction history
- Public explorer
- Mining pools
- Shares
- Adaptive difficulty
- Public issuance information
- Network statistics
- Person-to-person transfers
- Economic activity
- Buying and selling as those systems become available
- Merchant use where independently supported

The objective is not to create a cosmetic blockchain simulation.

Participants should learn cryptocurrency concepts by actually using them.

## LIVENET Is Expected To Break

A major purpose of LIVENET is to expose problems that cannot be discovered reliably in a small private test.

Expected areas of attack and failure include:

- Sybil and parallel-identity attacks
- Attempts to bypass miner-registration limits
- Malformed mining clients
- Protocol abuse
- Share replay
- Denial-of-service attempts
- Cloned or contested identities
- Automated account creation
- Unusual CPU/GPU/miner implementations
- Network instability
- Scaling bottlenecks
- Wallet mistakes
- Transaction edge cases
- Pool failures
- Infrastructure failures
- OTA/update failures
- Security vulnerabilities
- Unexpected economic behavior
- Unexpected human behavior

Finding these problems during LIVENET is not considered a failure of the testing process.

**Finding them before Mainnet is one of the reasons LIVENET exists.**

## Mining Participation

MINI is not restricted to ESP32 hardware.

Compatible hardware may include:

- ESP32
- CPUs
- GPUs
- Raspberry Pis
- Other microcontrollers
- Custom mining hardware
- Independently developed miners

MINI does not attempt to prove that a miner is physically an ESP32.

The important distinction is between a legitimate miner and attempts to manufacture large numbers of independently rewarded miner identities.

## Adaptive Difficulty and Miner Identity

MINI treats two scaling problems separately.

### Vertical Scaling

One miner becomes dramatically faster.

Adaptive difficulty assigns work appropriate to miner performance so raw speed does not automatically translate into a proportional increase in per-miner rewards.

### Horizontal Scaling

One participant attempts to present one machine, cloud system or botnet as large numbers of independently rewarded miners.

LIVENET addresses this separately through registration, probation, activity requirements, account controls, monitoring and miner ceilings.

## LIVENET Miner Growth

Current planning begins a verified LIVENET account with:

**2 registered miners**

With sustained qualifying activity, capacity may progressively increase:

**2 → 7 → 12 → 17 → 22 → continued earned expansion**

A period that fails to meet the qualifying requirement does not remove miners already approved. It delays further expansion.

The current planned LIVENET maximum is:

**50 registered miners per account**

This is a LIVENET security and issuance safeguard.

It is not necessarily a permanent Mainnet rule.

The exact qualifying-activity calculation remains under development and will be published before it governs public LIVENET participation.

## No Premine / No Founder Allocation

MINI does not plan to allocate a special premine to its founder.

There is no planned privileged founder mining class.

The founder participates under the same applicable LIVENET mining, registration, probation, adaptive-difficulty and reward rules as other participants.

Founder-earned MINI must come from mining or normal economic activity rather than a special founder allocation.

## Public History and Explorer

LIVENET is intended to maintain a persistent and publicly inspectable economic history.

The LIVENET Explorer is planned to expose information including:

- Wallet addresses
- Balances
- Mining rewards
- Transactions
- Transaction IDs
- Issuance
- Supply
- Mining statistics
- Pool statistics
- Ledger history

The explorer is both a learning tool and an accountability mechanism.

LIVENET issuance should leave a running public history rather than appearing for the first time as a private database snapshot at Mainnet launch.

Tamper-evident and append-only accounting mechanisms are being evaluated as part of LIVENET engineering.

## Third-Party Pools

The official Mini Miners pool provides the initial development infrastructure.

The long-term MINI architecture is intended to support independently operated third-party pools.

Pool operators are intended to receive a defined percentage for legitimate infrastructure and mining-pool services.

The exact Mainnet compensation and pool-to-network verification mechanism is not yet finalized.

## LIVENET Security Monitoring

LIVENET infrastructure is planned to monitor for unusual activity such as:

- Abnormal share patterns
- Repeated invalid work
- Suspicious identity/session behavior
- Duplicate identities
- Rapid or unusual connection behavior
- Account-growth anomalies
- Coordinated miner behavior
- Protocol abuse

High-priority anomalies are intended to generate immediate administrator alerts and preserve supporting evidence for investigation.

An anomaly is not automatically proof of cheating.

Permanent punitive actions should not be based solely on an immature heuristic detector.

## LIVENET to Mainnet

When MINI is ready to transition to its independent blockchain, LIVENET mining and transactions are intended to freeze.

The LIVENET Explorer will remain available in read-only form so participants can inspect the final state and history.

Eligible LIVENET balances are intended to transition **1:1** into MINI Mainnet through the Alpha Block.

The detailed:

- Eligibility rules
- Audit rules
- Exploit-handling rules
- Verification procedure
- Final-state publication procedure
- Alpha Block transition procedure

have **not yet been finalized**.

They will be developed during LIVENET, tested and published well before the Alpha Block transition.

## Why Not Launch Mainnet Immediately?

Because decentralization makes mistakes harder to repair.

MINI would rather expose unfinished ideas to real-world testing while they remain fixable than prematurely make them permanent consensus rules.

The development philosophy is therefore:

**Closed Testnet — prove it works.**

**LIVENET — let the public use it and try to break it.**

**Mainnet — make the proven design permanent.**

## More Information

Technical MINI site:

https://miniminers.net

Full LIVENET explanation:

https://miniminers.net/livenet.html

Real-world power and performance testing:

https://miniminers.net/power.html

Beginner MINI site:

https://miniminers.org

---

**Build it. Test it. Break it. Fix it. Then make it permanent.**
