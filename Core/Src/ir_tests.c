#include "ir_config.h"
#ifdef IR_ENABLE_SELF_TESTS

#include "ir_decode.h"
#include "main.h"
#include <stdio.h>

/* LED status (NUCLEO-L432KC: PB3) */
#ifndef LD3_GPIO_Port
#define LD3_GPIO_Port GPIOB
#endif
#ifndef LD3_Pin
#define LD3_Pin GPIO_PIN_3
#endif

static int g_pass = 0, g_fail = 0;
#define ASSERT_TRUE(msg, cond) do{ if(cond){g_pass++; printf("[PASS] %s\r\n", msg);} else {g_fail++; printf("[FAIL] %s\r\n", msg);} }while(0)
#define ASSERT_EQU(msg,a,b) do{ unsigned _A=(unsigned)(a), _B=(unsigned)(b); if(_A==_B){g_pass++; printf("[PASS] %s\r\n", msg);} else {g_fail++; printf("[FAIL] %s exp=%u got=%u\r\n", msg,_B,_A);} }while(0)
#define ASSERT_EQ64(msg,a,b) do{ unsigned long long _A=(unsigned long long)(a), _B=(unsigned long long)(b); if(_A==_B){g_pass++; printf("[PASS] %s\r\n", msg);} else {g_fail++; printf("[FAIL] %s exp=0x%llX got=0x%llX\r\n", msg,_B,_A);} }while(0)

static inline void push(uint32_t* out, uint8_t* n, uint32_t v) { out[(*n)++] = v; }

/* Generatory syntetycznych ramek */
static uint8_t build_pd(uint32_t* out, uint16_t hm, uint16_t hs, uint16_t mk, uint16_t sp0, uint16_t sp1, uint64_t val, uint8_t bits, uint8_t lsb) {
    uint8_t n = 0; 
    if (hm && hs) { 
        push(out, &n, hm); 
        push(out, &n, hs); 
    }
    for (uint8_t i = 0; i < bits; i++) {
        uint8_t bit = lsb ? ((val >> i) & 1u) : ((val >> (bits - 1 - i)) & 1u);
        push(out, &n, mk); 
        push(out, &n, bit ? sp1 : sp0);
    }
    return n;
}
static uint8_t build_pw(uint32_t* out, uint16_t hm, uint16_t hs, uint16_t spc, uint16_t mk0, uint16_t mk1, uint64_t val, uint8_t bits, uint8_t lsb) {
    uint8_t n = 0; if (hm && hs) { push(out, &n, hm); push(out, &n, hs); }
    for (uint8_t i = 0; i < bits; i++) {
        uint8_t bit = lsb ? ((val >> i) & 1u) : ((val >> (bits - 1 - i)) & 1u);
        push(out, &n, bit ? mk1 : mk0); 
        push(out, &n, spc);
    }
    return n;
}
static uint8_t build_rc6(uint32_t* out, uint16_t T, uint32_t bits_total) {
    uint8_t n = 0; 
    push(out, &n, 6 * T); 
    push(out, &n, 2 * T);
    uint32_t halves = 0; 
    for (uint32_t b = 0; b < bits_total; b++) halves += (b == 4) ? 4 : 2;
    for (uint32_t h = 0; h < halves; h++) push(out, &n, T); 
    return n;
}
static uint8_t build_rc5(uint32_t* out, uint16_t T, uint32_t bits_total) {
    uint8_t n = 0; for (uint32_t h = 0; h < 2u * bits_total; h++) push(out, &n, T); return n;
}

/* Test cases */
static void t_nec32(void) {
    uint32_t segs[256]; uint8_t n;
    uint8_t addr = 0x10, cmd = 0x34;
    uint32_t v = (uint32_t)addr | ((uint32_t)(uint8_t)~addr << 8) | ((uint32_t)cmd << 16) | ((uint32_t)(uint8_t)~cmd << 24);
    n = build_pd(segs, 9000, 4500, 560, 560, 1690, v, 32, 1);
    IRDecodeOut r = ir_detect_and_decode(segs, n);
    ASSERT_EQU("NEC32 proto", r.proto, IR_PROTO_NEC);
    ASSERT_TRUE("NEC32 no repeat", !r.is_repeat);
    ASSERT_EQU("NEC32 bits", r.bits, 32);
    ASSERT_EQ64("NEC32 value", r.value, v);
}
static void t_nec_repeat(void) {
    uint32_t segs[4] = { 9000,2250,560,560 };
    IRDecodeOut r = ir_detect_and_decode(segs, 2);
    ASSERT_EQU("NEC repeat proto", r.proto, IR_PROTO_NEC);
    ASSERT_TRUE("NEC repeat flag", r.is_repeat);
}
static void t_nec24(void) {
    uint32_t segs[256]; uint8_t n;
    uint8_t addr = 0xA1, cmd = 0x5A;
    uint32_t v24 = (uint32_t)addr | ((uint32_t)cmd << 8) | ((uint32_t)(uint8_t)~cmd << 16);
    n = build_pd(segs, 9000, 4500, 560, 560, 1690, v24, 24, 1);
    IRDecodeOut r = ir_detect_and_decode(segs, n);
    ASSERT_EQU("NEC24 proto", r.proto, IR_PROTO_NEC);
    ASSERT_EQU("NEC24 bits", r.bits, 24);
    ASSERT_EQ64("NEC24 value", r.value, v24);
}
static void t_jvc16(void) {
    uint32_t segs[256]; uint8_t n;
    uint16_t v16 = 0xBEEF;
    n = build_pd(segs, 8400, 4200, 525, 1050, 2100, v16, 16, 1);
    IRDecodeOut r = ir_detect_and_decode(segs, n);
    ASSERT_EQU("JVC16 proto", r.proto, IR_PROTO_JVC);
    ASSERT_EQU("JVC16 bits", r.bits, 16);
}
static void t_sony12(void) {
    uint32_t segs[256]; uint8_t n;
    uint16_t v12 = 0x5A5;
    n = build_pw(segs, 2400, 600, 600, 600, 1200, v12, 12, 1);
    IRDecodeOut r = ir_detect_and_decode(segs, n);
    ASSERT_EQU("SIRC12 proto", r.proto, IR_PROTO_SONY_SIRC);
    ASSERT_EQU("SIRC12 bits", r.bits, 12);
}
static void t_kaseikyo48(void) {
    uint32_t segs[512]; uint8_t n;
    uint64_t v48 = 0x2002ull | (0x123456ull << 16);
    n = build_pd(segs, 3500, 1750, 435, 435, 1305, v48, 48, 1);
    IRDecodeOut r = ir_detect_and_decode(segs, n);
    ASSERT_EQU("Kaseikyo proto", r.proto, IR_PROTO_PANASONIC_KASEIKYO);
    ASSERT_TRUE("Kaseikyo bits ok", r.bits >= 48);
}
static void t_sharp15(void) {
    uint32_t segs[256]; uint8_t n; uint32_t v15 = 0x5A3D;
    n = build_pd(segs, 0, 0, 320, 1000, 2000, v15, 15, 1);
    IRDecodeOut r = ir_detect_and_decode(segs, n);
    ASSERT_EQU("Sharp proto", r.proto, IR_PROTO_SHARP);
}
static void t_rc6_20(void) {
    uint32_t segs[1024]; uint8_t n; n = build_rc6(segs, 444, 20);
    IRDecodeOut r = ir_detect_and_decode(segs, n);
    ASSERT_EQU("RC6 proto", r.proto, IR_PROTO_RC6);
    ASSERT_TRUE("RC6 bits ok", r.bits >= 20);
}
static void t_rc5_14(void) {
    uint32_t segs[256]; uint8_t n; n = build_rc5(segs, 889, 14);
    IRDecodeOut r = ir_detect_and_decode(segs, n);
    ASSERT_EQU("RC5 proto", r.proto, IR_PROTO_RC5);
    ASSERT_TRUE("RC5 bits ok", r.bits >= 14);
}

void IR_Tests_RunUART(void)
{
    printf("\r\nIR decode on-target self-tests start\r\n");
    g_pass = g_fail = 0;

    t_nec32(); t_nec_repeat(); t_nec24();
    t_jvc16(); t_sony12(); t_kaseikyo48();
    t_sharp15(); t_rc6_20(); t_rc5_14();

    printf("Summary: PASS=%d FAIL=%d\r\n", g_pass, g_fail);

    for (;;) {
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
        HAL_Delay(g_fail ? 250 : 1000);  // szybciej miga gdy FAIL
    }
}
#endif /* IR_ENABLE_SELF_TESTS */
