/***************************************************************************
    Hardware Sprites.
    
    This class stores sprites in the converted format expected by the
    OutRun graphics hardware.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "engine/osprite.hpp"

//  Out Run/X-Board-style sprites
//
//      Offs  Bits               Usage
//       +0   e------- --------  Signify end of sprite list
//       +0   -h-h---- --------  Hide this sprite if either bit is set
//       +0   ----bbb- --------  Sprite bank
//       +0   -------t tttttttt  Top scanline of sprite + 256
//       +2   oooooooo oooooooo  Offset within selected sprite bank
//       +4   ppppppp- --------  Signed 7-bit pitch value between scanlines
//       +4   -------x xxxxxxxx  X position of sprite (position $BE is screen position 0)
//       +6   -s------ --------  Enable shadows
//       +6   --pp---- --------  Sprite priority, relative to tilemaps
//       +6   ------vv vvvvvvvv  Vertical zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
//       +8   y------- --------  Render from top-to-bottom (1) or bottom-to-top (0) on screen
//       +8   -f------ --------  Horizontal flip: read the data backwards if set
//       +8   --x----- --------  Render from left-to-right (1) or right-to-left (0) on screen
//       +8   ------hh hhhhhhhh  Horizontal zoom factor (0x200 = full size, 0x100 = half size, 0x300 = 2x size)
//       +E   dddddddd dddddddd  Scratch space for current address
//
//    Out Run only:
//       +A   hhhhhhhh --------  Height in scanlines - 1
//       +A   -------- -ccccccc  Sprite color palette

osprite::osprite(void)
{
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    data[4] = 0;
    data[5] = 0;

    scratch = 0;
}

osprite::~osprite(void)
{
}

uint16_t osprite::get_x(void)
{
    return data[0x2]; // returning x uses whole value
}

uint16_t osprite::get_y(void)
{
    return data[0x0]; // returning y uses whole value
}

void osprite::set_x(uint16_t x)
{
    data[0x2] = x; // setting x wipes entire value
}

void osprite::set_pitch(uint8_t p)
{
    data[0x02] = (data[0x2] & 0x1FF) | ((p & 0xFE) << 8);
}

void osprite::inc_x(uint16_t v)
{
    data[0x2] = (data[0x2] & 0xFE00) | (((data[0x2] & 0x1FF) + v) & 0x1FF);
}

void osprite::set_y(uint16_t y)
{
    data[0x0] = y; // setting y wipes entire value
}

void osprite::set_vzoom(uint16_t z)
{
    data[0x03] = z;
}

void osprite::set_hzoom(uint16_t z)
{
    data[0x4] = z;
}

void osprite::set_priority(uint8_t p)
{
    data[0x03] |= (p << 8);
}

void osprite::set_offset(uint16_t o)
{
    data[0x1] = o;
}

void osprite::inc_offset(uint16_t o)
{
    data[0x1] += o;
}

void osprite::set_render(uint8_t bits)
{
    data[0x4] |= ((bits & 0xE0) << 8);
}

void osprite::set_pal(uint8_t pal)
{
    data[0x5] = (data[0x5] & 0xFF00) + pal;
}

void osprite::set_height(uint8_t h)
{
    data[0x5] = (data[0x5] & 0xFF) + (h << 8);
}

void osprite::sub_height(uint8_t h)
{
    uint8_t height = ((data[0x05] >> 8) - h) & 0xFF;
    data[0x5] = (data[0x5] & 0xFF) + (height << 8);
}

void osprite::set_bank(uint8_t bank)
{
    data[0x0] |= (bank << 8);
}

void osprite::hide(void)
{
    data[0x0] |= 0x4000;
    data[0x0] &= ~0x8000; // denote sprite list not ended
}