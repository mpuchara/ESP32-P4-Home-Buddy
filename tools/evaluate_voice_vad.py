"""Reproducible standalone libfvad admission probe, using synthetic PCM only.

Requires a checkout of dpirch/libfvad at PIN and a host C compiler. This is an
evaluation tool, not a device benchmark or speech accuracy test. No vendor code
is imported into firmware by running it.
"""
import argparse
import json
from pathlib import Path
import subprocess

PIN = '532ab666c20d3cfda38bca63abbb0f152706c369'
HARNESS = r'''
#include "fvad.h"
#include "vad/vad_core.h"
#include <stdio.h>
#include <math.h>
int main(void) {
    int16_t pcm[320];
    Fvad *vad=fvad_new(); if(!vad) return 2;
    printf("core_state_bytes=%zu\n",sizeof(VadInstT));
    for(unsigned fixture=0;fixture<4;++fixture) {
        fvad_reset(vad); fvad_set_sample_rate(vad,16000); fvad_set_mode(vad,3);
        unsigned positives=0; uint32_t seed=12345;
        for(unsigned frame=0;frame<151;++frame) {
            for(unsigned i=0;i<320;++i) {
                seed=seed*1664525u+1013904223u;
                int value=0;
                if(fixture==1) value=(int)(9000*sin((frame*320+i)*6.283185307*440/16000));
                if(fixture==2) value=(int)(seed>>16)-32768;
                if(fixture==3 && i==0) value=24000;
                pcm[i]=(int16_t)value;
            }
            int rv=fvad_process(vad,pcm,320); if(rv<0) return 3;
            positives += rv!=0;
        }
        printf("fixture=%u positive_frames=%u total_frames=151\n",fixture,positives);
    }
    fvad_free(vad);return 0;
}
'''


def evaluate(source, output, cc):
    revision = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    if revision != PIN:
        raise ValueError('Unpinned libfvad checkout')
    output.mkdir(parents=True, exist_ok=True)
    harness = output / 'vad-probe.c'
    harness.write_text(HARNESS)
    sources = sorted((source / 'src').rglob('*.c'))
    results = {'revision': PIN, 'device_test': False, 'speech_accuracy_validated': False}
    for name, options in [('sanitized', ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']),
                          ('normal', [])]:
        exe = output / ('vad-' + name)
        subprocess.run([cc, '-std=c11', '-O1', '-g', *options, '-I'+str(source/'include'),
                        '-I'+str(source/'src'), str(harness), *map(str, sources), '-lm', '-o', str(exe)], check=True)
        run = subprocess.run([str(exe.resolve())], capture_output=True, text=True, timeout=30)
        results[name] = {'exit_code': run.returncode, 'stdout': run.stdout, 'stderr': run.stderr}
    results['firmware_admitted'] = False
    (output/'vad-assessment.json').write_text(json.dumps(results, indent=2))
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--cc', default='cc')
    args = parser.parse_args()
    evaluate(args.source, args.output, args.cc)
