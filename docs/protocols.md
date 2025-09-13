Protokoły IR – skrót timingów

- **NEC (24/32)**: nagłówek ~9000/4500 µs, T~560 µs (0: 1T/1T, 1: 1T/3T), repeat ~9000/2250.
- **JVC (16/32)**: ~8400/4200, T~525 µs.
- **Panasonic Kaseikyo (~48)**: ~3500/1750, T~435 µs.
- **Sony SIRC (12/15/20)**: start 2400/600; space stałe ~600; mark 600 (0) / 1200 (1).
- **Sharp (~15)**: mark ~320; space ~1000 (0) / 2000 (1).
- **RC5 (14)**: Manchester, T=889 µs (bit=2T), brak lidera; start bity 1,1; bit toggle obecny.
- **RC6 (20/32)**: Manchester, lider 6T/2T, T=444 µs; bit toggle ma długość 2T (czyli 4 półbity).