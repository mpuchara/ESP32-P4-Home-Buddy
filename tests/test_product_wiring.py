"""Guard firmware wiring that cannot be exercised without the ESP-IDF peripheral runtime."""
from pathlib import Path
import re
import unittest
ROOT = Path(__file__).resolve().parents[1]

class ProductWiring(unittest.TestCase):
    def test_audio_direction(self):
        pins = (ROOT / 'components/board/vendor/board_p4_pins.h').read_text(encoding='utf-8')
        audio = (ROOT / 'components/audio/audio_service.c').read_text(encoding='utf-8')
        for name, gpio in [('DIN', 48), ('DOUT', 9), ('MCLK', 13), ('BCLK', 12), ('LRCK', 10)]:
            self.assertRegex(pins, rf'#define BOARD_P4_I2S_{name}\s+{gpio}\b')
        self.assertIn('.din = BOARD_P4_I2S_DIN', audio)
        self.assertIn('.dout = I2S_GPIO_UNUSED', audio)
        self.assertIn('.pa_pin = -1', audio)
        self.assertIn('.digital_mic = false', audio)
        self.assertIn('i2c_master_get_bus_handle', audio)
        self.assertNotIn('i2c_new_master_bus', audio)
        self.assertIn('60000000', audio)

    def test_primary_navigation(self):
        ui = (ROOT / 'components/ui/ui.c').read_text(encoding='utf-8')
        main = ui.split('const hb_menu_item main_items[4] =', 1)[1].split('};', 1)[0]
        self.assertEqual(re.findall(r'\{"([^"]+)"', main),
                         ['Domownicy', 'Rozpoznawanie', 'Ustawienia', 'Informacje'])
        recognition = ui.split('static void recognition_page(lv_event_t *e)\n{', 1)[1].split('static void identity_guest', 1)[0]
        self.assertNotIn('HB_ID_CMD_BACKGROUND', recognition)
        self.assertNotIn('HB_ID_CMD_TEST', recognition)
        self.assertIn('HB_ID_CMD_AUTO', recognition)
        household = ui.split('static void household_page(lv_event_t *e)\n{', 1)[1].split('static void profile_page', 1)[0]
        self.assertNotIn('Odśwież', household)
        self.assertIn('profile_ids[i] = s.profiles[i].id', household)
        self.assertIn('hb_product_timeout', ui)
        self.assertIn('keyboard && !lv_obj_has_flag', ui)
        self.assertIn('enrollment_observed && s.saved_id', ui)

if __name__ == '__main__':
    unittest.main()
