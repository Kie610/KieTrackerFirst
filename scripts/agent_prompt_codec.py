#!/usr/bin/env python3
"""Encode and verify byte-reversible agent-transfer prompt envelopes."""

from __future__ import annotations

import argparse
import base64
import gzip
import hashlib
import pathlib
import sys


MAGIC = "AX1"
EXECUTION_RULE = (
    "x=b64d(p)>gunzip>b;assert len(b)=n&sha256(b)=h;"
    "utf8(b)=>task;execute task exactly;preserve MUST/NEVER;ask only if blocked"
)


def encode(payload: bytes) -> str:
    payload.decode("utf-8", errors="strict")
    compressed = gzip.compress(payload, compresslevel=9, mtime=0)
    digest = hashlib.sha256(payload).hexdigest()
    data = base64.b64encode(compressed).decode("ascii")
    return (
        f"{MAGIC}|u=utf-8|c=gzip|e=base64|n={len(payload)}|"
        f"z={len(compressed)}|h={digest}\n"
        f"p={data}\n{EXECUTION_RULE}\n"
    )


def decode(envelope: str) -> bytes:
    lines = [line.strip() for line in envelope.splitlines() if line.strip()]
    if len(lines) != 3 or not lines[0].startswith(f"{MAGIC}|"):
        raise ValueError("invalid AX1 envelope")
    fields: dict[str, str] = {}
    for field in lines[0].split("|")[1:]:
        key, value = field.split("=", 1)
        if key in fields:
            raise ValueError(f"duplicate AX1 field: {key}")
        fields[key] = value
    if set(fields) != {"u", "c", "e", "n", "z", "h"}:
        raise ValueError("invalid AX1 fields")
    if fields.get("u") != "utf-8" or fields.get("c") != "gzip" or fields.get("e") != "base64":
        raise ValueError("unsupported AX1 encoding")
    if not lines[1].startswith("p=") or lines[2] != EXECUTION_RULE:
        raise ValueError("invalid AX1 payload or execution rule")
    compressed = base64.b64decode(lines[1][2:], validate=True)
    if len(compressed) != int(fields["z"]):
        raise ValueError("compressed length mismatch")
    payload = gzip.decompress(compressed)
    if len(payload) != int(fields["n"]):
        raise ValueError("payload length mismatch")
    if hashlib.sha256(payload).hexdigest() != fields["h"]:
        raise ValueError("payload SHA-256 mismatch")
    payload.decode("utf-8", errors="strict")
    return payload


def main() -> int:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)
    encode_parser = subparsers.add_parser("encode")
    encode_parser.add_argument("source", type=pathlib.Path)
    encode_parser.add_argument("output", type=pathlib.Path)
    decode_parser = subparsers.add_parser("decode")
    decode_parser.add_argument("source", type=pathlib.Path)
    decode_parser.add_argument("output", type=pathlib.Path)
    verify_parser = subparsers.add_parser("verify")
    verify_parser.add_argument("source", type=pathlib.Path)

    args = parser.parse_args()
    if args.command == "encode":
        with args.output.open("w", encoding="ascii", newline="\n") as output_file:
            output_file.write(encode(args.source.read_bytes()))
        return 0
    payload = decode(args.source.read_text(encoding="ascii"))
    if args.command == "decode":
        args.output.write_bytes(payload)
    else:
        print(f"OK bytes={len(payload)} sha256={hashlib.sha256(payload).hexdigest()}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyError, OSError, UnicodeError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
