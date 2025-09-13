Uniwersalny odbiornik + dekoder IR (NEC/JVC/Panasonic Kaseikyo/Sony/Sharp/RC5/RC6) na **STM32L432KC** z TSOP2236/2238.

## Funkcje
- **Capture**: pomiar obwiedni (mark/space) z rozdzielczością 1 µs (TIM2 Input Capture na oba zbocza, filtr IC).
- **Detekcja + dekodowanie**: tabela parametrów protokołów (Pulse Distance / Pulse Width) + Manchester (RC5/RC6).
- **Testy on-target**: syntetyczne ramki w pamięci → dekoder → log przez UART. (Włączane flagą `IR_ENABLE_SELF_TESTS`).

## Wymagania
- Płytka **NUCLEO-L432KC** lub inny STM32L4 z TIM2_CH1 na PA0.
- Odbiornik IR: **TSOP2236** (36 kHz) lub **TSOP2238** (38 kHz).
- STM32CubeIDE (HAL)

## Połączenia (NUCLEO-L432KC)
- **3V3** ← TSOP VIN (kondensatory: 100 nF + 4.7 µF blisko TSOP)
- **GND** ← TSOP GND
- **PA0** ← TSOP OUT
- **UART2**: TX = PA2; RX = PA3 konwerter uart->usb (standard w nucleo) (115200 8N1).
- **LED**: PB3 (LD3).

## Tryby pracy
- **Testy on-target (UART)** – włącz przez zdefiniowanie symbolu `IR_ENABLE_SELF_TESTS`.
- **Praca z TSOP** – domyślnie, gdy flaga testów nie jest zdefiniowana.

### Włączenie testów (IR_ENABLE_SELF_TESTS)
- CubeIDE: *Project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Preprocessor → Defined symbols* → dodaj `IR_ENABLE_SELF_TESTS`.
- Zdefiniowanie flagi w `firmware/App/ir_config.h`

Po włączeniu testów firmware uruchomi `IR_Tests_RunUART()` (nie wraca) i będzie raportował PASS/FAIL przez UART, a LED będzie migał (wolno = PASS, szybko = FAIL).

## Konfiguracja
Wszystkie progi i bufory są w `firmware/App/ir_config.h`:
- `IR_MIN_PULSE_US` – odrzucanie szumów < 80 µs,
- `IR_GAP_US` – luka kończąca ramkę ~18 ms,
- `IR_MAX_SEGS` – rozmiar bufora,
- `IR_ENABLE_SELF_TESTS` – (opcjonalna) flaga testów.

Protokoły:
- NEC (24/32): 9000/4500 µs, T~560 µs (repeat: 9000/2250).
- JVC (16/32): 8400/4200, T~525 µs.
- Panasonic Kaseikyo (~48): 3500/1750, T~435 µs.
- Sony SIRC (12/15/20): 2400/600; stała przerwa 600; mark 600/1200.
- Sharp (~15): mark ~320, space ~1000/2000.
- RC5 (14): Manchester, T=889 µs (bit=2T), brak lidera.
- RC6 (20/32): Manchester, lider 6T/2T, T=444 µs, toggle 2T (4 półbity).