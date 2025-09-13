#pragma once
#include "main.h"
#include "ir_config.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Inicjalizuje referencj� do timera/kana�u u�ywanego do przechwytywania ramki.
	 * @param htim    Uchwyt HAL do TIM (np. &htim2).
	 * @param channel Kana� input capture (np. TIM_CHANNEL_1).
	 */
	void IR_Capture_AttachTimer(TIM_HandleTypeDef* htim, uint32_t channel);

	/**
	 * @brief Startuje licznik i w��cza Input Capture z przerwaniem.
	 */
	void IR_Capture_Start(void);

	/**
	 * @brief ISR helper: wo�aj w HAL_TIM_IC_CaptureCallback.
	 * @details ISR zapisuje czasy, wykrywa luk� ko�ca ramki i robi zrzut.
	 */
	void IR_Capture_ISR(TIM_HandleTypeDef* htim);

	/**
	 * @brief Pobiera gotow� ramk� (je�li dost�pna) do bufora u�ytkownika.
	 * @param[out] out   Bufor czas�w (�s).
	 * @param[out] count Liczba skopiowanych element�w.
	 * @return true gdy ramka by�a gotowa (i zosta�a pobrana).
	 */
	bool IR_Capture_TakeFrame(uint32_t* out, uint8_t* count);

	/**
	 * @brief Zwraca bie��c� liczb� zebranych odcink�w (diagnostyka).
	 */
	uint8_t IR_Capture_CurrentCount(void);

#ifdef __cplusplus
}
#endif
