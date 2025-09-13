#pragma once
/**
 * @file ir_config.h
 * @brief Globalna konfiguracja dekodera IR (progi, bufory, tryby).
 *
 * Zmieniaj te makra w zale�no�ci od warunk�w pracy (�wiat�o, piloty).
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Tryb test�w on-target =====
 * Zdefiniuj IR_ENABLE_SELF_TESTS aby uruchomi� testy przez UART.
 */
 // #define IR_ENABLE_SELF_TESTS

 /* ===== Parametry akwizycji ===== */
#ifndef IR_MAX_SEGS
#define IR_MAX_SEGS      128u   ///< Maks. liczba odcink�w [mark, space, ...] w ramce
#endif

#ifndef IR_MIN_PULSE_US
#define IR_MIN_PULSE_US   80u   ///< Odrzucanie szumu poni�ej tej d�ugo�ci (�s)
#endif

#ifndef IR_GAP_US
#define IR_GAP_US      18000u   ///< Luka (cisza) ko�cz�ca ramk� (�s). 18�20 ms
#endif

#ifndef IR_MIN_SEGS
#define IR_MIN_SEGS       6u    ///< Minimalna liczba odcink�w, by uzna� bufor za ramk�
#endif

#ifdef __cplusplus
}
#endif