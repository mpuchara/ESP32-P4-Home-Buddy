"""Pinned MBF/PDL3 image contract shared by packaging, budget checks and tests."""
import hashlib
import struct

MODEL = "human_face_feat_mbf_s8_v1.espdl"
RAW_SIZE = 3522832
OFFSET = 112
IMAGE_SIZE = RAW_SIZE + OFFSET
RAW_SHA = "0cd2629f7111b821f419531d0d3ff33d81018391fa57039acd1022a4531cfd48"
IMAGE_SHA = "f8aa1d21c47814bf324b78ec3a050f993dfa38b1f6a582b6afb5ce0264daf294"


def validate_raw(data):
    if len(data) != RAW_SIZE or hashlib.sha256(data).hexdigest() != RAW_SHA:
        raise ValueError("Unexpected MBF weights")


def validate_image(data, partition_size=0x400000):
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
