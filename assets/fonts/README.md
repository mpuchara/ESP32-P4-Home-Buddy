# Polish LVGL font

Noto Sans, SIL Open Font License 1.1 (OFL-NotoSans.txt).
Source: https://github.com/google/fonts/tree/main/ofl/notosans
Input: NotoSans[wdth,wght].ttf. Generated at 20 px, 4 bpp using lv_font_conv 1.5.3.
Committed output: components/ui/hb_font_20.c.
Included: ASCII, degree sign and every upper/lower Polish letter.

Reproduction with the downloaded source font:
```sh
npx --yes --package=lv_font_conv@1.5.3 lv_font_conv --size 20 --bpp 4 --format lvgl --font 'NotoSans[wdth,wght].ttf' --range 0x20-0x7e,0xb0,0x104-0x107,0x118-0x119,0x141-0x144,0xd3,0xf3,0x15a-0x15b,0x179-0x17c --lv-font-name hb_font_20 --output components/ui/hb_font_20.c
```
