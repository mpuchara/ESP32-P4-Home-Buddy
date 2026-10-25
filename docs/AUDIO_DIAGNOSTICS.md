# Experimental microphone diagnostic

Implemented software; not physically accepted. Informacje > Diagnostyka zaawansowana >
Mikrofon opens a passive page. Start explicitly enables a maximum 60-second test;
Stop/leaving requests cleanup. No recording, HA audio, VAD or speaker inference.
The separate worker remains idle afterwards, owning no camera or AI model.

## Hardware evidence

GUITION's [I_W/Y specifications](https://www.guition.com/icms/upload/fb081940d6fc11f09850077a33e1404f/FTPData/UEditor/file/2026121/1768961095795/JC4880P443C_I_W%20Specifications-EN-V1.0.pdf)
identify the microphone position. Vendor schematics mirrored in the
[board archive](https://github.com/ultramcu/guition-jc4880p443c-i-w/tree/master/schematic),
originally in the [vendor package](https://pan.jczn1688.com/directlink/1/HMI%20display/JC4880P443C_I_W.zip),
show analog MIC1 MSM381A3729H9CP to ES8311 MIC1P, MIC1N grounded through R11,
and codec MICBIAS powering MIC1 through R16. Codec supply is 3.3 V; microphone
power is bias, not a GPIO. Existing pinned BSP mapping: MCLK13/BCLK12/LRCK10,
P4 DIN48 receives codec ASDOUT; P4 DOUT9 is the transmit/playback pin.
The previous iteration-8 interpretation reversed the P4 directions and was wrong.
Firmware now uses BOARD_P4_I2S_DIN rather than a literal. Sheet 01 identifies
ASDOUT (codec pin 7) as the ADC output and DSDIN (pin 9) as the DAC input;
sheet 05 identifies the analog MSM381A3729H9CP/MICBIAS route. GPIO direction is
cross-checked against the pinned BSP constants, not inferred from codec pin names. Installed variant/routing still needs physical confirmation.

Driver: pinned Espressif esp_codec_dev 1.6.2 and IDF 5.5.1 standard RX. Reuses
I2C0 SDA7/SCL8 without recreating bus or resetting touch/camera. Reads FD/FE chip ID
at 0x18 before codec writes, rejecting mismatch. ADC-only analog mic, I2S slave,
256*16000 = 4.096 MHz MCLK. No PA pin or DAC playback activation. Stereo 16-bit
DMA slots, left ADC slot converted to mono PCM. Actual clocks/slot/signal need
measurement; compilation is not microphone evidence.

Bounds: four descriptors * 320 stereo frames = 5120 DMA payload bytes; 640 int16
input + 320 mono samples = 1920 application bytes; task stack 6144 bytes. RX has
100 ms timeout, startup I2C transactions have bounded timeouts. PCM clears each
iteration. Metrics: DC-removed RMS, absolute peak, near-full-scale samples, frames,
capture errors, overflow lost samples, stack minimum. UI reports processed buffer
bytes/capacity, not an invented DMA occupancy percentage. Driver/task allocation
overhead and CPU need hardware measurement; payload budget is not total RAM cost.

## Operator acceptance

On authorized candidate installation, keep camera, AUTO, UBox and weather active.
Open page: no capture before Start. Start: verify codec ID and 16000 Hz, speak then
pause, cover/uncover microphone, inspect plausible RMS/peak. Check clipping at
normal distances and absence of permanent full-scale/DC. Measure clock/waveform
with test equipment where available. Stop mid-capture, leave page, repeat 20
starts: heap returns except retained idle task. Missing codec/read-timeout injection
uses a controlled fixture, not unplugging live components. Record free/largest
RAM, stack/CPU and JPEG/camera timing with/without audio. No physical result yet.

## VAD

Espressif [ESP-SR VAD](https://docs.espressif.com/projects/esp-sr/en/latest/esp32p4/audio_front_end/README.html)
is a candidate, subject to pinned component, P4/IDF and chunk-size verification.
Keep any future speech window bounded. No energy threshold is claimed as an
implemented VAD; voice activity does not identify the speaker.

The official P4 AFE exposes VAD; its model-partition defaults do not match this
layout. Prefer a separately pinned low-cost VAD after resource/ROM compatibility
checks; do not import the complete AFE or change partitions just for VAD.

## Iteration 9 routing and signal admission

Inspected the vendor-extracted sheets 01 and 05, the committed BSP pin map from
324970bade0d1f4e52880fe8016580368bc1e06e and IDF 5.5.1
`components/esp_driver_i2s/include/driver/i2s_std.h` (`din` = data input).
Driver source is the pinned esp_codec_dev 1.6.2 `device/es8311/es8311.c`:
ADC-only resume configures analog input (SYSTEM14, digital_mic=false), input
power/bias (SYSTEM0D/14), ADC gain (ADC16) and volume (ADC17); SDPOUT0A controls
ADC serial format. Its I2C adapter shifts the configured 0x30 address right one
bit; the pre-write identity probe uses the seven-bit 0x18 address. No second bus,
touch reset, speaker PA or playback is enabled. Close deletes codec/data/control
interfaces and disables/deletes the RX channel. Start rejects another request
while an earlier capture is starting/stopping, so cleanup cannot consume a new Start.

Stereo DMA is deliberately retained for slot diagnostics. Left is the candidate
mono stream, not a physically accepted channel assumption. The display also
reports right-slot RMS/peak, signed left DC and processed sample count. If only
the right slot responds to speech, capture is not accepted until its alignment
is corrected using a real receipt. Constant zero/full scale and exact periods
1..8 over at least four repeats are flagged as suspicious. Such a flag is a
heuristic and can also mark a periodic test tone; it does not identify speech.
RMS during silence is the observed noise level, not a calibrated noise floor.
There is still no signal capture receipt from the installed hardware revision.
