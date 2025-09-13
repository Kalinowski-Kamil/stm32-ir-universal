#Architektura i decyzje projektowe

## Moduły
- `ir_capture`: pomiar obwiedni (mark/space) z TIM2 IC (oba zbocza, 1 MHz).
  - ISR zapisuje odcinki, detekcja luki `IR_GAP_US` → snapshot ramki.
  - Brak printf w ISR. Minimum pracy w przerwaniu.
- `ir_decode`: detekcja + dekodowanie:
  - Tabela `IRSpec[]` opisuje protokoły (encoding, nagłówek, tolerancje, T).
  - Wspólne dekodery: Pulse Distance / Pulse Width.
  - Dedykowane: Manchester (RC5, RC6) – rekonstrukcja półbitów T.
- `ir_tests`: testy on-target (syntetyczne ramki) – warunkowo przez `IR_ENABLE_SELF_TESTS`.

## Parametry
- **Zegar**: 80 MHz SYSCLK, TIM2 preskaler 79 → **1 MHz** (1 µs/inkrement).
- **Filtracja**: ICFilter=8; programowy próg `IR_MIN_PULSE_US=80 µs`; luka końca `IR_GAP_US=18 ms`.
- **Bufor**: `IR_MAX_SEGS=128` – bezpieczne dla długich ramek (Kaseikyo ~48 bitów).

## Dlaczego 1 MHz
Progi czasowe protokołów są rzędu 0.3–2 ms. Rozdzielczość 1 µs upraszcza tolerancje i arytmetykę bez float.