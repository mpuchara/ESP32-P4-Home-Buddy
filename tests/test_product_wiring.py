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
        self.assertIn('profile_ids[i] = s->profiles[i].id', household)
        self.assertIn('hb_product_timeout', ui)
        self.assertIn('keyboard && !lv_obj_has_flag', ui)
        self.assertIn('enrollment_observed && s->saved_id', ui)

    def test_persistent_enrollment_receipts_and_worker_order(self):
        service = (ROOT / 'components/identity/identity_service.cpp').read_text(encoding='utf-8')
        ui = (ROOT / 'components/ui/ui.c').read_text(encoding='utf-8')
        worker = service.split('static void worker(void *)', 1)[1]
        self.assertLess(worker.index('persistence_start(state)'), worker.index('hb_config_identity_auto()'))
        save = worker.split('case HB_ID_CMD_SAVE:', 1)[1].split('case HB_ID_CMD_TEST:', 1)[0]
        self.assertLess(save.index('if (persistence_commit'), save.index('state.saved_id ='))
        create = worker.split('case HB_ID_CMD_CREATE:', 1)[1].split('case HB_ID_CMD_RENAME:', 1)[0]
        self.assertLess(create.index('persistence_commit'), create.index('state.created_id ='))
        deletion = worker.split('case HB_ID_CMD_DELETE:', 1)[1].split('case HB_ID_CMD_BEGIN:', 1)[0]
        self.assertLess(deletion.index('hb_household_voice_reconcile'), deletion.index('persistence_commit'))
        failure = service.split('if (r != HB_STORE_OK)', 1)[1].split('storage_dirty = false', 1)[0]
        self.assertNotIn('hb_profile_v2_restore', failure)  # Never lose new RAM templates on save failure.
        self.assertIn('hb_compute_enter(1000)', service)
        self.assertIn('~ComputeLease()', service)
        self.assertIn('HB_ID_CMD_RETRY_SAVE', ui)
        self.assertIn('s->persistence_message', ui)
        self.assertIn('s->saved_id == identity_selected &&', ui)
        self.assertNotIn('Profile tymczasowe - znikną po restarcie.', ui)
        self.assertIn('hb_config_biometric_notice()', ui)
        self.assertIn('HB_ID_CMD_NOTICE_ACK', ui)
        self.assertIn('load_attempts >= 3', worker)
        self.assertIn('next_load_ms = now_ms() + 30000', worker)

if __name__ == '__main__':
    unittest.main()
