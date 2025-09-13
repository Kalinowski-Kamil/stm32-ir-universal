#pragma once
/**
 * @file ir_config.h
 * @brief Globalna konfiguracja dekodera IR (progi, bufory, tryby).
 *
 * Zmieniaj te makra w zale¿noœci od warunków pracy (œwiat³o, piloty).
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Tryb testów on-target =====
 * Zdefiniuj IR_ENABLE_SELF_TESTS aby uruchomiæ testy przez UART.
 */
 // #define IR_ENABLE_SELF_TESTS

 /* ===== Parametry akwizycji ===== */
#ifndef IR_MAX_SEGS
#define IR_MAX_SEGS      128u   ///< Maks. liczba odcinków [mark, space, ...] w ramce
#endif

#ifndef IR_MIN_PULSE_US
#define IR_MIN_PULSE_US   80u   ///< Odrzucanie szumu poni¿ej tej d³ugoœci (µs)
#endif

#ifndef IR_GAP_US
#define IR_GAP_US      18000u   ///< Luka (cisza) koñcz¹ca ramkê (µs). 18–20 ms
#endif

#ifndef IR_MIN_SEGS
#define IR_MIN_SEGS       6u    ///< Minimalna liczba odcinków, by uznaæ bufor za ramkê
#endif

#ifdef __cplusplus
}
#endif