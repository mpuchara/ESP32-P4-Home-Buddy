# Polish LVGL font

Noto Sans, SIL Open Font License 1.1 (OFL-NotoSans.txt).
Source: https://github.com/google/fonts/tree/45e3d5269e4a17d823f6d84470bc084dc5ae46f6/ofl/notosans
Input SHA256: bfb7bb691513f12e734dc346c03a03f784912432d7e3fa8e56efcf906fe86b3d.
Input: NotoSans[wdth,wght].ttf. Generated at 20 px, 4 bpp without compression using lv_font_conv 1.5.3.
Committed output: components/ui/hb_font_20.c.
Included: ASCII, degree sign and every upper/lower Polish letter.
The generated include uses lvgl.h for the ESP-IDF component. Uncompressed bitmaps
work with the firmware's disabled LV_USE_FONT_COMPRESSED setting. The host font
test uses the actual LVGL 8.3.11 decoder with that setting and checks visible
bitmap data for ASCII and all Polish letters, not only glyph-map entries.

Reproduction with the downloaded source font:
```sh
npx --yes --package=lv_font_conv@1.5.3 lv_font_conv --size 20 --bpp 4 --format lvgl --no-compress --lv-include lvgl.h --font 'NotoSans[wdth,wght].ttf' --range 0x20-0x7e,0xb0,0x104-0x107,0x118-0x119,0x141-0x144,0xd3,0xf3,0x15a-0x15b,0x179-0x17c --lv-font-name hb_font_20 --output components/ui/hb_font_20.c
```
