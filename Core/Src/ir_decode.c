#include "ir_decode.h"
#include <stddef.h>  /* size_t */

/* ===== Funkcje pomocnicze ===== */
static inline bool in_pct(uint32_t x, uint32_t nom, uint32_t tol) {
    uint32_t d = nom * tol / 100u; return x >= nom - d && x <= nom + d;
}
static uint16_t guess_T_from_min(const uint32_t* segs, uint8_t n, uint16_t lo, uint16_t hi) {
    uint32_t T = 0xFFFFFFFFu;
    for (uint8_t i = 0; i < n; i++) { 
        uint32_t v = segs[i]; 
        if (v >= lo && v <= hi && v < T) T = v; 
    }
    return T == 0xFFFFFFFFu ? 0u : (uint16_t)T;
}

/* ===== Podstawowe dekodery ===== */
static bool decode_pd(const IRSpec* sp, const uint32_t* b, uint8_t n, IRDecodeOut* out) {
    uint8_t i = 0;
    /* powtórzenia (np. NEC) */
    if (sp->rpt_mark_us && sp->rpt_space_us && n >= 2) {
        if (in_pct(b[0], sp->rpt_mark_us, sp->tol_hdr_pct) &&
            in_pct(b[1], sp->rpt_space_us, sp->tol_hdr_pct)) {
            *out = (IRDecodeOut){ sp->id, sp->name, true, 0, 0 }; 
            return true;
        }
    }
    /* nag³ówek */
    if (sp->hdr_mark_us) {
        if (n < 2) return false;
        if (!(in_pct(b[0], sp->hdr_mark_us, sp->tol_hdr_pct) &&
            in_pct(b[1], sp->hdr_space_us, sp->tol_hdr_pct))) return false;
        i = 2;
    }
    /* podstawa czasu */
    uint16_t T = sp->T_us ? sp->T_us : guess_T_from_min(b + i, n - i, 300, 1200);
    if (!T) T = sp->mark_us ? sp->mark_us : 560;

    uint64_t v = 0; uint8_t bits = 0;
    while (i + 1 < n && bits < sp->bits_max) {
        uint32_t mk = b[i++], spc = b[i++];
        if (!in_pct(mk, sp->mark_us ? sp->mark_us : T, sp->tol_bit_pct)) break;
        uint8_t bit;
        if (in_pct(spc, sp->space0_us ? sp->space0_us : T, sp->tol_bit_pct)) bit = 0;
        else if (in_pct(spc, sp->space1_us ? sp->space1_us : 3 * T, sp->tol_bit_pct)) bit = 1;
        else break;
        if (sp->lsb_first) v |= ((uint64_t)bit << bits); 
        else v = (v << 1) | bit;
        bits++;
    }
    if (bits < sp->bits_min) return false;
    *out = (IRDecodeOut){ sp->id,sp->name,false,bits,v }; return true;
}

static bool decode_pw(const IRSpec* sp, const uint32_t* b, uint8_t n, IRDecodeOut* out) {
    uint8_t i = 0;
    if (sp->hdr_mark_us) {
        if (n < 2) return false;
        if (!(in_pct(b[0], sp->hdr_mark_us, sp->tol_hdr_pct) &&
            in_pct(b[1], sp->hdr_space_us, sp->tol_hdr_pct))) return false;
        i = 2;
    }
    uint16_t T = sp->T_us ? sp->T_us : guess_T_from_min(b + i, n - i, 300, 1200);
    if (!T) T = 600;

    uint64_t v = 0; uint8_t bits = 0;
    while (i + 1 < n && bits < sp->bits_max) {
        uint32_t mk = b[i++], spc = b[i++];
        if (!in_pct(spc, sp->space_us ? sp->space_us : T, sp->tol_bit_pct)) break;
        uint8_t bit;
        if (in_pct(mk, sp->mark0_us ? sp->mark0_us : T, sp->tol_bit_pct)) bit = 0;
        else if (in_pct(mk, sp->mark1_us ? sp->mark1_us : 2 * T, sp->tol_bit_pct)) bit = 1;
        else break;
        if (sp->lsb_first) v |= ((uint64_t)bit << bits); 
        else v = (v << 1) | bit;
        bits++;
    }
    if (bits < sp->bits_min) return false;
    *out = (IRDecodeOut){ sp->id,sp->name,false,bits,v }; return true;
}

/* ===== Manchester (RC5/RC6) ===== */

static bool rc5_try(const IRSpec* sp, const uint32_t* b, uint8_t n, IRDecodeOut* out) {
    if (n < 6) return false; 
    const uint16_t halfT = sp->T_us; 
    if (!halfT) return false;
    enum { MAXH = 256 }; 
    uint8_t halves[MAXH];
    uint16_t H = 0; 
    uint8_t lvl = 1;
    for (uint8_t i = 0; i < n && H < MAXH; i++) {
        uint16_t k = (uint16_t)((b[i] + halfT / 2) / halfT); 
        if (!k) k = 1; if (k > 6) k = 6;
        while (k-- && H < MAXH) { 
            halves[H++] = lvl; 
            lvl ^= 1u; 
        }
    }
    if (H < 2 * sp->bits_min) return false;

    for (uint8_t inv = 0; inv < 2; inv++) {
        uint64_t v = 0; uint8_t bits = 0; uint16_t pos = 0;
        while ((pos + 1) < H && bits < sp->bits_max) {
            uint8_t first = halves[pos] ^ inv;
            v = (v << 1) | first; bits++; pos += 2;
        }
        if (bits < sp->bits_min) continue;
        uint8_t s1 = (v >> (bits - 1)) & 1u, s2 = (v >> (bits - 2)) & 1u; 
        if (s1 != 1u || s2 != 1u) continue;
        *out = (IRDecodeOut){ sp->id,sp->name,false,bits,v }; 
        return true;
    }
    return false;
}

static bool rc6_try(const IRSpec* sp, const uint32_t* b, uint8_t n, IRDecodeOut* out) {
    if (n < 4) return false; 
    const uint16_t T = sp->T_us; 
    if (!T) return false;
    if (!(in_pct(b[0], 6 * T, sp->tol_hdr_pct) && in_pct(b[1], 2 * T, sp->tol_hdr_pct))) return false;

    enum { MAXH = 512 }; 
    uint8_t halves[MAXH]; 
    uint16_t H = 0; 
    uint8_t lvl = 1;
    for (uint8_t i = 2; i < n && H < MAXH; i++) {
        uint16_t k = (uint16_t)((b[i] + T / 2) / T); 
        if (!k) k = 1; 
        if (k > 8) k = 8;
        while (k-- && H < MAXH) { 
            halves[H++] = lvl;
            lvl ^= 1u;
        }
    }
    if (H < 2 * sp->bits_min) return false;

    const uint8_t toggle_index = 4; /* start(1), mode(3), toggle(na 5. pozycji) */

    for (uint8_t inv = 0; inv < 2; inv++) {
        uint64_t v = 0; 
        uint8_t bits = 0; 
        uint16_t pos = 0;
        while (pos < H && bits < sp->bits_max) {
            if (bits == toggle_index) {
                if (pos + 3 >= H) break;
                uint8_t first = halves[pos] ^ inv; 
                v = (v << 1) | first; 
                bits++; 
                pos += 4;
            }
            else {
                if (pos + 1 >= H) break;
                uint8_t first = halves[pos] ^ inv; 
                v = (v << 1) | first; 
                bits++; 
                pos += 2;
            }
        }
        if (bits < sp->bits_min) continue;
        uint8_t start = (v >> (bits - 1)) & 1u; 
        if (start != 1u) continue;
        *out = (IRDecodeOut){ sp->id,sp->name,false,bits,v }; 
        
        return true;
    }
    return false;
}

static bool decode_manchester(const IRSpec* sp, const uint32_t* b, uint8_t n, IRDecodeOut* out) {
    if (sp->id == IR_PROTO_RC5) return rc5_try(sp, b, n, out);
    if (sp->id == IR_PROTO_RC6) return rc6_try(sp, b, n, out);
    return false;
}

/* ===== Tabela specyfikacji ===== */
static const IRSpec P[] = {
  { "NEC", IR_PROTO_NEC, ENC_PULSE_DISTANCE, 1, 24, 32, 9000, 4500, 9000, 2250, 560, 20, 30,
    560, 560, 1690, 0, 0, 0, 0, 0, 0 },
  { "JVC", IR_PROTO_JVC, ENC_PULSE_DISTANCE, 1, 16, 32, 8400, 4200, 0, 0, 525, 25, 35,
    525, 1050, 2100, 0, 0, 0, 0, 0, 0 },
  { "Panasonic Kaseikyo", IR_PROTO_PANASONIC_KASEIKYO, ENC_PULSE_DISTANCE, 1, 48, 56, 3500, 1750, 0, 0, 435, 25, 35,
    435, 435, 1305, 0, 0, 0, 0, 0, 0 },
  { "Sony SIRC", IR_PROTO_SONY_SIRC, ENC_PULSE_WIDTH, 1, 12, 20, 2400, 600, 0, 0, 600, 25, 35,
    0, 0, 0, 600, 600, 1200, 0, 0, 0 },
  { "Sharp", IR_PROTO_SHARP, ENC_PULSE_DISTANCE, 1, 12, 20, 0, 0, 0, 0, 320, 25, 35,
    320, 1000, 2000, 0, 0, 0, 0, 0, 0 },
  { "RC6", IR_PROTO_RC6, ENC_MANCHESTER, 0, 20, 32, 0, 0, 0, 0, 444, 25, 30,
    0, 0, 0, 0, 0, 0, 2 * 444, 6 * 444, 2 * 444 },
  { "RC5", IR_PROTO_RC5, ENC_MANCHESTER, 0, 14, 14, 0, 0, 0, 0, 889, 25, 30,
    0, 0, 0, 0, 0, 0, 2 * 889, 0, 0 }
};
static const size_t PN = sizeof(P) / sizeof(P[0]);

IRDecodeOut ir_detect_and_decode(const uint32_t* segs, uint8_t n) {
    IRDecodeOut out = (IRDecodeOut){ IR_PROTO_UNKNOWN, "UNKNOWN", false, 0, 0 };
    if (n < 2) return out;

    /* Najpierw szybkie sprawdzenie powtórek (NEC) */
    for (size_t i = 0; i < PN; i++) {
        const IRSpec* sp = &P[i];
        if (sp->enc == ENC_PULSE_DISTANCE && sp->rpt_mark_us && sp->rpt_space_us && n >= 2) {
            if (in_pct(segs[0], sp->rpt_mark_us, sp->tol_hdr_pct) &&
                in_pct(segs[1], sp->rpt_space_us, sp->tol_hdr_pct)) {
                out.proto = sp->id; 
                out.name = sp->name;
                out.is_repeat = true; 
                return out;
            }
        }
    }
    /* Normalna detekcja + dekodowanie */
    for (size_t i = 0; i < PN; i++) {
        const IRSpec* sp = &P[i]; bool ok = false;
        if (sp->enc == ENC_PULSE_DISTANCE) ok = decode_pd(sp, segs, n, &out);
        else if (sp->enc == ENC_PULSE_WIDTH) ok = decode_pw(sp, segs, n, &out);
        else ok = decode_manchester(sp, segs, n, &out);
        if (ok) return out;
    }
    return out;
}

const char* ir_proto_name(ir_proto_t p) {
    switch (p) {
    case IR_PROTO_NEC: return "NEC";
    case IR_PROTO_JVC: return "JVC";
    case IR_PROTO_PANASONIC_KASEIKYO: return "Panasonic Kaseikyo";
    case IR_PROTO_SONY_SIRC: return "Sony SIRC";
    case IR_PROTO_RC5: return "RC5";
    case IR_PROTO_RC6: return "RC6";
    case IR_PROTO_SHARP: return "Sharp";
    default: return "UNKNOWN";
    }
}
