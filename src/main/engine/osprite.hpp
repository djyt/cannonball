/***************************************************************************
    Hardware Sprites.
    
    This class stores sprites in the converted format expected by the
    OutRun graphics hardware.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "stdint.hpp" 

class osprite
{
public:
	uint16_t data[0x7];
	uint32_t scratch;

	osprite(void);
	~osprite(void);

	uint16_t get_x();
	uint16_t get_y();
	void set_x(uint16_t);
	void inc_x(uint16_t);
	void set_y(uint16_t);
	void set_pitch(uint8_t);
	void set_vzoom(uint16_t);
	void set_hzoom(uint16_t);
	void set_priority(uint8_t);
	void set_offset(uint16_t o);
	void inc_offset(uint16_t o);
	void set_render(uint8_t b);
	void set_pal(uint8_t);
	void set_height(uint8_t);
	void sub_height(uint8_t);
	void set_bank(uint8_t);
	void hide();

private:

};