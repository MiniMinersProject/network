# MiniPOW1 Specification

**Status:** Closed Testnet / Development Specification  
**Current implementation:** MiniPOW1

MiniPOW1 is the proof-of-work function currently used by the MINI Closed Testnet.

It is designed to operate on low-resource hardware such as ESP32-class microcontrollers while remaining openly implementable on CPUs, GPUs, Raspberry Pis and other compatible hardware.

MiniPOW1 does **not** attempt to prove that mining is occurring on a particular type of physical device.

---

# 1. Current Parameters

| Parameter | Value |
|---|---:|
| Cryptographic hash | SHA-256 |
| Scratchpad size | 65,536 bytes (64 KiB) |
| Scratchpad block size | 32 bytes |
| Scratchpad blocks | 2,048 |
| Mixing rounds | 512 |
| Nonce size | 64-bit unsigned integer |
| Nonce byte order | Little-endian |
| Block-index byte order | Little-endian |
| Round-index byte order | Little-endian |
| Digest size | 256 bits |

These parameters describe the current Closed Testnet implementation.

They may change before MINI Mainnet if testing reveals a reason to change them.

---

# 2. Inputs

A MiniPOW1 attempt takes two primary inputs:

- `challenge`
- `nonce`

The challenge is represented by the mining protocol as hexadecimal and decoded into raw bytes before MiniPOW1 execution.

The nonce is an unsigned integer encoded as exactly 8 bytes in little-endian order.

Conceptually:

```text
challenge_bytes = HEX_DECODE(challenge)

nonce_bytes = UINT64_LE(nonce)
```

---

# 3. Seed Generation

The initial 32-byte seed is:

```text
seed = SHA256(challenge_bytes || nonce_bytes)
```

where `||` means byte concatenation.

The seed remains unchanged throughout that MiniPOW attempt.

---

# 4. Scratchpad Initialization

MiniPOW1 allocates a:

```text
65,536-byte scratchpad
```

The scratchpad consists of:

```text
2,048 blocks × 32 bytes
```

Initialization begins with:

```text
prev = seed
```

For each block index `i` from:

```text
0 through 2047
```

encode `i` as an unsigned 32-bit little-endian value:

```text
index_bytes = UINT32_LE(i)
```

Then calculate:

```text
prev = SHA256(prev || seed || index_bytes)
```

The resulting 32-byte `prev` value becomes scratchpad block `i`.

Therefore:

```text
scratchpad[i] = prev
```

After this loop completes, all 2,048 scratchpad blocks have been initialized.

---

# 5. Initial Mixing State

After scratchpad initialization:

```text
state = seed
```

MiniPOW1 then performs exactly:

```text
512 mixing rounds
```

---

# 6. State-Dependent Block Selection

For each mixing round, MiniPOW1 selects a scratchpad block based on the current state.

Take bytes 0 through 3 of `state`, interpret them as an unsigned 32-bit little-endian integer, and reduce the result modulo 2,048:

```text
block_index =
    UINT32_LE(state[0:4]) mod 2048
```

The corresponding 32-byte scratchpad block is:

```text
memory_block = scratchpad[block_index]
```

Because block selection depends on the current state, later memory access depends on previous computation.

---

# 7. Mixing Round

For each round index `r` from:

```text
0 through 511
```

encode the round number as an unsigned 32-bit little-endian value:

```text
round_bytes = UINT32_LE(r)
```

Calculate the new state:

```text
state =
    SHA256(
        state
        || memory_block
        || round_bytes
    )
```

The result is another 32-byte state.

---

# 8. Scratchpad Mutation

After calculating the new state, the selected scratchpad block is modified.

Each of its 32 bytes is XORed with the corresponding byte of the new state:

```text
for j = 0 through 31:

    scratchpad[block_index][j] =
        scratchpad[block_index][j]
        XOR
        state[j]
```

The modified block remains in the scratchpad and may affect later rounds if selected again.

This means MiniPOW1's memory state evolves during the 512-round mixing process.

---

# 9. Final Tail Selection

After all 512 mixing rounds are complete, MiniPOW1 selects one final scratchpad block.

This time bytes 4 through 7 of the final state are interpreted as an unsigned 32-bit little-endian integer:

```text
tail_index =
    UINT32_LE(state[4:8]) mod 2048
```

The corresponding 32-byte block is:

```text
tail_block = scratchpad[tail_index]
```

---

# 10. Final MiniPOW1 Digest

The final MiniPOW1 result is:

```text
SHA256(
    state
    || tail_block
    || seed
)
```

The resulting 256-bit value is the MiniPOW1 digest.

The current reference miner represents that result as a 64-character lowercase hexadecimal string.

---

# 11. Complete Algorithm

In compact pseudocode:

```text
function MiniPOW1(challenge, nonce):

    challenge_bytes = HEX_DECODE(challenge)

    nonce_bytes = UINT64_LE(nonce)

    seed =
        SHA256(
            challenge_bytes
            || nonce_bytes
        )

    scratchpad = 65536 bytes

    prev = seed

    for i = 0 through 2047:

        index_bytes = UINT32_LE(i)

        prev =
            SHA256(
                prev
                || seed
                || index_bytes
            )

        scratchpad[i] = prev

    state = seed

    for r = 0 through 511:

        block_index =
            UINT32_LE(state[0:4])
            mod 2048

        memory_block =
            scratchpad[block_index]

        round_bytes =
            UINT32_LE(r)

        state =
            SHA256(
                state
                || memory_block
                || round_bytes
            )

        scratchpad[block_index] =
            scratchpad[block_index]
            XOR
            state

    tail_index =
        UINT32_LE(state[4:8])
        mod 2048

    tail_block =
        scratchpad[tail_index]

    result =
        SHA256(
            state
            || tail_block
            || seed
        )

    return result
```

---

# 12. Known-Answer Test Vector

The current reference implementation contains a built-in MiniPOW1 self-test.

Input challenge:

```text
00112233445566778899aabbccddeeff
```

Nonce:

```text
42
```

Expected MiniPOW1 digest:

```text
f67043c783d01c3104f322c76de5d4f0fbbac377c724b48546c238c06f505657
```

An independent implementation using the specification above should produce exactly that digest.

If it does not, the implementation is not compatible with the current MiniPOW1 reference implementation.

---

# 13. Mining Target Comparison

The current reference CPU miner receives a 256-bit target represented as hexadecimal.

The target and MiniPOW1 digest are interpreted as unsigned integers.

A MiniPOW result qualifies when:

```text
result_value <= target_value
```

The miner searches nonces beginning at:

```text
nonce = 0
```

and increments the nonce until a qualifying result is found.

This starting strategy is a property of the current reference miner and is not itself a requirement of the MiniPOW1 hash function.

---

# 14. MiniPOW1 vs. Difficulty

MiniPOW1 defines the proof-of-work computation.

Difficulty determines how difficult it is to find a MiniPOW1 digest satisfying the assigned target.

These are separate concepts.

MINI currently uses adaptive difficulty so miners with dramatically different MiniPOW performance can receive appropriately difficult work.

For example, Closed Testnet development has included both:

- ESP32-class miners at approximately 6.9 MiniPOW/s
- An experimental AMD Ryzen 9 6900HX CPU miner at approximately 258–288 MiniPOW/s

The CPU can calculate MiniPOW1 much faster.

Adaptive difficulty is intended to compensate for that difference at the mining-job level.

The exact adaptive-difficulty controller will be documented separately.

---

# 15. MiniPOW1 Does Not Establish Miner Identity

MiniPOW1 proves computational work against a challenge and target.

It does not prove:

- what hardware performed that work;
- who owns the hardware;
- that two mining processes are physically separate devices;
- that one participant is not operating multiple miners.

MINI intentionally treats miner identity and proof-of-work as separate systems.

During LIVENET, horizontal scaling and Sybil-style abuse are addressed through account, registration, probation, activity, monitoring and miner-limit systems rather than by attempting to make MiniPOW1 identify physical hardware.

---

# 16. Reference Implementation

The public Python CPU reference miner contains the current working MiniPOW1 implementation and known-answer self-test.

Repository location:

```text
miners/cpu/miner.py
```

Developers are encouraged to compare independent implementations against both the specification and the published known-answer vector.

Additional test vectors will be added.

---

# 17. Development Status

MiniPOW1 is actively running on the MINI Closed Testnet.

The algorithm described here is therefore not merely a proposed proof-of-work design; it corresponds to the current development implementation.

However, MINI remains under active development.

Testing during Closed Testnet and LIVENET may expose reasons to revise MiniPOW before Mainnet.

Any incompatible algorithm change must be explicitly versioned rather than silently changing the meaning of MiniPOW1.

---

# 18. Work Still To Be Published

Additional technical documentation is still planned, including:

- Additional MiniPOW1 test vectors
- Formal mining-job serialization
- Full mining protocol specification
- Adaptive-difficulty algorithm
- Share/job expiry behavior
- Replay protection
- Reference implementations on additional hardware
- Performance benchmarks
- Energy-efficiency benchmarks

---

# Related Resources

Public CPU reference miner:

`miners/cpu/miner.py`

MINI technical site:

https://miniminers.net

LIVENET:

https://miniminers.net/livenet.html

Power and real-world testing:

https://miniminers.net/power.html

Beginner site:

https://miniminers.org

---

**MINI — Made for beginners. Open to everyone. Built for low power.**
