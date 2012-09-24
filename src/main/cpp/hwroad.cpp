#include "hwroad.hpp"

// Road Layer Emulation.
//
// Writes go to ram. Reads from ramBuff.

HWRoad hwroad;

HWRoad::HWRoad(void)
{
}

HWRoad::~HWRoad(void)
{
}

// Convert road to a more useable format
void HWRoad::init(const uint8_t* src_road)
{
	road_control = 0;
	color_offset1 = 0x400;
	color_offset2 = 0x420;
	color_offset3 = 0x780;
	x_offset = 0;

	decode_road(src_road);
}

void HWRoad::decode_road(const uint8_t* src_road)
{
	for (int y = 0; y < 256 * 2; y++) 
	{
		int src = ((y & 0xff) * 0x40 + (y >> 8) * 0x8000) % rom_size; // tempGfx
		int dst = y * 512; // System16Roads

		// loop over columns
		for (int x = 0; x < 512; x++) 
		{
			roads[dst + x] = (((src_road[src + (x / 8)] >> (~x & 7)) & 1) << 0) | (((src_road[src + (x / 8 + 0x4000)] >> (~x & 7)) & 1) << 1);

			// pre-mark road data in the "stripe" area with a high bit
			if (x >= 256 - 8 && x < 256 && roads[dst + x] == 3)
				roads[dst + x] |= 4;
		}
	}

	// set up a dummy road in the last entry
	for (int i = 0; i < 512; i++) 
	{
		roads[256 * 2 * 512 + i] = 3;
	}
}

void HWRoad::write16(uint32_t adr, const uint16_t data)
{
    ram[(adr >> 1) & 0x7FF] = data;
}

void HWRoad::write16(uint32_t* adr, const uint16_t data)
{
    uint32_t a = *adr;
    ram[(a >> 1) & 0x7FF] = data;
    *adr += 2;
}

void HWRoad::write32(uint32_t* adr, const uint32_t data)
{
    uint32_t a = *adr;
    ram[(a >> 1) & 0x7FF] = data >> 16;
    ram[((a >> 1) + 1) & 0x7FF] = data & 0xFFFF;
    *adr += 4;
}

uint16_t HWRoad::read_road_control()
{
	uint32_t *src = (uint32_t *)ram;
	uint32_t *dst = (uint32_t *)ramBuff;

	// swap the halves of the road RAM
	for (uint16_t i = 0; i < ROAD_RAM_SIZE/4; i++)
	{
		uint32_t temp = *src;
		*src++ = *dst;
		*dst++ = temp;
	}

    return 0xffff;
}

void HWRoad::write_road_control(const uint8_t road_control_)
{
	road_control = road_control_;
}

// Background: Look for solid fill scanlines
void HWRoad::render_background(uint32_t* pixels)
{
    int x, y;
    uint16_t* roadram = ramBuff;

    for (y = 0; y < S16_HEIGHT; y++) 
    {
        int dest = (y * S16_WIDTH);
        int data0 = roadram[0x000 + y];
        int data1 = roadram[0x100 + y];

        int color = -1;

        // based on the info->control, we can figure out which sky to draw
        switch (road_control & 3) 
	    {
		    case 0:
			    if (data0 & 0x800)
				    color = data0 & 0x7f;
			    break;

		    case 1:
			    if (data0 & 0x800)
				    color = data0 & 0x7f;
			    else if (data1 & 0x800)
				    color = data1 & 0x7f;
			    break;

		    case 2:
			    if (data1 & 0x800)
				    color = data1 & 0x7f;
			    else if (data0 & 0x800)
				    color = data0 & 0x7f;
			    break;

		    case 3:
			    if (data1 & 0x800)
				    color = data1 & 0x7f;
			    break;
        }

        // fill the scanline with color
        if (color != -1) 
        {
            for (x = 0; x < S16_WIDTH; x++)
                pixels[dest + x] = color | color_offset3;
        }
    }
}

// Foreground: Render From ROM
void HWRoad::render_foreground(uint32_t* pixels)
{
    int x, y;
    uint16_t* roadram = ramBuff;
	
    for (y = 0; y < S16_HEIGHT; y++) 
    {
	    uint16_t color_table[32];

        static const uint8_t priority_map[2][8] =
	    {
		    { 0x80,0x81,0x81,0x87,0,0,0,0x00 },
		    { 0x81,0x81,0x81,0x8f,0,0,0,0x80 }
	    };

        int32_t dest = (y * S16_WIDTH);
        uint32_t data0 = roadram[0x000 + y];
        uint32_t data1 = roadram[0x100 + y];

        // if both roads are low priority, skip
        if (((data0 & 0x800) != 0) && ((data1 & 0x800) != 0))
            continue;

        int32_t hpos0, hpos1, color0, color1;
        int32_t control = road_control & 3;

        int32_t src0, src1; // 8 bits
        int32_t bgcolor; // 8 bits

        // get road 0 data
        src0   = ((data0 & 0x800) != 0) ? 256 * 2 * 512 : ((0x000 + ((data0 >> 1) & 0xff)) * 512);
        hpos0  = roadram[0x200 + (((road_control & 4) != 0) ? y : (data0 & 0x1ff))] & 0xfff;
        color0 = roadram[0x600 + (((road_control & 4) != 0) ? y : (data0 & 0x1ff))];

        // get road 1 data
        src1   = ((data1 & 0x800) != 0) ? 256 * 2 * 512 : ((0x100 + ((data1 >> 1) & 0xff)) * 512);
        hpos1  = roadram[0x400 + (((road_control & 4) != 0) ? (0x100 + y) : (data1 & 0x1ff))] & 0xfff;
        color1 = roadram[0x600 + (((road_control & 4) != 0) ? (0x100 + y) : (data1 & 0x1ff))];

        // determine the 5 colors for road 0
        color_table[0x00] = color_offset1 ^ 0x00 ^ ((color0 >> 0) & 1);
        color_table[0x01] = color_offset1 ^ 0x02 ^ ((color0 >> 1) & 1);
        color_table[0x02] = color_offset1 ^ 0x04 ^ ((color0 >> 2) & 1);
        bgcolor = (color0 >> 8) & 0xf;
        color_table[0x03] = ((data0 & 0x200) != 0) ? color_table[0x00] : (color_offset2 ^ 0x00 ^ bgcolor);
        color_table[0x07] = color_offset1 ^ 0x06 ^ ((color0 >> 3) & 1);

        // determine the 5 colors for road 1
        color_table[0x10] = color_offset1 ^ 0x08 ^ ((color1 >> 4) & 1);
        color_table[0x11] = color_offset1 ^ 0x0a ^ ((color1 >> 5) & 1);
        color_table[0x12] = color_offset1 ^ 0x0c ^ ((color1 >> 6) & 1);
        bgcolor = (color1 >> 8) & 0xf;
        color_table[0x13] = ((data1 & 0x200) != 0) ? color_table[0x10] : (color_offset2 ^ 0x10 ^ bgcolor);
        color_table[0x17] = color_offset1 ^ 0x0e ^ ((color1 >> 7) & 1);

        // draw the road
        switch (control) 
        {
	        case 0:
		        if (data0 & 0x800)
				        continue;
		        hpos0 = (hpos0 - (0x5f8 + x_offset)) & 0xfff;
		        for (x = 0; x < S16_WIDTH; x++) 
		        {
			        int pix0 = (hpos0 < 0x200) ? roads[src0 + hpos0] : 3;
			        pixels[dest + x] = color_table[0x00 + pix0];
			        hpos0 = (hpos0 + 1) & 0xfff;
		        }
		        break;

	        case 1:
		        hpos0 = (hpos0 - (0x5f8 + x_offset)) & 0xfff;
		        hpos1 = (hpos1 - (0x5f8 + x_offset)) & 0xfff;
		        for (x = 0; x < S16_WIDTH; x++) 
		        {
			        int pix0 = (hpos0 < 0x200) ? roads[src0 + hpos0] : 3;
			        int pix1 = (hpos1 < 0x200) ? roads[src1 + hpos1] : 3;
			        if (((priority_map[0][pix0] >> pix1) & 1) != 0)
				        pixels[dest + x] = color_table[0x10 + pix1];
			        else
				        pixels[dest + x] = color_table[0x00 + pix0];

			        hpos0 = (hpos0 + 1) & 0xfff;
			        hpos1 = (hpos1 + 1) & 0xfff;
		        }
		        break;

	        case 2:
		        hpos0 = (hpos0 - (0x5f8 + x_offset)) & 0xfff;
		        hpos1 = (hpos1 - (0x5f8 + x_offset)) & 0xfff;
		        for (x = 0; x < S16_WIDTH; x++) 
		        {
			        int pix0 = (hpos0 < 0x200) ? roads[src0 + hpos0] : 3;
			        int pix1 = (hpos1 < 0x200) ? roads[src1 + hpos1] : 3;
			        if (((priority_map[1][pix0] >> pix1) & 1) != 0)
				        pixels[dest + x] = color_table[0x10 + pix1];
			        else
				        pixels[dest + x] = color_table[0x00 + pix0];

			        hpos0 = (hpos0 + 1) & 0xfff;
			        hpos1 = (hpos1 + 1) & 0xfff;
		        }
		        break;

	        case 3:
		        if (data1 & 0x800)
				        continue;
		        hpos1 = (hpos1 - (0x5f8 + x_offset)) & 0xfff;
		        for (x = 0; x < S16_WIDTH; x++) 
		        {
			        int pix1 = (hpos1 < 0x200) ? roads[src1 + hpos1] : 3;
			        pixels[dest + x] = color_table[0x10 + pix1];
			        hpos1 = (hpos1 + 1) & 0xfff;
		        }
		        break;
	        } // end switch
    } // end for
}

