import base64
import hashlib
import socket
import time
from pathlib import Path

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec


SERVER_HOST = "144.202.27.176"
SERVER_PORT = 2811

USERNAME = "CHANGE_ME"
MINER_NAME = "CHANGE_ME"
MINER_TYPE = "PC"

if USERNAME == "CHANGE_ME" or MINER_NAME == "CHANGE_ME":
    raise SystemExit(
        "Setup required: edit USERNAME and MINER_NAME near the top of miner.py."
    )

PROTOCOL_VERSION = "MINIMINERS/0.3"

IDENTITY_DIR = Path(__file__).resolve().parent / "identity"
PRIVATE_KEY_FILE = IDENTITY_DIR / "miner_private_key.pem"

SCRATCHPAD_SIZE = 64 * 1024
BLOCK_SIZE = 32
BLOCK_COUNT = SCRATCHPAD_SIZE // BLOCK_SIZE
MIX_ROUNDS = 512

TEST_CHALLENGE = (
    "00112233445566778899aabbccddeeff"
)

TEST_NONCE = 42

TEST_RESULT = (
    "f67043c783d01c3104f322c76de5d4f0"
    "fbbac377c724b48546c238c06f505657"
)



def load_or_create_identity():
    """
    Load this miner's persistent P-256 private key.

    The private key is generated once and remains local.
    Only the public key is sent to the Mini Miners server.
    """
    IDENTITY_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    if PRIVATE_KEY_FILE.exists():
        private_key = (
            serialization.load_pem_private_key(
                PRIVATE_KEY_FILE.read_bytes(),
                password=None,
            )
        )

        if not isinstance(
            private_key,
            ec.EllipticCurvePrivateKey,
        ):
            raise ValueError(
                "Stored identity is not an EC private key"
            )

        if not isinstance(
            private_key.curve,
            ec.SECP256R1,
        ):
            raise ValueError(
                "Stored identity does not use P-256"
            )

        created = False

    else:
        private_key = (
            ec.generate_private_key(
                ec.SECP256R1()
            )
        )

        private_key_pem = (
            private_key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.PKCS8,
                encryption_algorithm=serialization.NoEncryption(),
            )
        )

        PRIVATE_KEY_FILE.write_bytes(
            private_key_pem
        )

        created = True

    public_key_pem = (
        private_key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo,
        )
    )

    public_key_b64 = (
        base64.b64encode(
            public_key_pem
        ).decode("ascii")
    )

    fingerprint = hashlib.sha256(
        public_key_pem
    ).hexdigest()

    return (
        private_key,
        public_key_b64,
        fingerprint,
        created,
    )


def sign_challenge(
    private_key,
    challenge,
):
    signature = private_key.sign(
        challenge.encode("ascii"),
        ec.ECDSA(
            hashes.SHA256()
        ),
    )

    return base64.b64encode(
        signature
    ).decode("ascii")

def minipow1(challenge: str, nonce: int) -> str:
    challenge_bytes = bytes.fromhex(
        challenge
    )

    nonce_bytes = nonce.to_bytes(
        8,
        "little",
        signed=False,
    )

    seed = hashlib.sha256(
        challenge_bytes + nonce_bytes
    ).digest()

    scratchpad = bytearray(
        SCRATCHPAD_SIZE
    )

    prev = seed

    for block_index in range(
        BLOCK_COUNT
    ):
        index_bytes = (
            block_index.to_bytes(
                4,
                "little",
                signed=False,
            )
        )

        prev = hashlib.sha256(
            prev
            + seed
            + index_bytes
        ).digest()

        offset = (
            block_index * BLOCK_SIZE
        )

        scratchpad[
            offset:offset + BLOCK_SIZE
        ] = prev

    state = seed

    for round_index in range(
        MIX_ROUNDS
    ):
        block_index = (
            int.from_bytes(
                state[0:4],
                "little",
                signed=False,
            )
            % BLOCK_COUNT
        )

        offset = (
            block_index * BLOCK_SIZE
        )

        memory_block = bytes(
            scratchpad[
                offset:offset
                + BLOCK_SIZE
            ]
        )

        round_bytes = (
            round_index.to_bytes(
                4,
                "little",
                signed=False,
            )
        )

        state = hashlib.sha256(
            state
            + memory_block
            + round_bytes
        ).digest()

        for byte_index in range(
            BLOCK_SIZE
        ):
            scratchpad[
                offset + byte_index
            ] ^= state[byte_index]

    tail_index = (
        int.from_bytes(
            state[4:8],
            "little",
            signed=False,
        )
        % BLOCK_COUNT
    )

    offset = (
        tail_index * BLOCK_SIZE
    )

    tail_block = bytes(
        scratchpad[
            offset:offset + BLOCK_SIZE
        ]
    )

    return hashlib.sha256(
        state
        + tail_block
        + seed
    ).hexdigest()


def run_self_test():
    print(
        "Running MiniPOW1 self-test..."
    )

    result = minipow1(
        TEST_CHALLENGE,
        TEST_NONCE,
    )

    if result != TEST_RESULT:
        print(
            "MINIPOW1 SELF-TEST FAILED"
        )

        print(
            f"Expected: {TEST_RESULT}"
        )

        print(
            f"Received: {result}"
        )

        raise SystemExit(1)

    print(
        "MiniPOW1 self-test PASSED"
    )

    print()


def send_line(sock, message):
    sock.sendall(
        (message + "\n").encode(
            "utf-8"
        )
    )


def read_line(sock):
    data = bytearray()

    while True:
        byte = sock.recv(1)

        if not byte:
            raise ConnectionError(
                "Server closed "
                "the connection"
            )

        if byte == b"\n":
            return (
                data.decode(
                    "utf-8"
                ).strip()
            )

        data.extend(byte)


def mine_job(
    challenge,
    target_hex,
):
    nonce = 0
    attempts = 0

    target_value = int(
        target_hex,
        16,
    )

    start_time = (
        time.perf_counter()
    )

    while True:
        digest = minipow1(
            challenge,
            nonce,
        )

        attempts += 1

        result_value = int(
            digest,
            16,
        )

        if (
            result_value
            <= target_value
        ):
            elapsed = (
                time.perf_counter()
                - start_time
            )

            speed = (
                attempts / elapsed
                if elapsed > 0
                else 0
            )

            return (
                nonce,
                digest,
                attempts,
                speed,
                elapsed,
            )

        nonce += 1


def mining_session(
    private_key,
    public_key_b64,
    local_fingerprint,
):
    accepted = 0
    rejected = 0

    print(
        f"Connecting to "
        f"{SERVER_HOST}:"
        f"{SERVER_PORT}..."
    )

    with socket.create_connection(
        (
            SERVER_HOST,
            SERVER_PORT,
        ),
        timeout=15,
    ) as sock:

        sock.settimeout(None)

        hello = read_line(sock)

        print(
            f"Server: {hello}"
        )

        expected_hello = (
            f"HELLO|"
            f"{PROTOCOL_VERSION}"
        )

        if hello != expected_hello:
            raise ConnectionError(
                "Unexpected server "
                f"greeting: {hello}"
            )

        send_line(
            sock,
            f"AUTH|"
            f"{USERNAME}|"
            f"{MINER_NAME}|"
            f"{MINER_TYPE}|"
            f"{public_key_b64}",
        )

        challenge_response = (
            read_line(sock)
        )

        challenge_parts = (
            challenge_response.split("|")
        )

        if (
            len(challenge_parts) != 2
            or challenge_parts[0]
            != "CHALLENGE"
        ):
            raise ConnectionError(
                "Authentication challenge "
                "failed: "
                f"{challenge_response}"
            )

        challenge = (
            challenge_parts[1]
        )

        signature_b64 = (
            sign_challenge(
                private_key,
                challenge,
            )
        )

        send_line(
            sock,
            f"AUTH_RESPONSE|"
            f"{signature_b64}",
        )

        auth_response = (
            read_line(sock)
        )

        print(
            f"Auth:   "
            f"{auth_response}"
        )

        auth_parts = (
            auth_response.split("|")
        )

        if (
            len(auth_parts) != 2
            or auth_parts[0]
            != "AUTH_OK"
        ):
            raise ConnectionError(
                "Authentication failed: "
                f"{auth_response}"
            )

        server_fingerprint = (
            auth_parts[1]
        )

        if (
            server_fingerprint
            != local_fingerprint
        ):
            raise ConnectionError(
                "Identity fingerprint "
                "mismatch"
            )

        print(
            "Identity verified: "
            f"{local_fingerprint}"
        )

        print()
        print(
            "MiniPOW1 mining started."
        )
        print()

        while True:
            send_line(
                sock,
                "JOB",
            )

            response = (
                read_line(sock)
            )

            parts = response.split("|")

            if (
                len(parts) != 4
                or parts[0] != "JOB"
            ):
                raise ConnectionError(
                    "Unexpected job "
                    f"response: {response}"
                )

            job_id = parts[1]
            challenge = parts[2]
            target_hex = parts[3]

            if len(target_hex) != 64:
                raise ConnectionError(
                    "Invalid target "
                    f"received: {target_hex}"
                )

            print(
                f"Mining job {job_id}"
            )

            print(
                f"Target: "
                f"{target_hex[:16]}..."
            )

            (
                nonce,
                digest,
                attempts,
                speed,
                elapsed,
            ) = mine_job(
                challenge,
                target_hex,
            )

            print(
                f"Found nonce "
                f"{nonce:,} | "
                f"{attempts:,} attempts | "
                f"{speed:,.2f} "
                f"MiniPOW/s | "
                f"{elapsed:.2f}s"
            )

            send_line(
                sock,
                f"SUBMIT|"
                f"{job_id}|"
                f"{nonce}",
            )

            result = read_line(sock)

            if result.startswith(
                "ACCEPTED|"
            ):
                accepted += 1

                result_parts = (
                    result.split("|")
                )

                reward = (
                    result_parts[1]
                    if len(
                        result_parts
                    ) > 1
                    else "?"
                )

                print(
                    f"ACCEPTED! "
                    f"Reward {reward} MINI "
                    f"[{accepted} accepted / "
                    f"{rejected} rejected]"
                )

            else:
                rejected += 1

                print(
                    f"{result} "
                    f"[{accepted} accepted / "
                    f"{rejected} rejected]"
                )

            print()


def main():
    run_self_test()

    (
        private_key,
        public_key_b64,
        fingerprint,
        identity_created,
    ) = load_or_create_identity()

    if identity_created:
        print(
            "New persistent miner identity created."
        )
        print(
            f"Private key: {PRIVATE_KEY_FILE}"
        )
    else:
        print(
            "Existing persistent miner identity loaded."
        )

    print(
        f"Identity fingerprint: {fingerprint}"
    )
    print()

    while True:
        try:
            mining_session(
                private_key,
                public_key_b64,
                fingerprint,
            )

        except KeyboardInterrupt:
            print()
            print(
                "Miner stopped."
            )
            break

        except Exception as exc:
            print()

            print(
                f"Connection error: "
                f"{exc}"
            )

            print(
                "Reconnecting in "
                "5 seconds..."
            )

            print()

            time.sleep(5)


if __name__ == "__main__":
    main()