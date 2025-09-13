#include "ir_capture.h"

/* Zmienne o obs³ugi przerwañ i buforowania wyników */
static TIM_HandleTypeDef* s_tim = NULL;
static uint32_t s_channel = TIM_CHANNEL_1;

static volatile uint32_t segs[IR_MAX_SEGS];
static volatile uint8_t  seg_cnt = 0;
static volatile uint32_t last_cc = 0;

static volatile uint32_t frame_buf[IR_MAX_SEGS];
static volatile uint8_t  frame_cnt = 0;
static volatile bool     frame_ready = false;

void IR_Capture_AttachTimer(TIM_HandleTypeDef* htim, uint32_t channel) {
    s_tim = htim; s_channel = channel;
}

void IR_Capture_Start(void) {
    if (!s_tim) return;
    HAL_TIM_Base_Start(s_tim);
    HAL_TIM_IC_Start_IT(s_tim, s_channel);
}

void IR_Capture_ISR(TIM_HandleTypeDef* htim) {
    if (!s_tim || htim != s_tim) return;

    /* SprawdŸ, który kana³ by³ aktywny (Cube/HAL dostarcza) */
    uint32_t active = HAL_TIM_GetActiveChannel(htim);
    if (s_channel == TIM_CHANNEL_1 && active != HAL_TIM_ACTIVE_CHANNEL_1) return;

    /* Ró¿nica czasów na CCR (1 µs) */
    uint32_t cc = HAL_TIM_ReadCapturedValue(htim, s_channel);
    uint32_t dt = cc - last_cc;
    last_cc = cc;

    /* Odrzuæ bardzo krótkie czasy */
    if (dt < IR_MIN_PULSE_US) return;

    /* Luka koñcz¹ca ramkê: zrzut + reset */
    if (dt >= IR_GAP_US) {
        if (!frame_ready && seg_cnt >= IR_MIN_SEGS) {
            uint8_t cnt = seg_cnt;
            if (cnt > IR_MAX_SEGS) cnt = IR_MAX_SEGS;
            for (uint8_t i = 0; i < cnt; i++) frame_buf[i] = segs[i];
            frame_cnt = cnt;
            frame_ready = true;
        }
        seg_cnt = 0; // start nowej ramki
        return;
    }

    /* Normalny zapis odcinka */
    if (seg_cnt < IR_MAX_SEGS) segs[seg_cnt++] = dt;
}

bool IR_Capture_TakeFrame(uint32_t* out, uint8_t* count) {
    if (!frame_ready) return false;
    __disable_irq();
    uint8_t cnt = frame_cnt;
    for (uint8_t i = 0; i < cnt; i++) out[i] = frame_buf[i];
    frame_ready = false;
    __enable_irq();
    *count = cnt;
    return true;
}

uint8_t IR_Capture_CurrentCount(void) { return seg_cnt; }
