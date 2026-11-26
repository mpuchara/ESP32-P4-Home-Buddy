# Home Buddy 0.15.0 — testy na ESP32-P4

Kandydat do **testów**, nie wydanie produkcyjne. Wymaga GUITION ESP32-P4
z istniejącą tablicą single-factory-v1 i wcześniej wgranymi modelami MBF
oraz sv_model_3s. Aktualizacja wyłącznie aplikacji 0x20000. Nie kasować Flash,
NVS, profili ani modeli. Jeśli test L/R zgłasza brak modelu, sprawdzić osobną
procedurę speaker-model w docs/USB_FLASHING.md; nie wykonywać full flash.
Bez fizycznego potwierdzenia nie zmieniać ustawień bezpieczeństwa ani partycji.

## Procedura

1. Chrome/Edge na komputerze: otworzyć opublikowany instalator, zamknąć inne
   monitory USB, wgrać i sprawdzić wersję 0.15.0-voice-test na urządzeniu.
2. Włączyć monitor USB 115200 na tej samej stronie. Sprawdzić kamerę, kolory,
   istniejące profile, wygaszacz oraz kalibrację jasności (dzień/noc).
3. Diagnostyka audio: cisza i mowa, następnie Model 3 s: slot L (jeśli nie
   działa, R). Mówić samodzielnie co najmniej 3–4 s. Oczekiwany model 512D,
   poprawna częstotliwość 16 kHz, brak błędów i zwolnienie PCM.
4. Domownicy: wybrać istniejący profil, zaakceptować lokalny zapis biometrii,
   zarejestrować cztery różne wypowiedzi, zweryfikować piątą, zapisać.
   Powtórzyć dla drugiej osoby. Tryb ręczny TEST: po 10 wypowiedzi każdej
   osoby, po 10 osoby niezarejestrowanej; zanotować KNOWN/UNKNOWN/AMBIGUOUS.
5. Rozpoznawanie > Głos AUTO TEST: po 10 prób w ciszy, z odległości 0.5–2 m,
   z wentylacją, telewizorem, muzyką, oraz w rozmowie dwóch osób. Weryfikować
   rozpoznanie po dwóch niezależnych oknach. Notować pomyłki i brak wyniku.
6. Twarz + głos TEST: ta sama osoba, twarz bez mowy, głos bez widocznej
   twarzy, różne osoby widoczna/mówiąca, dwie twarze. Sprawdzić konflikt,
   wagi i wygaśnięcie starego wyniku po 5 s. Zmiana trybu i STOP powinny
   anulować poprzednią sesję. Nie traktować zgodności jako dowodu źródła mowy.
7. Test stabilności: 30 min głos AUTO + 30 min twarz/głos, krótkie przerwy,
   przejście do wygaszacza, kalibracja jasności w trakcie, zapis/odczyt profilu.
   Ponowny restart: profile zachowane, nasłuch głosowy wyłączony do świadomego
   uruchomienia; ponowny test L/R jest wymagany.
8. Zatrzymać monitor i pobrać TXT. Przejrzeć dane osobowe przed udostępnieniem.
   Nie wysyłać nagrań PCM ani prywatnych kopii Flash.

## Diagnostyka

VOICE_WINDOW: end, err, screen (0 pass, 1 invalid, 2 low activity, 3 broadband,
4 stationary, 5 VAD), stale, empty, quality, ai_ms, skipped, more. VOICE_AUTO:
liczba okien, odrzuconych, starych, błędów, potwierdzeń i backoff.
PCM: RMS, clipping, DC, DMA drops, pamięć i stack. Zbierać czasy modelu,
liczby restartów i pomyłek. Ocena FAR/FRR, wydajności i stabilności wymaga
rzeczywistej próby; nie ma jeszcze wiarygodnego progu produkcyjnego.

Nie publikujemy zdarzeń głosowych ani BOTH do HA; VAD nie rozróżnia wiarygodnie
telewizora od człowieka i nie wykonuje diarizacji. Jest to zakres testów,
nie potwierdzona funkcjonalność.
