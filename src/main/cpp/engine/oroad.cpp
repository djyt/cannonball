#include "engine/oroad.hpp"

// - Run this after CPU 0
// - CPU 0 should be initalized before exection
// - Tick 1: init_road_code only
// - Tick 2: main loop

ORoad oroad;

ORoad::ORoad(void)
{

}

ORoad::~ORoad(void)
{
}

void ORoad::tick()
{
    do_road();
}

// Helper function
int16_t ORoad::get_road_y(uint16_t index)
{
    return -(road_y[road_p0 + index] >> 4) + 223;
}

// Initialize Road Values
// 
// Source Address: 0x10B8
// Input:          None
// Output:         None

void ORoad::init()
{
    // Extra initalization code here
    road_pos = 0;
    tilemap_h_target = 0;
    stage_lookup_off = 0;
    road_width_bak = 0;
    car_x_bak = 0;
    height_lookup = 0;
    road_pos_change = 0;
    road_load_bonus = 0;
    road_ctrl = ROAD_OFF;
    road_load_split = 0;
    
    horizon_y2 = 0;
    horizon_y_bak = 0;
    pos_fine = 0;
    horizon_base = 0;

    for (int i = 0; i < ARRAY_LENGTH; i++)
    {
        road_x[i] = 0;
        road0_h[i] = 0;
        road1_h[i] = 0;
        road_unk[i] = 0;
    }

     for (int i = 0; i < 0x1000; i++)
         road_y[i] = 0;

    road_pos_old = 0;
    road_data_offset = 0;
    curve_x1_diff = 0;
    curve_x2_diff = 0;
    curve_x1_dist = 0;
    curve_x2_dist = 0;
    curve_x1_next = 0;
    curve_x2_next = 0;
    curve_inc_old = 0;
    curve_start = 0;
    curve_inc = 0;
    curve_end = 0;
    height_start = 0;
    height_ctrl = 0;
    pos_fine_old = 0;
    pos_fine_diff = 0;
    counter = 0;
    height_index = 0;
    height_final = 0;
    height_inc = 0;
    height_step = 0;
    height_ctrl2 = 0;
    height_addr = 0;
    elevation = 0;
    height_lookup_wrk = 0;
    updowncombo = 0;
    dist_ctrl = 0;
    do_height_inc = 0;
    height_end = 0;
    up_mult = 0;
    down_mult = 0;
    horizon_mod = 0;
    length_offset = 0;
    a1_lookup = 0;
    change_per_entry = 0;
    d5_o = 0;
    a3_o = 0;
    y_addr = 0;
    scanline = 0;
    total_height = 0;
    // End of extra initialization code 

    stage_loaded = -1;
    horizon_set  = 0;

    road_p0 = 0;
    road_p1 = road_p0 + 0x400;
    road_p2 = road_p1 + 0x400;
    road_p3 = road_p2 + 0x400;

    set_default_hscroll();
    clear_road_ram();
    init_stage1();
}

// Main Loop
// 
// Source Address: 0x1044
// Input:          None
// Output:         None

void ORoad::do_road()
{
    rotate_values();
    setup_road_x();
    setup_road_y();
    set_road_y();
    set_horizon_y();
    do_road_data();
    blit_roads();
    output_hscroll(&road0_h[0], HW_HSCROLL_TABLE0);
    output_hscroll(&road1_h[0], HW_HSCROLL_TABLE1);
    copy_bg_color();

    hwroad.read_road_control(); // swap halves of road ram
}

// Set Default Horizontal Scroll Values
// 
// Source Address: 0x1106
// Input:          None
// Output:         None

void ORoad::set_default_hscroll()
{
    uint32_t adr = HW_HSCROLL_TABLE0;

    for (uint16_t i = 0; i <= 0x1FF; i++)
        hwroad.write32(&adr, 0x1000100);

    hwroad.read_road_control();
}

// Clear Road RAM
// 
// Source Address: 0x115A
// Input:          None
// Output:         None

// Initialises 0x80000 - 0x8001C0 as follows.
//
// 0x80000 : 00 00 [scanline 0]
// 0x80002 : 00 01 [scanline 1]
// 0x80004 : 00 02 [scanline 2]

void ORoad::clear_road_ram()
{
    uint32_t adr = 0x80000; // start of road ram

    for (uint8_t scanline = 0; scanline < S16_HEIGHT; scanline++)
        hwroad.write16(&adr, scanline);

    hwroad.read_road_control();
}

// Initalize Stage 1
// 
// Source Address: 0x10DE
// Input:          None
// Output:         None

void ORoad::init_stage1()
{
    // Sets to 0x3c [Stage 1] - First Entry Of Stage Master Table In RAM
    // Multiply by 4, as table contains longs
    uint16_t stage1 = oinitengine.stage_data[0] << 2; 
    stage_addr = roms.rom1.read32(ROAD_DATA_LOOKUP + stage1);
    
    road_pos = 0;
    road_ctrl = ROAD_BOTH_P0;
    // ignore init counter stuff (move.b  #$A,$49(a5))
}

// Rotate Values & Load Next Level/Split/Bonus Road
// 
// Source Address: 0x119E
// Input:          None
// Output:         None

void ORoad::rotate_values()
{
    uint16_t p0_copy = road_p0;
    road_p0 = road_p1;
    road_p1 = road_p2;
    road_p2 = road_p3;
    road_p3 = p0_copy;

    road_pos_change = (road_pos >> 16) - road_pos_old;
    road_pos_old = road_pos >> 16;

    check_load_road();
}

// Check whether we need to load the next stage / road split / bonus data
// 
// Source Address: 0x11D0
// Input:          None
// Output:         None

void ORoad::check_load_road()
{
    // If stage is not loaded
    if (ostats.cur_stage != stage_loaded)
    {
        stage_loaded = ostats.cur_stage; // Denote loaded

        // Table contains longs, so multiply by 4
        uint16_t stage_index = oinitengine.stage_data[stage_lookup_off] << 2;
        stage_addr = roms.rom1.read32(ROAD_DATA_LOOKUP + stage_index);
        return;
    }

    // Check Split
    if (road_load_split)
    {
        stage_addr = ROAD_DATA_SPLIT;
        road_load_split = 0;
    }

    // Check Bonus
    else if (road_load_bonus)
    {
        stage_addr = ROAD_DATA_BONUS;
        road_load_bonus = 0;
    }
}

// 1/ Setup Road X Data from ROM
// 2/ Apply H-Scroll To Data
// 
// Source Address: 0x1590
// Input:          None
// Output:         None

void ORoad::setup_road_x()
{
    // If moved to next chunk of road data, setup x positions
    if (road_pos_change != 0)
    {
        road_data_offset = (road_pos >> 16) << 2;
        //std::cout << std::hex << "Setup x data: " << road_data_offset << std::endl;
        setup_x_data();
    }
    setup_hscroll();
}

// Setup Road X Data from ROM
//
// Source Address: 0x15B0
//
// 1/ Reads Road X Data From ROM for Stage
// 2/ Sets Up Road X Position Table (H-Scroll Not Applied At This Stage)
// 3/ Sets Up Tilemap X Scroll
//
// Inputs:
//
// a4 = Road X Position Table (Before H-Scroll) [DESTINATION]
//
// Note that each word entry in this table represents one scanline of the screen
// 60800 = bottom line of the screen.
// so more of this table is filled based on horizon
//
// In Use:
//
// a6 = Road Data [SOURCE]
// d4 = Index into X-Scroll Destination Table
//
// Road Format:
//
// NOTE - This doesn't contain height or width information :)
//
// Word 0: X Position of segment #1
// Word 1: Y Position of segment #1 (Length of segment #1)
//
// Word 2: X Position of segment #2
// Word 3: Y Position of segment #2 (Length of segment #2)
//
// Note, both X and Y positions need to be summed to each other to make sense.
//
//  | (point 2)
//  | 
//  |
//  | (point 1)
//
// Signed bytes.
//
// Notes:
//
// Stage 1 Data Stored At 3da74


void ORoad::setup_x_data()
{
    uint32_t addr = stage_addr + road_data_offset;

    // d0 = road_x1 + road_x1_next [x difference]
    curve_x1_diff = roms.rom1.read16(addr);
    curve_x1_diff += (int16_t) roms.rom1.read16(addr + 4);

    // d1 = x2 difference
    curve_x2_diff = roms.rom1.read16(addr + 2);
    curve_x2_diff += (int16_t) roms.rom1.read16(addr + 6);

    int32_t distance = (curve_x1_diff * curve_x1_diff) + (curve_x2_diff * curve_x2_diff);

    // The distance formula in Cartesian coordinates is derived from the Pythagorean theorem
    // Use Pythagorus' theorem to find the distance between point 1 & point 2
    // Result stored in d0 [distance]
    distance = (uint16_t) outils::isqrt(distance);

    int32_t d2 = (curve_x1_diff << 14); // extend to 32
    int32_t d3 = (curve_x2_diff << 14); // extend to 32

    // divide x,y
    // source x (16-bit), destination y (32-bit)
    // result y (32-bit). Remainder is upper 16 bits.
    curve_x1_dist = d2 / distance; // here we only care about the lower 16 bits
    curve_x2_dist = d3 / distance;

    set_tilemap_x();

    // Setup Default Straight Positions Of Road at 60800 - 608FF
    for (uint8_t i = 0; i < 0x80;)
    {
        // Set marker to ignore current road position, and use last good value
        road_x[i++] = 0x3210;
        road_x[i++] = 0x3210;
    }

    // Now We Setup The Real Road Data at 60800
    curve_x1_next = 0;
    curve_x2_next = 0;
    curve_inc_old = 0;
    curve_start = 0;

    int16_t scanline = 0x37E / 2; // Index into road_x
    uint8_t road_x_addr = 0;

    // Note this works its way from closest to the camera, further into the horizon
    // We sample 20 Road Positions to generate the road.
    for (uint8_t i = 0; i <= 0x20; i++)
    {
        int32_t x1_next = (int16_t) roms.rom1.read16(&addr);
        int32_t x2_next = (int16_t) roms.rom1.read16(&addr);
        x1_next += (int16_t) roms.rom1.read16(&addr);
        x2_next += (int16_t) roms.rom1.read16(&addr);
        curve_x1_next += x1_next;
        curve_x2_next += x2_next;

        // Calculate curve increment and end position
        create_curve();
        int16_t curve_steps = curve_end - curve_start;
        if (curve_steps < 0) return;
        if (curve_steps == 0) continue; // skip_pos

        // X Amount to increment curve by each iteration
        int32_t xinc = (curve_inc - curve_inc_old) / curve_steps;
        int16_t x = curve_inc_old;

        for (int16_t pos = curve_start; pos < curve_end; pos++)
        {
            x += xinc;              
            if (x < -0x3200 || x > 0x3200) return;
            road_x[scanline] = x;           
            if (--scanline < 0) return;   
        }

        curve_inc_old = curve_inc;
        curve_start = curve_end;
    }
}

// Set The Correct Tilemap X *target* Position From The Road Data
// 
// Source Address: 0x16F6
//
// Use Euclidean distance between a series of points.
// This is essentially the standard distance that you'd measure with a ruler.

void ORoad::set_tilemap_x()
{
    uint32_t addr = stage_addr + road_data_offset;
    
    // d0 = Word 0 + Word 2 + Word 4 + Word 6 [Next 4 x1 positions]
    // d1 = Word 1 + Word 3 + Word 5 + Word 7 [Next 4 x2 positions]
    int16_t x1_diff = roms.rom1.read16(&addr);
    int16_t x2_diff = roms.rom1.read16(&addr);
    x1_diff += (int16_t) roms.rom1.read16(&addr);
    x2_diff += (int16_t) roms.rom1.read16(&addr);
    x1_diff += (int16_t) roms.rom1.read16(&addr);
    x2_diff += (int16_t) roms.rom1.read16(&addr);
    x1_diff += (int16_t) roms.rom1.read16(&addr);
    x2_diff += (int16_t) roms.rom1.read16(&addr);

    int16_t x1_abs = x1_diff;
    int16_t x2_abs = x2_diff;
    if (x1_abs < 0) x1_abs = -x1_abs;
    if (x2_abs < 0) x2_abs = -x2_abs;

    int16_t scroll_x;

    // scroll right
    if (x2_abs > x1_abs)
    {
        scroll_x = (0x100 * x1_diff) / x2_diff;
    }
    // scroll left
    else
    {
        scroll_x = (-0x100 * x2_diff) / x1_diff;
    }
    
    // turn right
    if (x1_diff > 0)
    {
        scroll_x += 0x200;
    }
    else if (x1_diff < 0)
    {
        scroll_x += 0x600;
    }
    else if (x2_diff >= 0)
    {
        scroll_x += 0x200;
    }
    else
    {
        scroll_x += 0x600;
    }

    if (x2_abs > x1_abs)
    {
        int32_t d0 = x1_diff * x2_diff;

        if (d0 >= 0)
            scroll_x -= 0x200;
        else
            scroll_x += 0x200;
    }

    tilemap_h_target = scroll_x;
}

// Interpolates road data into a smooth curve.
//
// Source Address: 0x16B6

void ORoad::create_curve()
{    
    // Note multiplication result should be 32bit, inputs 16 bit
    int32_t d0 = ((curve_x1_next >> 5) * curve_x2_dist);
    int32_t d1 = ((curve_x2_next >> 5) * curve_x1_dist);
    int32_t d2 = ((curve_x1_next >> 5) * curve_x1_dist);
    int32_t d3 = ((curve_x2_next >> 5) * curve_x2_dist);

    d0 -= d1;
    d2 += d3;
    d2 >>= 7;        
    d1 = d2;
    d1 >>= 7;        
    d1 += 0x410;

    d0 = d0 / d1;
    d2 = d2 / d1;
    
    curve_inc = d0; // truncates to int16      
    curve_end = d2 * 4; // truncates to int16
}

// Creates the bend in the next section of road.
//
// Source Address: 0x180C

void ORoad::setup_hscroll()
{
    switch (road_ctrl)
    {
        case ROAD_OFF:
            return;

        case ROAD_R0:
        case ROAD_R0_SPLIT:
            do_road_offset(road0_h, -road_width_bak, false);
            break;

        case ROAD_R1:
            do_road_offset(road1_h, +road_width_bak, false);
            break;

        case ROAD_BOTH_P0:
        case ROAD_BOTH_P1:
            do_road_offset(road0_h, -road_width_bak, false);
            do_road_offset(road1_h, +road_width_bak, false);
            break;

        case ROAD_BOTH_P0_INV:
        case ROAD_BOTH_P1_INV:
            do_road_offset(road0_h, -road_width_bak, false);
            do_road_offset(road1_h, +road_width_bak, true);
            break;

        case ROAD_R1_SPLIT:
            do_road_offset(road1_h, +road_width_bak, true);
            break;  
    }
}

// Adjust the offset for Road 0 or Road 1
//
// Source Address: 0x1864
//
// Setup the H-Scroll Values in RAM (not Road RAM) for Road 0.
// Take into account the position of the player's car.
//
// The road data is adjusted per scanline.
// It is adjusted a different amount depending on how far into the horizon it is. 
// So lines closer to the player's car are adjusted more.

void ORoad::do_road_offset(int16_t* dst_x, int16_t width, bool invert)
{
    int32_t car_offset = car_x_bak + width + oinitengine.camera_x_off; // note extra debug of camera x
    int32_t scanline_inc = 0; // Total amount to increment scanline (increased every line)
    int16_t* src_x = road_x; // a0

    // ---------------------------------------------------------------
    // Process H-Scroll: Car not central on road 0
    // The following loop ensures that the road offset 
    // becomes before significant the closer it is to the player's car
    // ---------------------------------------------------------------
    if (car_offset != 0)
    {
        car_offset <<= 7;
        for (uint16_t i = 0; i <= 0x1FF; i++)
        {
            int16_t h_scroll = scanline_inc >> 16;

            // If ignore car position
            if (*src_x == 0x3210)
                h_scroll = 0;

            // Get next line of road x data
            int16_t x_off = (*src_x++) >> 6;

            if (!invert)
                h_scroll += x_off;
            else
                h_scroll -= x_off;
            
            // Write final h-scroll value 
            (*dst_x++) = h_scroll;

            scanline_inc += car_offset;
        }
        return;
    }
    // ------------------------------
    // Ignore Car Position / H-Scroll
    // ------------------------------

    if (!invert)
    {
        for (uint16_t i = 0; i <= 0x3F; i++)
        {
            (*dst_x++) = (*src_x++) >> 6;
            (*dst_x++) = (*src_x++) >> 6;
            (*dst_x++) = (*src_x++) >> 6;
            (*dst_x++) = (*src_x++) >> 6;

            (*dst_x++) = (*src_x++) >> 6;
            (*dst_x++) = (*src_x++) >> 6;
            (*dst_x++) = (*src_x++) >> 6;
            (*dst_x++) = (*src_x++) >> 6;
        }
    }
    else
    {
        for (uint16_t i = 0; i <= 0x3F; i++)
        {
            (*dst_x++) = -(*src_x++) >> 6;
            (*dst_x++) = -(*src_x++) >> 6;
            (*dst_x++) = -(*src_x++) >> 6;
            (*dst_x++) = -(*src_x++) >> 6;

            (*dst_x++) = -(*src_x++) >> 6;
            (*dst_x++) = -(*src_x++) >> 6;
            (*dst_x++) = -(*src_x++) >> 6;
            (*dst_x++) = -(*src_x++) >> 6;
        }
    }
}

// Main Loop
// 
// Source Address: 0x1B12
// Input:          None
// Output:         None

// Road height information is stored in blocks in this ROM.
// Each block is interchangeable and can be used with any segment of road.
// Each block can vary in length.
//
// The format of each height block is as follows:
//
// +0  [Byte] Jump Table Control
//            0 = Elevated section
//            4 = Flat section
// +1  [Byte] Length of each section
// +2  [Byte] Length of ascents (multiplied with +1)
// +3  [Byte] Length of descents (multiplied with +1)
// +4  [Word] 1st Signed Height Value For Section Of Road
// +6  [Word] 2st Signed Height Value For Section Of Road
//
// etc.
//
// +xx [Word] 0xFFFF / -1 Signifies end of height data

void ORoad::setup_road_y()
{
    pos_fine_diff = pos_fine - pos_fine_old;
    pos_fine_old = pos_fine;

    // Setup horizon base value if necessary
    if (!horizon_set)
    {
        horizon_base = 0x240;
        horizon_set = 1;
    }

    // Code omitted that doesn't seem to be used

    switch (height_ctrl)
    {
        // Clear Road Height Segment & Init Segment 0
        case 0:
            height_lookup = 0;
            init_height_seg();
            break;

        // Init Next Road Height Segment
        case 1:
            init_height_seg();
            break;

        // Segment Initialized: Use Elevation Flag
        case 2:
            do_elevation();
            break;

        // Segment Initalized: ??? steep hill
        case 3:
            do_elevation_hill();
            break;

        // Needed for stage 0x1b
        case 4:
            do_level_4d();
            break;

        // Segment Initalized: Ignore Elevation Flag
        case 5:
            do_horizon_adjust();
            break;
    }
}

// Source Address: 0x1BCE
void ORoad::init_height_seg()
{
    height_index = 0;
    height_inc = 0;
    elevation = FLAT;
    height_step = 1;    

    height_lookup_wrk = height_lookup;

    // Get Address of actual road height data
     uint32_t h_addr = roms.rom1.read32(ROAD_HEIGHT_LOOKUP + (height_lookup_wrk * 4));

    height_ctrl2 = roms.rom1.read8(&h_addr);
    dist_ctrl = roms.rom1.read8(&h_addr);

    switch (height_ctrl2)
    {
        case 0:
            init_elevation(h_addr);
            break;
        // Chicane on stage 1
        case 1:
        case 2:
            init_elevation_hill(h_addr);
            break;
        case 3:
            init_level_4d(h_addr);
            //std::cerr << "init_height_seg() - UNIMPLEMENTED height_ctrl2 value" << std::endl;
            break;
        case 4:
            init_horizon_adjust(h_addr);
            break;
    }
}

// Note - this can be either flat, up or down at this stage
//
// Source Address: 0x1C2C
void ORoad::init_elevation(uint32_t& addr)
{
    up_mult = roms.rom1.read8(&addr);
    down_mult = roms.rom1.read8(&addr);
    height_addr = addr;
    height_ctrl = 2; // Use do_elevation_flat function below
    do_elevation();
}

void ORoad::do_elevation()
{
    height_step += pos_fine_diff * 12;
    uint16_t d3 = dist_ctrl;

    if (elevation > 0)
        d3 *= down_mult;
    else if (elevation < 0)
        d3 *= up_mult;

    uint16_t d1 = (height_step / d3);
    if (d1 > 0xFF) d1 = 0xFF;
    d1 += 0x100;

    height_start = d1;
    height_end = d1;

    height_index += height_inc;
    height_inc = 0;

    // Increment to next height entry
    if (height_start == 0x1FF)
    {
        height_start = 0x1FF;
        height_end = 0x1FF;
        height_step = 1; // Set position on road segment to start
        height_inc = 1;
        elevation = FLAT;
        return;
    }
    if (height_lookup == 0 || height_lookup_wrk != 0)
        return;
    // If working road height index is 0, then init next section
    height_ctrl = 1;
}

// Source: 1CE4 (first used at the chicane at stage 1)
void ORoad::init_elevation_hill(uint32_t& addr)
{
    updowncombo = roms.rom1.read16(&addr);
    height_addr = addr;
    do_height_inc = 1;
    height_inc = 0;
    height_end = 0x100;
    height_ctrl = 3; // Use do_elevation_hill function below
    do_elevation_hill();
}

// Source: 1D04
void ORoad::do_elevation_hill()
{
    int16_t d1 = pos_fine_diff * 12;
    uint16_t d3 = dist_ctrl;
    height_index += height_inc; // Next height entry
    height_inc = 0;

    if (height_index == 0 || do_height_inc == 0)
    {
        // 1D50 inc_pos_in_seg:
        height_step += d1; // height step is 1 on mame...
        d1 = 0;
        d1 = height_step / d3;
        if (d1 > 0xFF) d1 = 0xFF;

        height_start = d1;
        if (height_start > 0xFE)
        {
            height_start = 0xFF;
            height_step = 1;
            height_inc = 1;
            elevation = FLAT;
        }

        if (height_index != 0)
        {
            d1 = 0xFF - height_start;
        }

        height_start = d1 + 0x100;
    }
    // 1D2E 
    else
    {
        //d1 = d1 / dist_ctrl;
        updowncombo -= (d1 / dist_ctrl);    
        height_start = 0x1FF;

        if (updowncombo < 0) // // Set flag so we inc next time
            do_height_inc = 0;
    }
}

// Source: 1DAC
void ORoad::init_level_4d(uint32_t& addr)
{
    updowncombo = roms.rom1.read16(&addr);
    height_addr = addr;
    do_height_inc = 1;
    height_inc = 0;
    height_ctrl = 4; // Use function below
    do_level_4d();
}

// Source: 1D46
void ORoad::do_level_4d()
{
    uint16_t d1 = pos_fine_diff * 12;

    height_index += height_inc;
    height_inc = 0;
    
    // 1E1C - On crest of hill (hill type >= 6)
    if (height_index >= 6)
    {
        //std::cout << "untested road code block (level 4d)" << std::endl;
        uint16_t d3 = dist_ctrl;
        if (do_height_inc != 0)
        {
            updowncombo -= (d1 / d3);
            height_start = 0x1FF;
            height_end = 0x100;
            // 1E46
            if (updowncombo < 0)
            {
                height_addr += 12;
                do_height_inc = 0;
            }
        }
        // 1E58
        else
        {
            height_step += d1;
            d1 = height_step / d3;
            if (d1 > 0xFF) d1 = 0xFF;
            height_start = 0x1FF - d1;
            if (height_start != 0x100) return;
            
            height_step = 1; // Set position on road segment to start
            height_inc = 1;
            elevation = 0;
        }
    }
    // 1DEA
    else
    {
        height_step += d1;
        d1 = height_step / 4;
        if (d1 > 0xFF) d1 = 0xFF;
        d1 += 0x100;
        height_start = d1;
        height_end = d1;

        if (height_start < 0x1FF) return;
        
        // set_end2:
        height_start = 0x1FF;
        height_end = 0x1FF;
        height_step = 1; // Set position on road segment to start
        height_inc = 1;
        elevation = 0;
    }
}
    
// Source Address: 0x1EB6
void ORoad::init_horizon_adjust(uint32_t& addr)
{
    height_addr = addr;
    //  Base Horizon Y-Offset - Up/Down Modifier
    horizon_mod = (int16_t) roms.rom1.read16(addr) - horizon_base;
    height_ctrl = 5; // Use do_horizon_adjust() function below
    do_horizon_adjust();
}

void ORoad::do_horizon_adjust()
{
    height_step += pos_fine_diff * 12;
    uint16_t d1 = (height_step / dist_ctrl);

    // Scale height between 100 and 1FF
    if (d1 > 0xFF) d1 = 0xFF;
    d1 += 0x100;

    height_start = d1;
    height_end = d1;
}

// ----------------------------------------------------------------------------
// END OF SETUP SECTION
// ----------------------------------------------------------------------------

// Source Address: 0x1F00
void ORoad::set_road_y()
{
    switch (height_ctrl2)
    {
        // Set Normal Y Coordinate
        case 0:
        case 1:
        case 2:
        case 3:
            set_y_interpolate();
            break;
        // Set Y with Horizon Adjustment (Stage 2 - Gateway)
        case 4:
            set_y_horizon();
            break;
    }
}


// 1/ Takes distance into track section
// 2/ Interpolates this value into a series of counters, stored between 60700 - 6070D
// 3/ Each of these counters define how many times to write and adjust the height value

// Source Address: 0x1F22
void ORoad::set_y_interpolate()
{
    // ------------------------------------------------------------------------
    // Convert desired elevation of track into a series of section lengths
    // Each stored in 7 counters which will be used to output the track
    // ------------------------------------------------------------------------

    uint16_t d2 = height_end;
    uint16_t d1 = 0x1FF - d2;

    section_lengths[0] = d1;
    d2 >>= 1;
    section_lengths[1] = d2;    
    
    uint16_t d3 = 0x200 - d1 - d2;
    section_lengths[3] = d3;
    d3 >>= 1;
    section_lengths[2] = d3;
    section_lengths[3] -= d3;
    
    d3 = section_lengths[3];
    section_lengths[4] = d3;
    d3 >>= 1;
    section_lengths[3] = d3;
    section_lengths[4] -= d3;
    
    d3 = section_lengths[4];
    section_lengths[5] = d3;
    d3 >>= 1;
    section_lengths[4] = d3;
    section_lengths[5] -= d3;

    d3 = section_lengths[5];
    section_lengths[6] = d3;
    d3 >>= 1;
    section_lengths[5] = d3;
    section_lengths[6] -= d3;

    length_offset = 0;

    // Address of next entry in road height table data [rom so no need to scale]
    a1_lookup = (height_index * 2) + height_addr;

    // Road Y Positions (references to this decrement) [Destination]    
    y_addr = 0x200 + road_p1;

    a3_o = 0;
    road_unk[a3_o++] = 0x1FF;
    road_unk[a3_o++] = 0;
    counter = 0; // Reset interpolated counter index to 0
    //uint16_t d4 = 0x200; // 200 (512 pixels)

    // height_final = (Next Height Value * (Distance into section - 0x100)) / 16
    height_final = (((int16_t) roms.rom1.read16(a1_lookup)) * (height_start - 0x100)) >> 4;

    // 1faa 
    int32_t horizon_copy = horizon_base << 4;
    if (height_ctrl2 == 2) 
        horizon_copy += height_final;

    //set_y_2044(0x200, horizon_copy, 0, y_addr, 2);

    change_per_entry = horizon_copy;
    scanline = 0x200; // 200 (512 pixels)
    total_height = 0;
    set_y_2044();

    // loc: 2044    
}

// Source: 0x2044
void ORoad::set_y_2044()
{
    if (change_per_entry > 0x10000)
        change_per_entry = 0x10000;

    d5_o = change_per_entry;

    // ------------------------------------------------------------------------
    // Get length of this road section in scanlines
    // ------------------------------------------------------------------------
    int16_t section_length = section_lengths[length_offset++] - 1;
    scanline -= section_length;

    // ------------------------------------------------------------------------
    // Write this section of road number of times dictated by length
    // Source: 0x2080: write_y
    // ------------------------------------------------------------------------
    if (section_length >= 0)
    {
        for (uint16_t i = 0; i <= section_length; i++)
        {
            total_height += change_per_entry; 
            road_y[--y_addr] = (total_height << 4) >> 16;
        }        
    }

    // ------------------------------------------------------------------------
    // Whilst there are still sections of track to write, read the height for
    // the upcoming section
    // Source: 0x216C
    // ------------------------------------------------------------------------
    if (++counter != 7)
    {
        read_next_height();
        return;
    }

    // Writing last part of interpolated data
    road_unk[a3_o] = 0;

    int16_t y = roms.rom1.read16(a1_lookup);

    // Return if not end at end of height section data
    if (y != -1) return;

    // At end of height section data, initalize next segmnet 
    if (height_lookup == height_lookup_wrk)
        height_lookup = 0;

    height_ctrl = 1; // init next road segment
}

// Source Address: 0x1FC6
void ORoad::read_next_height()
{
    switch (height_ctrl2)
    {
        case 0:
            // set_elevation_flag
            break;

        case 1:
            change_per_entry += height_final;
            set_elevation();
            return; // Note we return

        case 2:
            set_elevation();
            return; // Note we return

        case 3:
            if (height_index >= 6)
            {
                change_per_entry += height_final;
                set_elevation();
                return;
            }
            // otherwise set_elevation_flag
            break;

        default:
            std::cout << "Possible Error in ORoad::read_next_height()" << std::endl;
            break;
    }

    // 1ff2: set_elevation_flag
    // Note the way this bug was fixed
    // Needed to read the signed value into an int16_t before assigning to a 32 bit value
    change_per_entry = ((int16_t) roms.rom1.read16(&a1_lookup)) << 4;
    change_per_entry += d5_o;
    if (counter != 1)
    {
        set_elevation();
        return;
    }

    // Writing first part of interpolated data

    change_per_entry -= height_final;

    int32_t horizon_shift = (horizon_base << 4);

    if (horizon_shift == change_per_entry)
    {
        set_elevation();
    }
    else if (change_per_entry > horizon_shift)
    {
        elevation = UP;
        set_elevation();
    }
    else
    {
        elevation = DOWN;
        set_elevation();
    }
}

// Source Address: 0x2028
void ORoad::set_elevation()
{
    // d2: h
    // d3: total_height
    // d4: scanline
    // d5: h_copy (global)
    d5_o -= change_per_entry;

    if (d5_o == 0)
    {
        set_y_2044();
        return;
    }

    // should be 2400, 2400, 2400, ffffe764, ffffdc00
    if (d5_o < 0)
        d5_o = -d5_o;

    road_unk[a3_o++] = scanline;
    int32_t total_height_wrk = total_height;
    if (total_height_wrk < 0)
        total_height_wrk = -total_height_wrk;
    total_height_wrk <<= 4;

    road_unk[a3_o++] = total_height_wrk >> 16;

    set_y_2044();
}

// Set Horizon Base Position
//
// Source Address: 0x219E
void ORoad::set_y_horizon()
{
    uint32_t a0 = 0x200 + road_p1; // a0 = Road Height Positions + Relevant Offset
    
    int32_t d1 = (horizon_mod * (height_start - 0x100)) >> 4;
    int32_t d2 = (horizon_base << 4) + d1;
    
    uint32_t total_height = 0;

    // write_next_y:
    for (uint16_t i = 0; i <= 0x1FF; i++) // (1FF height positions)
    {
        total_height += d2;
        uint32_t d0 = (total_height << 4) >> 16;
        road_y[--a0] = d0;
    }

    road_unk[0] = 0;
    
    // If not at end of section return. Otherwise, setup new horizon y-value
    if (height_start != 0x1FF) return;

    // Read Up/Down Multiplier Word From Lookup Table and set new horizon base value
    horizon_base = (int32_t) roms.rom1.read16(height_addr);

    if (height_lookup == height_lookup_wrk)
        height_lookup = 0;

    // Set master switch control to init next road segment
    height_ctrl = 1;
}

// Aspect correct road inclines.
// Set Horizon Tilemap Y Position
//
// Source Address: 0x13B8
void ORoad::set_horizon_y()
{
    // ------------------------------------------------------------------------
    // Aspect correct road inclines
    // ------------------------------------------------------------------------

    uint32_t road_int = 0; // a0
    uint32_t road_y_addr = (0 + road_p1); // a1

    // read_next
    while (true && !debug_road)
    {
        int16_t d0 = road_unk[road_int];
        if (d0 == 0) break;
        int16_t d7 = d0 >> 3;
        int16_t d6 = (d7 + d0) - 0x1FF;

        if (d6 > 0)
        {
            d7 += d7;
            d7 -= d6;
            d7 >>= 1;
            d0 = 0x1FF - d7;
        }
        // 13e6
        d6 = d7 >> 1; 
        if (d6 <= 2) break;
        int16_t d1 = d0 - d6;
        int16_t d2 = d0;
        int16_t d3 = d0 + d6;
        int16_t d4 = d0 + d7;

        // 1402: Chris - made some edits below here, seems to go rather wrong from here onwards
        d0 -= d7;
        int16_t d5 = d0;
        uint16_t a2 = road_y_addr + d5;
        
        d0 = road_y[road_y_addr + d0];
        d1 = road_y[road_y_addr + d1];
        d2 = road_y[road_y_addr + d2];
        d3 = road_y[road_y_addr + d3];
        d4 = road_y[road_y_addr + d4];

        // 1426:
        d5 = d0;
        
        int32_t d2l = (d2 + d0 + d4) * 0x5555; // turn d2 into a long
        d2l >>= 16;
        int32_t d1l = (d1 + d0 + (int16_t) d2l) * 0x5555; // turn d2 into a long
        d1l >>= 16;
        int32_t d3l = (d3 + d4 + (int16_t) d2l) * 0x5555;
        d3l >>= 16;

        d0 -=  (int16_t) d1l;
        d1l -= (int16_t) d2l;
        d2l -= (int16_t) d3l;
        d3l -= d4;

        d0 = (d0 << 2)  / d6;
        d1 = (d1l << 2) / d6;
        d2 = (d2l << 2) / d6;
        d3 = (d3l << 2) / d6;

        d5 <<= 2;
        d6--;

        // loop 1:
        for (int16_t i = 0; i <= d6; i++)
        {
            road_y[a2++] = d5 >> 2;
            d5 -= d0;
        }
        // loop 2:
        for (int16_t i = 0; i <= d6; i++)
        {
            road_y[a2++] = d5 >> 2;
            d5 -= d1;
        }
        // loop 3:
        for (int16_t i = 0; i <= d6; i++)
        {
            road_y[a2++] = d5 >> 2;
            d5 -= d2;
        }
        // loop 4:
        for (int16_t i = 0; i <= d6; i++)
        {
            road_y[a2++] = d5 >> 2;
            d5 -= d3;
        }

        road_int += 2; 
    } // end loop

    // ------------------------------------------------------------------------
    // Set Horizon Y Position
    // ------------------------------------------------------------------------

    int16_t y_pos = road_y[road_p3] >> 4;
    horizon_y2 = -y_pos + 224;

    //std::cout << std::hex << horizon_y2 << std::endl;
}

// Parse Road Scanline Data.
//
// Output hardware ready format.
// Also output priority information for undulations in road for sprite hardware.
// This is so that sprites can be hidden behind crests of hills.
//
// Destination Output Format:
//
// ----s--- --------  Solid fill (1) or ROM fill
// -------- -ccccccc  Solid color (if solid fill)
// -------i iiiiiiii  Index for other tables
//
// Source: 0x1318

void ORoad::do_road_data()
{
    // Road data in RAM #1 [Destination] (Solid fill/index fill etc)
    // This is the final block of data to be output to road hardware
    uint32_t addr_dst = 0x400 + road_p1;        // [a0]

    // Road data in RAM [Source]
    uint32_t addr_src = 0x200 + road_p1;        // [a1]

    // Road Priority Elevation Data [Destination]
    uint32_t addr_priority = 0x280 + road_p1;

    int16_t rom_line_select = 0x1FF;            // This is the triangular road shape from ROM. Lines 0 - 512. 
    int16_t src_this = road_y[--addr_src] >> 4; // source entry #1 / 16 [d0]
    int16_t src_next = 0;                       // [d4]
    int16_t write_priority = 0;                 // [d5]

    road_y[--addr_dst] = rom_line_select; // Write source entry counter to a0 / destination
    int16_t scanline = 254; //  Loop counter #2 (Scanlines) - Start at bottom of screen

    // ------------------------------------------------------------------------
    // Draw normal road
    // ------------------------------------------------------------------------
    while (--rom_line_select > 0) // Read next line of road source data
    {
        src_next = road_y[--addr_src] >> 4;
        
        // Plot next position of road using rom_line_select source
        // The speed at which we advance through the source data is controlled by the values in road_y, 
        // which are a sequential list of numbers when the road is flat
        if (src_this < src_next)
        {
            // Only draw if within screen area
            if (--scanline <= 0)
            {
                road_y[addr_priority] = 0; road_y[addr_priority + 1] = 0; // Denote end of priority list
                return;
            }
            src_this = src_next; // d0 = source entry #1 / 16
         
            road_y[--addr_dst] = rom_line_select; // Set next line of road appearance info, from ROM
            write_priority = -1;                  // Denote that we need to write priority data
        }
        else if (src_this == src_next) 
            continue;
        // Write priority data for elevation. Only called when there is a hill. 
        // This is denoted by the next number in the sequence being smaller than the current
        // Note: This isn't needed to directly render the road.
        else if (write_priority == -1)
        {
            road_y[addr_priority++] = rom_line_select; // Write rom line select value
            road_y[addr_priority++] = src_this; // Write source entry value
            write_priority = 0; // Denote that values have been written
        }
    } // end loop

    // ------------------------------------------------------------------------
    // Fill Horizon above road at top of screen
    //
    // Note that this is the area above the road, and is detected because
    // the rom line select is negative, and therefore there is no more to 
    // read.
    // ------------------------------------------------------------------------
    if (rom_line_select <= 0)
    {
        const uint16_t SOLID_FILL = 0x800;
        
        // d3 = (0x3F | 0x800) = 0x83F [1000 00111111] Sets Solid Fill, Transparent Colour
        const uint16_t TRANSPARENT = 0x3F | SOLID_FILL; 
        int16_t d7 = 255 - src_next - scanline;

        if (d7 < 0)
            d7 = SOLID_FILL; // use default solid fill

        // loc 1388
        else if (d7 > 0x3F)
        {
            while (scanline-- > 0)
                road_y[--addr_dst] = TRANSPARENT; // Copy transparent line to ram
            
            // end
            road_y[addr_priority] = 0; road_y[addr_priority + 1] = 0;
            return;
        }
        else
            d7 |= SOLID_FILL; // Solid Fill Bits | Colour Info

        // 1392
        do // Output solid colours while looping (compare with default solid fill d3)
        {
            road_y[--addr_dst] = d7; // Output solid colour
            if (--scanline < 0) // Check whether scanloop counter expired
            {
                road_y[addr_priority] = 0; road_y[addr_priority + 1] = 0;
                return;
            }
            road_y[--addr_dst] = d7; // Output solid colour
            if (--scanline < 0) // Check whether scanloop counter expired
            {
                road_y[addr_priority] = 0; road_y[addr_priority + 1] = 0;
                return;
            }
            d7++; // Increment solid colour
        }
        while (d7 <= TRANSPARENT);
        
        while (scanline-- > 0)
            road_y[--addr_dst] = TRANSPARENT; // Copy transparent line to ram
    // 1346
    }

    // end:
    road_y[addr_priority] = 0; road_y[addr_priority + 1] = 0; // Denote end of priority list
}

// ------------------------------------------------------------------------------------------------
// ROUTINES FOLLOW TO BLIT RAM TO ROAD HARDWARE
// ------------------------------------------------------------------------------------------------

// Source Address: 0x14C8
void ORoad::blit_roads()
{
    const uint32_t road0_adr = 0x801C0;
    const uint32_t road1_adr = 0x803C0;

    switch (road_ctrl)
    {
        case ROAD_OFF:         // Both Roads Off
            return;

        case ROAD_R0:          // Road 0
        case ROAD_R0_SPLIT:    // Road 0 (Road Split.)
            blit_road(road0_adr);
            hwroad.write_road_control(0);
            break;

        case ROAD_R1:          // Road 1
        case ROAD_R1_SPLIT:    // Road 1 (Road Split. Invert Road 1)
            blit_road(road1_adr);
            hwroad.write_road_control(3);
            break;

        case ROAD_BOTH_P0:     // Both Roads (Road 0 Priority) [DEFAULT]
        case ROAD_BOTH_P0_INV: // Both Roads (Road 0 Priority) (Road Split. Invert Road 1)
            blit_road(road0_adr);
            blit_road(road1_adr);
            hwroad.write_road_control(1);
            break;

        case ROAD_BOTH_P1:     // Both Roads (Road 1 Priority) 
        case ROAD_BOTH_P1_INV: // Both Roads (Road 1 Priority) (Road Split. Invert Road 1)
            blit_road(road0_adr);
            blit_road(road1_adr);
            hwroad.write_road_control(2);
            break;
    }
}

void ORoad::blit_road(uint32_t a0)
{
    uint32_t a2 = 0x400 + road_p2;

    // Write 0x1A0 bytes total Src: (0x260 - 0x400) Dst: 0x1C0
    for (int16_t i = 0; i <= 13; i++)
    {
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);

        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
        a0 -= 2; hwroad.write16(a0, road_y[--a2]);
    }
}

void ORoad::output_hscroll(int16_t* src, uint32_t dst)
{
    const int32_t d6 = 0x654;
    uint32_t src_addr = 0;

    // Copy 16 bytes
    for (int32_t i = 0; i <= 0x3F; i++)
    {
        int16_t d0h = -src[src_addr++] + d6;
        int16_t d0l = -src[src_addr++] + d6;
        int16_t d1h = -src[src_addr++] + d6;
        int16_t d1l = -src[src_addr++] + d6;
        int16_t d2h = -src[src_addr++] + d6;
        int16_t d2l = -src[src_addr++] + d6;
        int16_t d3h = -src[src_addr++] + d6;
        int16_t d3l = -src[src_addr++] + d6;
        hwroad.write16(&dst, d0h);
        hwroad.write16(&dst, d0l);
        hwroad.write16(&dst, d1h);
        hwroad.write16(&dst, d1l);
        hwroad.write16(&dst, d2h);
        hwroad.write16(&dst, d2l);
        hwroad.write16(&dst, d3h);
        hwroad.write16(&dst, d3l);
    }
}

// Copy Background Colour To Road.
// + Scroll stripe data over road based on fine position
// + pos_fine is used as an offset into a table in rom.
//
// Source Address: 0x1ABC
//
// Notes:
//
// C00-FFF  ----bbbb --------  Background color index
//          -------- s-------  Road 1: stripe color index
//          -------- -a------  Road 1: pixel value 2 color index
//          -------- --b-----  Road 1: pixel value 1 color index
//          -------- ---c----  Road 1: pixel value 0 color index
//          -------- ----s---  Road 0: stripe color index
//          -------- -----a--  Road 0: pixel value 2 color index
//          -------- ------b-  Road 0: pixel value 1 color index
//          -------- -------c  Road 0: pixel value 0 color index
 
void ORoad::copy_bg_color()
{
    // Scroll stripe data over road based on fine position
    uint32_t pos_fine_copy = (pos_fine & 0x1F) << 10;
    uint32_t src = ROAD_BGCOLOR + pos_fine_copy;
    uint32_t dst = HW_BGCOLOR;

    // Copy 1K
    for (int i = 0; i < 32; i++)
    {
        hwroad.write32(&dst, roms.rom1.read32(&src));
        hwroad.write32(&dst, roms.rom1.read32(&src));
        hwroad.write32(&dst, roms.rom1.read32(&src));
        hwroad.write32(&dst, roms.rom1.read32(&src));
        hwroad.write32(&dst, roms.rom1.read32(&src));
        hwroad.write32(&dst, roms.rom1.read32(&src));
        hwroad.write32(&dst, roms.rom1.read32(&src));
        hwroad.write32(&dst, roms.rom1.read32(&src));
    }
}