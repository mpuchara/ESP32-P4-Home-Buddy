"""Extract unmodified reference math from pinned Espressif source, never from firmware.

Run with --source local/iteration11 or omit it to fetch the pinned public sources.
The emitted header is build-only; no recordings, models or biometric data.
"""
import argparse
import hashlib
from pathlib import Path
from urllib.request import urlopen

PIN = 'eb72563e6a4e61d2bbc0d46e4c79f2c9f816c3c6'
FILES = {
    'dl_audio_common.cpp': ('esp-dl/audio/common/', '2a297510d30eeff5f3881ba14e211284536603b13335f06014628883c50c571d'),
    'dl_fbank.cpp': ('esp-dl/audio/speech_features/', '36f909d46ddbb2a165244411ca83b4251963b76689928e666cd67829c697cfa9'),
}


def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


def generate(output, folder=None):
    sources = {}
    for name, (prefix, digest) in FILES.items():
        raw = (folder / name).read_bytes() if folder else urlopen(
            f'https://raw.githubusercontent.com/espressif/esp-dl/{PIN}/{prefix}{name}', timeout=30).read()
        raw = raw.replace(b'\r\n', b'\n')
        if hashlib.sha256(raw).hexdigest() != digest:
            raise ValueError(f'Unpinned reference: {name}')
        sources[name] = raw.decode()
    common = sources['dl_audio_common.cpp']
    selected = ['float *win_func_init(', 'void remove_dc_offset(', 'float hz2mel(',
                'float compute_energy(', 'mel_filter_t *mel_filter_init(',
                'void mel_filter_deinit(', 'float *apply_preemphasis(',
                'float *apply_window(', 'float *compute_spectrum(', 'float *mel_dotprod(']
    code = '\n'.join(function(common, x) for x in selected)
    code += function(sources['dl_fbank.cpp'], 'esp_err_t Fbank::process_frame(const float *')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('// Espressif ESP-DL MIT, pinned ' + PIN + '\n' + code, encoding='utf-8')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--source', type=Path)
    args = parser.parse_args()
    generate(args.output, args.source)
