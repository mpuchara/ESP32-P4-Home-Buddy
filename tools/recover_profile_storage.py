"""Explicit recovery of headerless first-write data, after a private USB backup.

Default is offline inspection. Never repairs a committed bank or guesses that
unknown bytes are expendable. Execution requires explicit permission to discard
the uncommitted upper bank, and rechecks security, layout and every profile byte.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile

from flash_usb import package_plan
from flash_layout import parse_table

PROFILE_OFFSET = 0xb20000
BANK_BYTES = 0x80000
PARTITION_BYTES = 2 * BANK_BYTES
HEADER_BYTES = 256
PAYLOAD_BYTES = 312416


def validate_layout(table):
    profiles = [r for r in parse_table(table) if r['name'] == 'profiles']
    if len(profiles) != 1 or any(profiles[0][key] != expected for key, expected in
                                [('type', 1), ('subtype', 0x41), ('offset', PROFILE_OFFSET),
                                 ('size', PARTITION_BYTES), ('flags', 1)]):
        # Keep the unchanged table's historical encryption flag. Actual Flash
        # Encryption must be disabled, as checked from the device before erasing.
        raise ValueError('Expected the existing 1 MiB profiles partition and flags')


def recovery_plan(data):
    if len(data) != PARTITION_BYTES:
        raise ValueError("Expected an exact 1 MiB profile-partition backup")
    erased = b'\xff'
    if data[:BANK_BYTES] != erased * BANK_BYTES:
        raise ValueError("Lower bank is not blank; preserve it for offline recovery")
    upper = data[BANK_BYTES:]
    if upper[:HEADER_BYTES] != erased * HEADER_BYTES:
        raise ValueError("Upper header is not blank; preserve committed/unknown metadata")
    if upper == erased * BANK_BYTES:
        raise ValueError("Both banks are blank; no destructive recovery is needed")
    if upper[HEADER_BYTES + PAYLOAD_BYTES:] != erased * (BANK_BYTES - HEADER_BYTES - PAYLOAD_BYTES):
        raise ValueError("Data outside the schema-2 payload region; reviewed recovery required")
    # No commit marker/checksum exists. This is NOT a proof of valid biometric data.
    return PROFILE_OFFSET + BANK_BYTES, BANK_BYTES


def execute_recovery(data, table, port, runner):
    offset, size = recovery_plan(data)
    validate_layout(table)
    base = [sys.executable, '-m', 'esptool', '--chip', 'esp32p4', '--port', port,
            '--before', 'usb_reset', '--after', 'no_reset']

    def run(args):
        result = runner(base + args, text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError("USB operation failed; stop and retain the private backup")
        return result.stdout

    security = run(['get_security_info'])
    for label in ('Secure Boot', 'Flash Encryption'):
        values = [line.strip().split(':', 1)[1].strip() for line in security.splitlines()
                  if line.strip().startswith(label + ':')]
        if values != ['Disabled']:
            raise ValueError("Unknown or protected security state; recovery refused")
    with tempfile.TemporaryDirectory(prefix='home-buddy-profile-recovery-') as temporary:
        temporary = Path(temporary)
        current_table = temporary / 'table.bin'
        run(['read_flash', '0x10000', str(len(table)), str(current_table)])
        if current_table.read_bytes() != table:
            raise ValueError("Installed partition table differs; recovery refused")
        current = temporary / 'profiles-before.bin'
        run(['read_flash', hex(PROFILE_OFFSET), hex(PARTITION_BYTES), str(current)])
        if current.read_bytes() != data:
            raise ValueError("Profiles changed since the approved backup; recovery refused")
        run(['erase_region', hex(offset), hex(size)])
        after = temporary / 'profiles-after.bin'
        run(['read_flash', hex(PROFILE_OFFSET), hex(PARTITION_BYTES), str(after)])
        if after.read_bytes() != b'\xff' * PARTITION_BYTES:
            raise RuntimeError("Erase verification failed; stop and retain the private backup")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('backup', type=Path)
    parser.add_argument('--expected-sha256', required=True)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--port', required=True)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--discard-uncommitted-bank', action='store_true')
    args = parser.parse_args()
    data = args.backup.read_bytes()
    if hashlib.sha256(data).hexdigest() != args.expected_sha256:
        raise ValueError("Private backup checksum mismatch")
    _, table = package_plan(args.package, 'application')
    validate_layout(table)
    offset, size = recovery_plan(data)
    print(f'Reviewed scope: discard headerless upper-bank data at {offset:#x}, {size:#x} bytes')
    print('No committed database is established by this backup. No templates are printed.')
    if not args.execute:
        print('Offline dry run. No serial port opened; no Flash changed.')
        return
    if not args.discard_uncommitted_bank:
        raise ValueError("Explicit permission to discard the uncommitted bank is required")
    execute_recovery(data, table, args.port, subprocess.run)
    print('Upper bank erased and both blank banks read-verified. Private backup retained.')


if __name__ == '__main__':
    main()
