#pragma once

#include "stdint.hpp"

class HWRoad
{
public:
    HWRoad();
    ~HWRoad();

    void init(const uint8_t*, const bool hires);
    void write16(uint32_t adr, const uint16_t data);
    void write16(uint32_t* adr, const uint16_t data);
    void write32(uint32_t* adr, const uint32_t data);
    uint16_t read_road_control();
    void write_road_control(const uint8_t);
    void (HWRoad::*render_background)(uint16_t*);
    void (HWRoad::*render_foreground)(uint16_t*);
  
private:
    uint8_t road_control;
    uint16_t color_offset1;
    uint16_t color_offset2;
    uint16_t color_offset3;
    int32_t x_offset;

    static const uint16_t ROAD_RAM_SIZE = 0x1000;
    static const uint16_t rom_size = 0x8000;

    // Decoded road graphics
    uint8_t roads[0x40200];

    // Two halves of RAM
    uint16_t ram[ROAD_RAM_SIZE / 2];
    uint16_t ramBuff[ROAD_RAM_SIZE / 2];
    uint16_t ramScratch[ROAD_RAM_SIZE / 2]; // JJP optimisation

    void decode_road(const uint8_t*);
//    void render_background_lores(uint16_t*);
//    void render_foreground_lores(uint16_t*);
//    void render_background_hires(uint16_t*);
//    void render_foreground_hires(uint16_t*);

    // JJP
    void render_background_lores(uint16_t*);
    void render_foreground_lores(uint16_t*);
    void render_foreground_lores_thread(uint32_t threadID, uint32_t totalThreads, uint16_t* pixels);
    void render_background_hires(uint16_t*);
    void render_foreground_hires(uint16_t*);
    void render_foreground_hires_thread(uint32_t threadID, uint32_t totalThreads, uint16_t* pixels);

};

extern HWRoad hwroad;
