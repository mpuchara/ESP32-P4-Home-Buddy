"""Pinned SV 3s/PDL3 image contract shared by packaging, budget checks and tests."""
import hashlib
import struct

MODEL = "sv_tdnn_tiny_3s.espdl"
RAW_SIZE = 2844528
OFFSET = 96
IMAGE_SIZE = RAW_SIZE + OFFSET
RAW_SHA = "736ca24bc3430db65b063ce4af7341904d1970d8505aa2eaab0d7be9e760c42e"
IMAGE_SHA = "b503ff049a0cd5c62b4c1ac337cf20b7de1549857a4d5329a1821cfb55e9b3a8"


def validate_raw(data):
    if len(data) != RAW_SIZE or hashlib.sha256(data).hexdigest() != RAW_SHA:
        raise ValueError("Unexpected speaker weights")


def validate_image(data, partition_size=0x300000):
    if len(data) != IMAGE_SIZE or len(data) > partition_size:
        raise ValueError("Model image/partition size mismatch")
    if data[:4] != b"PDL3" or data[4:20] != b"v1.0" + bytes(12):
        raise ValueError("Unsupported model package version")
    if struct.unpack_from("<I", data, 20)[0] != IMAGE_SIZE or struct.unpack_from("<4I", data, 56) != (1, OFFSET, 72, len(MODEL)):
        raise ValueError("Invalid package index")
    if data[72:72 + len(MODEL)] != MODEL.encode() or any(data[72 + len(MODEL):OFFSET]):
        raise ValueError("Unexpected model name/padding")
    if hashlib.sha256(data).hexdigest() != IMAGE_SHA:
        raise ValueError("Model package SHA256 mismatch")
    validate_raw(data[OFFSET:])
