#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Typy wspieranych protoko��w IR.
	 */
	typedef enum {
		IR_PROTO_UNKNOWN = 0,            /**< Brak dopasowania */
		IR_PROTO_NEC,
		IR_PROTO_JVC,
		IR_PROTO_PANASONIC_KASEIKYO,
		IR_PROTO_SONY_SIRC,
		IR_PROTO_RC5,
		IR_PROTO_RC6,
		IR_PROTO_SHARP
	} ir_proto_t;

	/**
	 * @brief Klasy kodowania w protoko�ach IR.
	 */
	typedef enum { ENC_PULSE_DISTANCE, ENC_PULSE_WIDTH, ENC_MANCHESTER } ir_enc_t;

	/**
	 * @brief Specyfikacja czasowa i strukturalna protoko�u (definiowana tabelowo).
	 */
	typedef struct {
		const char* name;
		ir_proto_t   id;
		ir_enc_t     enc;
		uint8_t      lsb_first;
		uint8_t      bits_min, bits_max;
		uint16_t     hdr_mark_us, hdr_space_us;
		uint16_t     rpt_mark_us, rpt_space_us;
		uint16_t     T_us;
		uint8_t      tol_hdr_pct, tol_bit_pct;
		/* Pulse Distance */
		uint16_t     mark_us, space0_us, space1_us;
		/* Pulse Width */
		uint16_t     space_us, mark0_us, mark1_us;
		/* Manchester */
		uint16_t     bit_time_us, pre_mark_us, pre_space_us;
	} IRSpec;

	/**
	 * @brief Wynik detekcji + dekodowania.
	 * @note Dla protoko��w LSB-first @ref value ma bity LSB-first.
	 *       Dla RC5/RC6 u�ywamy MSB-first (�atwiej wydzieli� bity start/toggle).
	 */
	typedef struct {
		ir_proto_t   proto;     /**< Zidentyfikowany protok� */
		const char* name;      /**< Nazwa protoko�u */
		bool         is_repeat; /**< Flaga powt�rki (np. NEC repeat) */
		uint8_t      bits;      /**< Liczba zdekodowanych bit�w (je�li nie repeat) */
		uint64_t     value;     /**< Surowa warto�� bit�w (do 48 bit�w) */
	} IRDecodeOut;

	/**
	 * @brief Detekcja protoko�u i dekodowanie warto�ci na podstawie bufora odcink�w.
	 * @param segs  Naprzemienne czasy mark/space [�s].
	 * @param n     Liczba element�w w buforze.
	 * @return      Struktura @ref IRDecodeOut z wynikiem.
	 */
	IRDecodeOut  ir_detect_and_decode(const uint32_t* segs, uint8_t n);

	/**
	 * @brief Zwraca skr�tow� nazw� protoko�u.
	 */
	const char* ir_proto_name(ir_proto_t p);

#ifdef __cplusplus
}
#endif
