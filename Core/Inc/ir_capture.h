#pragma once
#include "main.h"
#include "ir_config.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Inicjalizuje referencjê do timera/kana³u u¿ywanego do przechwytywania ramki.
	 * @param htim    Uchwyt HAL do TIM (np. &htim2).
	 * @param channel Kana³ input capture (np. TIM_CHANNEL_1).
	 */
	void IR_Capture_AttachTimer(TIM_HandleTypeDef* htim, uint32_t channel);

	/**
	 * @brief Startuje licznik i w³¹cza Input Capture z przerwaniem.
	 */
	void IR_Capture_Start(void);

	/**
	 * @brief ISR helper: wo³aj w HAL_TIM_IC_CaptureCallback.
	 * @details ISR zapisuje czasy, wykrywa lukê koñca ramki i robi zrzut.
	 */
	void IR_Capture_ISR(TIM_HandleTypeDef* htim);

	/**
	 * @brief Pobiera gotow¹ ramkê (jeœli dostêpna) do bufora u¿ytkownika.
	 * @param[out] out   Bufor czasów (µs).
	 * @param[out] count Liczba skopiowanych elementów.
	 * @return true gdy ramka by³a gotowa (i zosta³a pobrana).
	 */
	bool IR_Capture_TakeFrame(uint32_t* out, uint8_t* count);

	/**
	 * @brief Zwraca bie¿¹c¹ liczbê zebranych odcinków (diagnostyka).
	 */
	uint8_t IR_Capture_CurrentCount(void);

#ifdef __cplusplus
}
#endif
