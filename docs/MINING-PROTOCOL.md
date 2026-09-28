# MINI Mining Protocol

**Protocol Version:** `MINIMINERS/0.3`  
**Status:** Closed Testnet / Development Protocol

This document describes the mining protocol currently implemented by the public MINI Python CPU reference miner.

The protocol is under active development and is expected to be hardened before public LIVENET operation.

---

# 1. Overview

The current Closed Testnet protocol provides:

- Pool connection
- Protocol-version negotiation
- Miner identification
- Persistent P-256 cryptographic miner identity
- Challenge-response authentication
- Mining-job requests
- Adaptive target assignment
- MiniPOW1 share submission
- Pool-side verification
- Reward responses

The current reference miner communicates using newline-terminated UTF-8 text messages with fields separated by the pipe character:

```text
|
```

Example:

```text
COMMAND|FIELD1|FIELD2
```

---

# 2. Connection

The miner opens a TCP connection to a MINI mining pool.

Current official pool information is maintained separately in:

```text
pools.json
```

Developers should not assume that a development server address will remain permanent.

---

# 3. Server Greeting

Immediately after connection, the miner expects the server to send:

```text
HELLO|MINIMINERS/0.3
```

The current reference miner requires an exact match.

If the protocol version does not match, the miner terminates that connection.

This prevents the miner from silently continuing against a protocol version it does not understand.

---

# 4. Miner Authentication Request

After receiving the correct greeting, the miner sends:

```text
AUTH|USERNAME|MINER_NAME|MINER_TYPE|PUBLIC_KEY
```

Fields:

### `USERNAME`

Account or participant identifier used by the current Closed Testnet.

### `MINER_NAME`

Human-readable name assigned to the miner.

### `MINER_TYPE`

Identifies the miner implementation/hardware category.

The current CPU reference miner uses:

```text
PC
```

### `PUBLIC_KEY`

The miner's P-256 public key serialized as PEM and Base64 encoded for transport.

The corresponding private key remains local to the miner.

---

# 5. Persistent Miner Identity

The current reference miner generates an elliptic-curve key pair using:

```text
P-256 / secp256r1
```

The private key is stored locally.

The public key is sent to the mining server during authentication.

The miner also calculates its identity fingerprint as:

```text
SHA256(public_key_pem)
```

represented as hexadecimal.

This cryptographic identity is intended to persist across miner restarts.

It is not a proof that the miner is running on a particular physical device.

---

# 6. Authentication Challenge

After receiving the `AUTH` message, the server returns:

```text
CHALLENGE|CHALLENGE_DATA
```

The miner requires exactly two fields.

The challenge is supplied by the server for the current authentication exchange.

---

# 7. Challenge Signature

The miner signs the challenge using its persistent P-256 private key.

The current implementation signs:

```text
ASCII(CHALLENGE_DATA)
```

using:

```text
ECDSA with SHA-256
```

The resulting signature is Base64 encoded.

The miner then sends:

```text
AUTH_RESPONSE|BASE64_SIGNATURE
```

The private key is never transmitted.

---

# 8. Authentication Success

After verifying the signature, the server responds:

```text
AUTH_OK|FINGERPRINT
```

The miner compares the returned fingerprint with the fingerprint it calculated locally from its public key.

If they do not match, the miner rejects the authentication session.

A successful exchange therefore establishes that the miner possesses the private key corresponding to the public identity presented during authentication.

---

# 9. Requesting Work

After successful authentication, the miner requests a mining job by sending:

```text
JOB
```

The current reference miner requests one job at a time.

---

# 10. Mining Job

The server responds:

```text
JOB|JOB_ID|CHALLENGE|TARGET
```

Fields:

### `JOB_ID`

Identifier assigned to the mining job.

The miner returns this identifier when submitting work.

### `CHALLENGE`

Hexadecimal challenge used as the MiniPOW1 challenge input.

### `TARGET`

A 256-bit target represented as exactly 64 hexadecimal characters.

The target reflects the work difficulty assigned to that miner.

---

# 11. Performing MiniPOW1

For each nonce candidate, the miner computes:

```text
MiniPOW1(CHALLENGE, NONCE)
```

The current Python reference miner begins with:

```text
nonce = 0
```

and increments the nonce until it finds a qualifying result.

Starting at zero is reference-miner behavior, not a requirement of the MiniPOW1 function itself.

The complete MiniPOW1 specification is documented in:

```text
docs/MINIPOW1.md
```

---

# 12. Target Test

Both the MiniPOW1 digest and target are interpreted as unsigned 256-bit integers.

A result qualifies when:

```text
MiniPOW1_result <= target
```

When that condition is satisfied, the miner has found a candidate share.

---

# 13. Share Submission

The miner submits the result using:

```text
SUBMIT|JOB_ID|NONCE
```

The current protocol does not require the miner to submit its calculated digest.

The pool independently performs the required verification.

This means a miner cannot receive credit merely by claiming that a nonce produced valid work.

---

# 14. Accepted Share

For a successfully verified share, the current reference miner expects a response beginning with:

```text
ACCEPTED|
```

The next field contains the reward reported by the pool.

Conceptually:

```text
ACCEPTED|REWARD
```

The miner displays the reward and updates its local accepted-share count.

---

# 15. Rejected or Other Responses

Any share-submission response that does not begin with:

```text
ACCEPTED|
```

is currently treated by the Python reference miner as a rejected/non-accepted result and displayed to the user.

More formal error codes and response definitions may be introduced as the protocol is hardened.

---

# 16. Current Mining Loop

The current reference miner operates conceptually as:

```text
CONNECT

RECEIVE:
HELLO|MINIMINERS/0.3

SEND:
AUTH|USERNAME|MINER_NAME|MINER_TYPE|PUBLIC_KEY

RECEIVE:
CHALLENGE|CHALLENGE_DATA

SIGN CHALLENGE

SEND:
AUTH_RESPONSE|SIGNATURE

RECEIVE:
AUTH_OK|FINGERPRINT

VERIFY FINGERPRINT

LOOP:

    SEND:
    JOB

    RECEIVE:
    JOB|JOB_ID|CHALLENGE|TARGET

    FIND VALID MINIPOW1 NONCE

    SEND:
    SUBMIT|JOB_ID|NONCE

    RECEIVE:
    ACCEPTED|REWARD
    or rejection/error response
```

---

# 17. Connection Recovery

The current Python reference miner automatically attempts to reconnect after connection or protocol errors.

Its present reconnect delay is:

```text
5 seconds
```

The miner retains its persistent cryptographic identity across reconnections.

---

# 18. Adaptive Difficulty

Difficulty is assigned by the mining infrastructure rather than selected by the miner.

The target supplied in each `JOB` determines the work required for that job.

This permits the server to assign different targets to miners with dramatically different MiniPOW performance.

The exact adaptive-difficulty controller is separate from the wire protocol and will be documented independently.

---

# 19. Current Security Limitations

`MINIMINERS/0.3` is a **Closed Testnet development protocol**.

Its existence in this repository should not be interpreted as a claim that the current transport and abuse protections are sufficient for hostile public Internet operation.

Before LIVENET, planned protocol/infrastructure hardening includes work in areas such as:

- Secure transport
- Session handling
- Job expiration
- Replay protection
- Invalid-share throttling
- Connection rate limiting
- Authentication abuse controls
- Malformed-message handling
- Resource limits
- Denial-of-service resistance
- Security logging
- Anomaly detection

Where possible, future protocol changes will be explicitly versioned.

---

# 20. Compatibility

Developers building an independent miner for the current Closed Testnet should reproduce:

- `MINIMINERS/0.3` protocol behavior
- P-256 identity generation
- Public-key serialization
- Identity fingerprint calculation
- ECDSA/SHA-256 challenge signing
- MiniPOW1
- Target comparison
- Job and submission message formats

The Python reference miner is the current public implementation for comparison.

Location:

```text
miners/cpu/miner.py
```

---

# 21. Protocol Evolution

MINI is still under active development.

The mining protocol may change as Closed Testnet testing exposes problems and as security controls required for LIVENET are implemented.

Protocol documentation and reference miners should evolve together.

An incompatible protocol change should receive a new protocol version rather than silently changing the meaning of `MINIMINERS/0.3`.

---

# Related Documentation

MiniPOW1 specification:

```text
docs/MINIPOW1.md
```

CPU reference miner:

```text
miners/cpu/miner.py
```

LIVENET architecture:

```text
docs/LIVENET.md
```

Development roadmap:

```text
docs/ROADMAP.md
```

Current pool discovery:

```text
pools.json
```

Technical site:

https://miniminers.net

Beginner site:

https://miniminers.org

---

**MINI — Made for beginners. Open to everyone. Built for low power.**
