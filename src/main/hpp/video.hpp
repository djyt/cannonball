#pragma once

// For reporting errors :)
#include <iostream>

#include <SDL.h>
#include "stdint.hpp"
#include "romloader.hpp"
#include "hwtiles.hpp"
#include "hwsprites.hpp"
#include "hwroad.hpp"

class hwsprites;

class Video
{
public:
	static const uint16_t S16_PALETTE_ENTRIES = 0x1000;

	hwsprites* sprite_layer;
    hwtiles* tile_layer;
	uint32_t *pixels;

	Video();
	void draw_frame();
	int init(uint8_t*, uint8_t*, uint8_t*);
	~Video();

    void clear_text_ram();
    void write_text8(uint32_t, const uint8_t);
	void write_text16(uint32_t*, const uint16_t);
	void write_text16(uint32_t, const uint16_t);
    void write_text32(uint32_t*, const uint32_t);
    void write_text32(uint32_t, const uint32_t);
    uint8_t read_text8(uint32_t);

    void clear_tile_ram();    
    void write_tile8(uint32_t, const uint8_t);
	void write_tile16(uint32_t*, const uint16_t);
	void write_tile16(uint32_t, const uint16_t);
    void write_tile32(uint32_t*, const uint32_t);
    void write_tile32(uint32_t, const uint32_t);
    uint8_t read_tile8(uint32_t);

	void write_sprite16(uint32_t*, const uint16_t);

	void write_pal8(uint32_t*, const uint8_t);
	void write_pal16(uint32_t*, const uint16_t);
	void write_pal32(uint32_t*, const uint32_t);
	void write_pal32(uint32_t, const uint32_t);
	uint8_t read_pal8(uint32_t);
	uint16_t read_pal16(uint32_t*);
	uint16_t read_pal16(uint32_t);
    uint32_t read_pal32(uint32_t*);

private:
	SDL_Surface *surface;

	uint8_t palette[Video::S16_PALETTE_ENTRIES * 2]; // 2 Bytes Per Palette Entry
	uint32_t rgb[Video::S16_PALETTE_ENTRIES * 3]; // Extended to hold shadow/hilight colours

	void refresh_palette(uint32_t);
};

extern Video video;

