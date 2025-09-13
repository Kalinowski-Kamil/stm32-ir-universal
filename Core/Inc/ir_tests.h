#pragma once
#include "ir_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IR_ENABLE_SELF_TESTS
	/**
	 * @brief Uruchamia zestaw testów na docelowej p³ytce, raportuje przez UART
	 *        i mruga LED (wolno = PASS, szybko = FAIL). Funkcja nie wraca.
	 */
	void IR_Tests_RunUART(void);
#endif

#ifdef __cplusplus
}
#endif