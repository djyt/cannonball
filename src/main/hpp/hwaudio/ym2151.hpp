#pragma once

#include "stdint.hpp"
#include "romloader.hpp"
#include "hwaudio/soundchip.hpp"

/* struct describing a single operator */
typedef struct
{
    uint32_t phase;             /* accumulated operator phase */
    uint32_t freq;              /* operator frequency count */
    int32_t  dt1;               /* current DT1 (detune 1 phase inc/decrement) value */
    uint32_t mul;               /* frequency count multiply */
    uint32_t dt1_i;             /* DT1 index * 32 */
    uint32_t dt2;               /* current DT2 (detune 2) value */

    signed int *connect;        /* operator output 'direction' */

    /* only M1 (operator 0) is filled with this data: */
    signed int *mem_connect;    /* where to put the delayed sample (MEM) */
    int32_t     mem_value;      /* delayed sample (MEM) value */

    /* channel specific data; note: each operator number 0 contains channel specific data */
    uint32_t fb_shift;          /* feedback shift value for operators 0 in each channel */
    int32_t  fb_out_curr;       /* operator feedback value (used only by operators 0) */
    int32_t  fb_out_prev;       /* previous feedback value (used only by operators 0) */
    uint32_t kc;                /* channel KC (copied to all operators) */
    uint32_t kc_i;              /* just for speedup */
    uint32_t pms;               /* channel PMS */
    uint32_t ams;               /* channel AMS */
    /* end of channel specific data */

    uint32_t AMmask;            /* LFO Amplitude Modulation enable mask */
    uint32_t state;             /* Envelope state: 4-attack(AR) 3-decay(D1R) 2-sustain(D2R) 1-release(RR) 0-off */
    uint8_t  eg_sh_ar;          /*  (attack state) */
    uint8_t  eg_sel_ar;         /*  (attack state) */
    uint32_t tl;                /* Total attenuation Level */
    int32_t  volume;            /* current envelope attenuation level */
    uint8_t  eg_sh_d1r;         /*  (decay state) */
    uint8_t  eg_sel_d1r;        /*  (decay state) */
    uint32_t d1l;               /* envelope switches to sustain state after reaching this level */
    uint8_t  eg_sh_d2r;         /*  (sustain state) */
    uint8_t  eg_sel_d2r;        /*  (sustain state) */
    uint8_t  eg_sh_rr;          /*  (release state) */
    uint8_t  eg_sel_rr;         /*  (release state) */

    uint32_t key;               /* 0=last key was KEY OFF, 1=last key was KEY ON */

    uint32_t ks;                /* key scale    */
    uint32_t ar;                /* attack rate  */
    uint32_t d1r;               /* decay rate   */
    uint32_t d2r;               /* sustain rate */
    uint32_t rr;                /* release rate */

    uint32_t reserved0;         /**/
    uint32_t reserved1;         /**/

} YM2151Operator;

class YM2151 : public SoundChip
{
public:
    // JEMU executes z80 for 200 cycles when set to true
    bool irq;

    YM2151(float volume, uint32_t clock);
    ~YM2151();
    void init(int rate, int fps);
    void stream_update();
    void write_reg(int r, int v);
    int read_status();

private:
    int clock;        /*chip clock in Hz (passed from 2151intf.c)*/
    int sampfreq;     /*sampling frequency in Hz (passed from 2151intf.c)*/
    float volume;

    void init_tables();
    void init_chip_tables();
    inline void envelope_KONKOFF(YM2151Operator * op, int v);
    inline void set_connect(YM2151Operator *om1, int cha, int v);
    inline void refresh_EG(YM2151Operator * op);
    void ym2151_reset_chip();
    inline signed int op_calc(YM2151Operator * OP, unsigned int env, signed int pm);
    inline signed int op_calc1(YM2151Operator * OP, unsigned int env, signed int pm);
    inline void chan_calc(unsigned int chan);
    inline void chan7_calc();
    inline void advance_eg();
    inline void advance();
};