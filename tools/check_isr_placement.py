"""Check actual linked camera/display ISR entrypoints and frame-pool callees."""
import json
from pathlib import Path
import sys

REQUIRED = ('camera_next_buffer', 'camera_frame_finished', 'hb_frame_reserve',
            'hb_frame_abort', 'hb_frame_publish', 'hb_frame_complete',
            'csi_dma_trans_done_callback', 'refresh_done')


def validate(sections):
    for name in REQUIRED:
        if name not in sections:
            raise ValueError(f'Missing linked ISR/callee: {name}')
        if not any(section.startswith('.iram') for section in sections[name]):
            raise ValueError(f'Flash-resident ISR/callee: {name}: {sections[name]}')


def main():
    from elftools.elf.elffile import ELFFile
    config = json.loads(Path(sys.argv[2]).read_text())
    if not config.get('HB_BOARD_PROFILE_CONFIRMED'):
        print('ISR placement: peripherals disabled in diagnostics profile')
        return
    sections = {}
    with Path(sys.argv[1]).open('rb') as source:
        elf = ELFFile(source)
        for symbol in elf.get_section_by_name('.symtab').iter_symbols():
            if symbol.name in REQUIRED and isinstance(symbol['st_shndx'], int):
                section = elf.get_section(symbol['st_shndx']).name
                sections.setdefault(symbol.name, []).append(section)
    validate(sections)
    print(f'Linked ISR placement verified: {len(REQUIRED)} camera/display functions in IRAM')


if __name__ == '__main__':
    main()
