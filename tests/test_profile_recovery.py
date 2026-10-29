"""Synthetic recovery boundaries; no physical USB operations or household data."""
from pathlib import Path
import subprocess
import struct
import hashlib
import contextlib
import io
import tempfile
from unittest.mock import patch
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from recover_profile_storage import (BANK_BYTES, HEADER_BYTES, PARTITION_BYTES,
                                     PAYLOAD_BYTES, PROFILE_OFFSET, execute_recovery,
                                     recovery_plan)
import recover_profile_storage


def interrupted():
    data = bytearray(b'\xff' * PARTITION_BYTES)
    data[BANK_BYTES + HEADER_BYTES:BANK_BYTES + HEADER_BYTES + 14368] = b'\0' * 14368
    return bytes(data)


class Recovery(unittest.TestCase):
    def test_execute_requires_separate_discard_authorization(self):
        data = interrupted()
        record = struct.pack('<HBBII16sI', 0x50aa, 1, 0x41, PROFILE_OFFSET,
                             PARTITION_BYTES, b'profiles', 1)
        table = record + b'\xeb\xeb' + b'\xff' * 14 + hashlib.md5(record).digest()
        with tempfile.TemporaryDirectory() as temporary:
            backup = Path(temporary) / 'synthetic.bin'
            backup.write_bytes(data)
            argv = ['recovery', str(backup), '--expected-sha256', hashlib.sha256(data).hexdigest(),
                    '--package', 'synthetic-package', '--port', 'COM_TEST']
            with patch.object(sys, 'argv', argv), \
                 patch.object(recover_profile_storage, 'package_plan', return_value=([], table)), \
                 patch.object(recover_profile_storage, 'execute_recovery') as execute, \
                 contextlib.redirect_stdout(io.StringIO()):
                recover_profile_storage.main()
                execute.assert_not_called()
                argv.append('--execute')
                with self.assertRaisesRegex(ValueError, 'Explicit permission'):
                    recover_profile_storage.main()
                execute.assert_not_called()
                argv.append('--discard-uncommitted-bank')
                recover_profile_storage.main()
                execute.assert_called_once()

    def test_bad_checksum_never_opens_usb_or_changes_backup(self):
        data = interrupted()
        with tempfile.TemporaryDirectory() as temporary:
            backup = Path(temporary) / 'synthetic.bin'
            backup.write_bytes(data)
            argv = ['recovery', str(backup), '--expected-sha256', '0' * 64,
                    '--package', 'unused', '--port', 'COM_TEST', '--execute',
                    '--discard-uncommitted-bank']
            with patch.object(sys, 'argv', argv), \
                 patch.object(recover_profile_storage, 'package_plan') as package, \
                 patch.object(recover_profile_storage.subprocess, 'run') as usb:
                with self.assertRaisesRegex(ValueError, 'checksum mismatch'):
                    recover_profile_storage.main()
                package.assert_not_called()
                usb.assert_not_called()
            self.assertEqual(backup.read_bytes(), data)

    def test_layout_and_security_refuse_ambiguous_states(self):
        data = interrupted()
        for key, value in [('type', 2), ('subtype', 0x40), ('offset', PROFILE_OFFSET + 4096),
                           ('size', PARTITION_BYTES - 4096), ('flags', 0)]:
            fields = dict(type=1, subtype=0x41, offset=PROFILE_OFFSET, size=PARTITION_BYTES, flags=1)
            fields[key] = value
            record = struct.pack('<HBBII16sI', 0x50aa, fields['type'], fields['subtype'],
                                 fields['offset'], fields['size'], b'profiles', fields['flags'])
            table = record + b'\xeb\xeb' + b'\xff' * 14 + hashlib.md5(record).digest()
            with self.subTest(key=key), self.assertRaises(ValueError):
                execute_recovery(data, table, 'COM_TEST', lambda *a, **kw: self.fail('USB opened'))
        record = struct.pack('<HBBII16sI', 0x50aa, 1, 0x41, PROFILE_OFFSET,
                             PARTITION_BYTES, b'profiles', 1)
        table = record + b'\xeb\xeb' + b'\xff' * 14 + hashlib.md5(record).digest()
        for security in ('Secure Boot: Enabled\nFlash Encryption: Disabled',
                         'Secure Boot: Disabled\nFlash Encryption: Enabled',
                         'Secure Boot: Disabled', 'Flash Encryption: Disabled',
                         'Secure Boot: DisabledX\nFlash Encryption: Disabled',
                         'Secure Boot: Disabled\nSecure Boot: Enabled\nFlash Encryption: Disabled', ''):
            calls = []
            def run(command, **kwargs):
                calls.append(command)
                return subprocess.CompletedProcess(command, 0, stdout=security)
            with self.subTest(security=security), self.assertRaises(ValueError):
                execute_recovery(data, table, 'COM_TEST', run)
            self.assertEqual(len(calls), 1)

    def test_scope_rejects_committed_unknown_and_unnecessary_erases(self):
        data = interrupted()
        self.assertEqual(recovery_plan(data), (0xba0000, BANK_BYTES))
        cases = [b'', b'\xff' * PARTITION_BYTES]
        for index in (0, BANK_BYTES, BANK_BYTES + HEADER_BYTES + PAYLOAD_BYTES):
            changed = bytearray(data)
            changed[index] = 0
            cases.append(bytes(changed))
        for changed in cases:
            with self.subTest(size=len(changed)):
                with self.assertRaises(ValueError):
                    recovery_plan(changed)

    def test_usb_rechecks_every_byte_before_the_only_erase(self):
        data = interrupted()
        record = struct.pack('<HBBII16sI', 0x50aa, 1, 0x41, PROFILE_OFFSET,
                             PARTITION_BYTES, b'profiles', 1)
        table = record + b'\xeb\xeb' + b'\xff' * 14 + hashlib.md5(record).digest()
        with self.assertRaises(ValueError):
            execute_recovery(data, b'invalid layout', 'COM_TEST', lambda *a, **kw: self.fail('USB opened'))
        for failure in ('none', 'security', 'table', 'drift', 'read', 'erase', 'verify'):
            calls = []

            def run(command, **kwargs):
                self.assertEqual(command[command.index('--after') + 1], 'no_reset')
                operation = command[command.index('--after') + 2:]
                calls.append(operation)
                if operation[0] == 'get_security_info':
                    out = '' if failure == 'security' else 'Secure Boot: Disabled\nFlash Encryption: Disabled'
                    return subprocess.CompletedProcess(command, 0, stdout=out)
                if operation[0] == 'read_flash':
                    if failure == 'read':
                        return subprocess.CompletedProcess(command, 1, stdout='')
                    if operation[1] == '0x10000':
                        content = b'wrong table' if failure == 'table' else table
                    elif any(x[0] == 'erase_region' for x in calls):
                        content = data if failure == 'verify' else b'\xff' * PARTITION_BYTES
                    else:
                        content = data[:-1] + b'\0' if failure == 'drift' else data
                    Path(operation[-1]).write_bytes(content)
                if operation[0] == 'erase_region' and failure == 'erase':
                    return subprocess.CompletedProcess(command, 1, stdout='')
                return subprocess.CompletedProcess(command, 0, stdout='')

            if failure == 'none':
                execute_recovery(data, table, 'COM_TEST', run)
            else:
                with self.assertRaises((ValueError, RuntimeError)):
                    execute_recovery(data, table, 'COM_TEST', run)
            erases = [x for x in calls if x[0] == 'erase_region']
            if failure in ('security', 'table', 'drift', 'read'):
                self.assertEqual(erases, [])
            else:
                self.assertEqual(erases, [['erase_region', hex(PROFILE_OFFSET + BANK_BYTES), hex(BANK_BYTES)]])
            self.assertFalse(any(x[0] in ('write_flash', 'erase_flash', 'erase_chip') for x in calls))


if __name__ == '__main__':
    unittest.main()
