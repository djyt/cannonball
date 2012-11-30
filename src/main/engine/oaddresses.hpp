/***************************************************************************
    68000 Program Code Addresses 
    Addresses to data within the Master and Sub CPU Program ROMs.
    
    These are typically large blocks of data that we don't want to include
    in the codebase. 
    
    To convert Cannonball to work with a different version of the program
    roms (e.g. the Japanese edition), this file would need to be updated.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

// ----------------------------------------------------------------------------
// Text Structures
// ----------------------------------------------------------------------------

// Text1 = Use BlitText1 routine | Text2 = Use BlitText2 routine

// Text: Credits
const uint16_t TEXT1_CREDIT  = 0x6D38;
const uint16_t TEXT1_CREDITS = 0x6D48;
const uint16_t TEXT1_CLEAR_CREDITS = 0x6D58;
const uint16_t TEXT1_FREEPLAY = 0x6D6C;

// Text: Course Map
const uint16_t TEXT2_COURSEMAP = 0xBBC2;

// Text: Press Start
const uint16_t TEXT1_PRESS_START = 0xBBD0;
const uint16_t TEXT1_CLEAR_START = 0xBBEC;

// Text: Insert Coins
const uint16_t TEXT1_INSERT_COINS = 0xBC08;
const uint16_t TEXT1_CLEAR_COINS  = 0xBC1E;

// Text: Game Over
const uint16_t TEXT2_GAMEOVER = 0xBCB0;

// Text: Select Music By Steering
const uint16_t TEXT2_SELECT_MUSIC = 0xBCBE;

// Text: 1986 Sega
const uint16_t TEXT1_1986_SEGA = 0xBCF2;

// Text: Copyright Symbol
const uint16_t TEXT1_COPYRIGHT = 0xBD04;

// Text: Magical Sound Shower
const uint16_t TEXT2_MAGICAL = 0xCE04;

// Text: Passing Breeze
const uint16_t TEXT2_BREEZE = 0xCE1E;

// Text: Splash Wave
const uint16_t TEXT2_SPLASH = 0xCE38;

// Text: Your Score
const uint16_t TEXT1_YOURSCORE = 0xD5E0;

// Text: Best OutRunners
const uint16_t TEXT2_BEST_OR = 0xD5F2;

// Text: Score, Name, Route, Record
const uint16_t TEXT1_SCORE_ETC = 0xD606;

// Text: ABCDEFGHIJKLMNOPQRSTUVWXYZ
const uint16_t TEXT2_ALPHABET = 0xD5C2;

// Text: Extend Time
const uint16_t TEXT1_EXTEND1 = 0x90DC;
const uint16_t TEXT1_EXTEND2 = 0x90F6;

// Text: Clear Extend Time
const uint16_t TEXT1_EXTEND_CLEAR1 = 0x9110;
const uint16_t TEXT1_EXTEND_CLEAR2 = 0x912A;

// Text: Laptime
const uint16_t TEXT1_LAPTIME1 = 0x9144;
const uint16_t TEXT1_LAPTIME2 = 0x9150;

// Text: Clear Laptime
const uint16_t TEXT1_LAPTIME_CLEAR1 = 0x915C;
const uint16_t TEXT1_LAPTIME_CLEAR2 = 0x917A;

// Text: Easter Egg
const uint16_t TEXT1_EASTER       = 0x91B4;
const uint16_t TEXT1_EASTER_CLEAR = 0x91D6;

// Text: Bonus Points Section
const uint16_t TEXT2_BONUS_POINTS = 0x9C0C;
const uint16_t TEXT1_BONUS_STOP   = 0x9C1C;
const uint16_t TEXT1_BONUS_SEC    = 0x9C26;
const uint16_t TEXT1_BONUS_X      = 0x9C34;
const uint16_t TEXT1_BONUS_PTS    = 0x9C3E;
const uint16_t TEXT1_BONUS_100K   = 0x9C4A;
const uint16_t TEXT2_BONUS_CLEAR1 = 0x9C52;
const uint16_t TEXT2_BONUS_CLEAR2 = 0x9C64;
const uint16_t TEXT2_BONUS_CLEAR3 = 0x9C78;

// ----------------------------------------------------------------------------
// HUD
// ----------------------------------------------------------------------------

// SCORE Graphic (2 Lines)
const uint16_t HUD_SCORE1 = 0xBC3E;
const uint16_t HUD_SCORE2 = 0xBC4C;

// TIME Graphic (2 Lines)
const uint16_t HUD_TIME1 = 0xBC5A;
const uint16_t HUD_TIME2 = 0xBC66;

// KPH (2 Lines)
const uint16_t HUD_KPH1 = 0xBC72;
const uint16_t HUD_KPH2 = 0xBC7E;

// STAGE (2 Lines)
const uint16_t HUD_STAGE1 = 0xBC8A;
const uint16_t HUD_STAGE2 = 0xBC98;

// Number "1" to appear after stage
const uint16_t HUD_ONE = 0xBCA6;

// LAP (2 Lines)
const uint16_t HUD_LAP1 = 0xBCDA;
const uint16_t HUD_LAP2 = 0xBCE6;

// ----------------------------------------------------------------------------
// Tilemaps
// ----------------------------------------------------------------------------

// Tilemap hardware addresses
const uint32_t HW_FG_PSEL       = 0xE80;
const uint32_t HW_BG_PSEL       = 0xE82;
const uint32_t HW_BG_HSCROLL    = 0xE9A;
const uint32_t HW_FG_VSCROLL    = 0xE90;
const uint32_t HW_BG_VSCROLL    = 0xE92;
const uint32_t HW_FG_HSCROLL    = 0xE98;

// In-Game Tilemap Defaults
const uint32_t TILES_PAGE_FG1   = 0x17E4C;
const uint32_t TILES_PAGE_BG1   = 0x17E5C;
const uint32_t TILES_PAGE_FG2   = 0x17E68;  // Used for road split
const uint32_t TILES_PAGE_BG2   = 0x17E78;  // Used for road split
const uint32_t TILES_DEF_LOOKUP = 0x17E84;  // Tilemap default lookup indexes, for values in table below
const uint32_t TILES_TABLE      = 0x17EAC;  // Stage Tilemap Default Values

const uint32_t TILES_MINIMAP    = 0x8C04;

// Table of h_scroll offsets (words) for road split
// Note the h_scroll is set manually during the road split from the actual road position
const uint32_t H_SCROLL_TABLE   = 0x30B00;

// Tilemap: Music Selection Screen
const uint32_t TILEMAP_MUSIC_SELECT = 0x383F2;

// ----------------------------------------------------------------------------
// Palettes
// ----------------------------------------------------------------------------

// In-Game Tilemap Palettes
const uint32_t TILEMAP_PALS = 0xDF9C;

// Palette: Music Select Screen
const uint32_t PAL_MUSIC_SELECT = 0x175CC;

// Palette Data. Stored in blocks of 32 bytes.
const uint32_t PAL_DATA = 0x14ED8;

// Table of long addresses of ground colours
const uint32_t PAL_GROUND_TABLE = 0x17350;

// Table of palette addresses
const uint32_t PAL_TABLE = 0x17590;

// Palette Data: Best Outrunners Name Entry
const uint32_t PAL_BESTOR = 0x17DCC;

// ----------------------------------------------------------------------------
// Sprites
// ----------------------------------------------------------------------------

// Sprite Animation Sequences For Crashes
//
// +00 [Long] Sprite Data Frame Address
// +04 [Byte] Bit 7: Set to H-Flip Sprite
//            Bit 0: Set Sprite to Sprite Priority Higher (Unused so far?)
// +05 [Byte] Sprite Colour Palette
// +06 [Byte] Passenger Frame
//            OR FOR Passenger Sprites: X Offset
// +07 [Byte] Set to denote end of frame sequence
//            OR FOR Passenger Sprites: Y Offset

const uint32_t SPRITE_CRASH_SPIN1      = 0x2294;
const uint32_t SPRITE_CRASH_SPIN2      = 0x22D4;
const uint32_t SPRITE_BUMP_DATA1       = 0x2314;
const uint32_t SPRITE_BUMP_DATA2       = 0x232C;
const uint32_t SPRITE_CRASH_MAN1       = 0x2344;
const uint32_t SPRITE_CRASH_GIRL1      = 0x23B4;
const uint32_t SPRITE_CRASH_FLIP       = 0x2424; // Flip: Car
const uint32_t SPRITE_CRASH_FLIP_MAN1  = 0x2464; // Flip: Man
const uint32_t SPRITE_CRASH_FLIP_GIRL1 = 0x255C; // Flip: Girl
const uint32_t SPRITE_CRASH_FLIP_MAN2  = 0x24DC; // Post Flip: Man
const uint32_t SPRITE_CRASH_FLIP_GIRL2 = 0x25D4; // Post Flip: Girl
const uint32_t SPRITE_CRASH_MAN2       = 0x2604;
const uint32_t SPRITE_CRASH_GIRL2      = 0x2660;

// Sprite Default Properties
//
// +0: Sprite Properties
// +1: Draw Properties
// +2: Sprite Priority
// +3: Sprite Palette
// +4: Sprite Type
// +6: Sprite X World
// +8: Sprite Y World
// +A: Sprite Z
// +C: Routine Address
const uint32_t SPRITE_DEF_PROPS1 = 0x2B70;

// Best OutRunners Sprites
const uint32_t SPRITE_DEF_PROPS2 = 0x2FB2;

// Sprite: Cloud Frames
const uint32_t SPRITE_CLOUD_FRAMES = 0x4246;

// Sprite: Mini Tree Frames
const uint32_t SPRITE_MINITREE_FRAMES = 0x435C;

// Sprite: Grass Frames (Vary in thickness. Closer to the camera = Need Thicker Sprite.)
const uint32_t SPRITE_GRASS_FRAMES = 0x4548;

// Sprite: Sand Frames
const uint32_t SPRITE_SAND_FRAMES = 0x4588;

// Sprite: Stone Frames
const uint32_t SPRITE_STONE_FRAMES = 0x45C8;

// Sprite: Water Frames (Vary in thickness. Closer to the camera = Need Thicker Sprite.)
const uint32_t SPRITE_WATER_FRAMES = 0x4608;

// Sprite: Shadow Frames
const uint32_t SPRITE_SHDW_FRAMES = 0x7862;

// Sprite: Ferrari Frames, Offsets
const uint32_t SPRITE_FERRARI_FRAMES = 0x9ECC;

// Sprite: Frame Data For Ferrari Skid
const uint32_t SPRITE_SKID_FRAMES = 0x9F1C;

// Sprite: Passenger Frames (2 Frames for each, hair up and hair down)
const uint32_t SPRITE_PASS_FRAMES = 0xA6EC;

// Table of smoke data from wheels
const uint32_t SMOKE_DATA = 0xACC6;

// Table of spray data from wheels
const uint32_t SPRAY_DATA = 0xAD06;

// Sprite: Shadow Data
const uint32_t SPRITE_SHADOW_DATA = 0x103B6;

// Sprite: Passenger Skid Frames
const uint32_t SPRITE_PASS1_SKIDL = 0x1107C;
const uint32_t SPRITE_PASS1_SKIDR = 0x110C2;
const uint32_t SPRITE_PASS2_SKIDL = 0x110CC;
const uint32_t SPRITE_PASS2_SKIDR = 0x11112;

// Long addresses of sprite data for hardware
const uint32_t SPRITE_TYPE_TABLE = 0x11ED2;

// Master Sprite Table
//
// Each one of the following addresses contains the following:
//
// [+0] Sprite Frequency Value Bitmask [Word]
// [+2] Reload Value For Sprite Info Offset [Word]
// [+4] Start of table with x,y,type,palette etc.
const uint32_t SPRITE_MASTER_TABLE = 0x1A43C;

// OutRun logo data
const uint32_t SPRITE_LOGO_BG = 0x11162;
const uint32_t SPRITE_LOGO_CAR = 0x1128E;
const uint32_t SPRITE_LOGO_BIRD1 = 0x112C0;
const uint32_t SPRITE_LOGO_BIRD2 = 0x112F2;
const uint32_t SPRITE_LOGO_BASE = 0x1125C;
const uint32_t SPRITE_LOGO_TEXT = 0x11194;

const uint32_t SPRITE_LOGO_PALM1 = 0x111C6;
const uint32_t SPRITE_LOGO_PALM2 = 0x111F8;
const uint32_t SPRITE_LOGO_PALM3 = 0x1122A;

// Music Selection Screen - Sprite Data
const uint32_t SPRITE_FM_LEFT   = 0x11892;
const uint32_t SPRITE_FM_CENTRE = 0x1189C;
const uint32_t SPRITE_FM_RIGHT  = 0x118A6;
const uint32_t SPRITE_DIAL_LEFT   = 0x118B0;
const uint32_t SPRITE_DIAL_CENTRE = 0x118BA;
const uint32_t SPRITE_DIAL_RIGHT  = 0x118C4;
const uint32_t SPRITE_EQ = 0x118CE; // EQ Sprite, Part of Radio, For Music Selection Screen
const uint32_t SPRITE_RADIO = 0x118D8;
const uint32_t SPRITE_HAND_LEFT   = 0x118E2;
const uint32_t SPRITE_HAND_CENTRE = 0x118EC;
const uint32_t SPRITE_HAND_RIGHT  = 0x118F6;

// Shadow data
const uint32_t SPRITE_SHDW_SMALL = 0x1193C;


// Sprite Collision X Offsets [Signed]
// Table is indexed with the type of sprite. 
//
// Format:
// Word 1: X-Left Offset
// Word 2: X-Right Offset
const uint32_t SPRITE_X_OFFS = 0x1212A;

// Sprite Zoom Lookup Table.
//
// Table Of Longs that represent X & Y Zoom Value
const uint32_t SPRITE_ZOOM_LOOKUP = 0x28000;

const uint32_t MOVEMENT_LOOKUP_Z = 0x30900;

// Table to alter sprite based on its y position.
//
// Input = Y Position
//
// Format:
//
// +0: Frame Number To Use
// +2: Entry In Zoom Lookup Table
const uint32_t MAP_Y_TO_FRAME = 0x30A00;

// ----------------------------------------------------------------------------
// Road
// ----------------------------------------------------------------------------

// Road Data Lookup Table
// Long Addresses where road data is looked up from.
// Note this only contains the road data itself. Namely the x position, and the length of each segment.
//
// The height and width are fetched from elsewhere.
// The sprites etc are fetched from elsewhere.
//
// Note: Although 1AD92 might not be used, this seems to be a section of road with road split / checkpoint sign
const uint32_t ROAD_DATA_LOOKUP = 0x1224;

// Lookup table of road height information.
// Each entry is a long address into this rom.
const uint32_t ROAD_HEIGHT_LOOKUP = 0x220A;

// Segment data for End Sequences (Master CPU Code)
const uint32_t ROAD_SEG_TABLE_END = 0xE514;

// Road Segment Table Information (Master CPU Code)
const uint32_t ROAD_SEG_TABLE = 0xE528;

// Data for Road Split
const uint32_t ROAD_DATA_SPLIT = 0x3A33E;

// Data for Road Bonus
const uint32_t ROAD_DATA_BONUS = 0x3ACA0;

// Data for Road Background Colour
const uint32_t ROAD_BGCOLOR = 0x109EE;

// Segment data for Road Split (Master CPU Code)
const uint32_t ROAD_DATA_SPLIT_SEGS = 0x1DFA4;


// ----------------------------------------------------------------------------
// Traffic Data
// ----------------------------------------------------------------------------

// Traffic Property Table
const uint32_t TRAFFIC_PROPS = 0x4CFA;

// There are six types of traffic in OutRun:
// Lorry, Pickup, Beetle, BMW, Corvette, Porsche.
//
// Each has 5 directional frames, including horizontal flipping.
// Some vehicles have different frames for inclines.
//
// Format is [0x20 byte boundaries]:
//
// [+0] Straight Frame
// [+4] Straight Frame (same as above)
// [+8] Right Frame
// [+C] Rightmost Frame 
//
// [+10] Straight Frame  [uphill version]
// [+14] Straight Frame  [uphill version] (same as above)
// [+18] Right Frame     [uphill version]
// [+1C] Rightmost Frame [uphill version]

const uint32_t TRAFFIC_DATA = 0x5424;

const uint32_t PORSCHE_SPRITE = 0xF290;

// ----------------------------------------------------------------------------
// Animation Sequences
// ----------------------------------------------------------------------------

// Flag Waver
const uint32_t ANIM_SEQ_FLAG = 0x12382;

// Ferrari Drive-In Sequence
const uint32_t ANIM_FERRARI_CURR = 0x12970;
const uint32_t ANIM_FERRARI_NEXT = 0x129C0;

const uint32_t ANIM_PASS1_CURR = 0x129C8;
const uint32_t ANIM_PASS1_NEXT = 0x12A18;

const uint32_t ANIM_PASS2_CURR = 0x12A20;
const uint32_t ANIM_PASS2_NEXT = 0x12A70;

// ----------------------------------------------------------------------------
// End Sequence / Bonus Mode
// ----------------------------------------------------------------------------

// Ferrari Sprite Frame Data For Bonus Mode
//
// +0 [Long]: Address of frame
// +4 [Byte]: Passenger Offset (always 0!)
// +5 [Byte]: Ferrari X Change
// +6 [Byte]: Sprite Colour Palette
// +7 [Byte]: H-Flip
const uint32_t ANIM_FERRARI_FRAMES = 0xA2F0;


// Note each table is used by every end sequence animation.
// And contains the variance for that particular animation.
// Ferrari Door Opening
const uint32_t ANIM_ENDSEQ_OBJ1 = 0x124B0;

// Ferrari Interior
const uint32_t ANIM_ENDSEQ_OBJ2 = 0x124D8;

// Car Shadow & Man Sprite
const uint32_t ANIM_ENDSEQ_OBJ3 = 0x12500;

// Man Shadow & Female Sprite
const uint32_t ANIM_ENDSEQ_OBJ4 = 0x12528;

// Female Shadow & Person Presenting Trophy
const uint32_t ANIM_ENDSEQ_OBJ5 = 0x12550;

const uint32_t ANIM_ENDSEQ_OBJ6 = 0x12578;

const uint32_t ANIM_ENDSEQ_OBJ7 = 0x125A0;

// Animation Data Sequence For Trophy Person
const uint32_t ANIM_ENDSEQ_OBJ8 = 0x125C8;

const uint32_t ANIM_ENDSEQ_OBJA = 0x125F0;

const uint32_t ANIM_ENDSEQ_OBJB = 0x12618;

// Start/End Animation Sequence Markers (Only End Markers Appear To Be Used)
const uint32_t ANIM_END_TABLE = 0x123A2;

// ----------------------------------------------------------------------------
// Course Map Data
// ----------------------------------------------------------------------------

// Course Map Sprite Data
const uint32_t SPRITE_COURSEMAP = 0x26BE;

// Coursemap Sprite Components
const uint32_t SPRITE_COURSEMAP_TOP = 0x3784;
const uint32_t SPRITE_COURSEMAP_BOT = 0x386C;
const uint32_t SPRITE_COURSEMAP_END = 0x3954;

// Minicar
const uint32_t SPRITE_MINICAR_RIGHT = 0x10C58;
const uint32_t SPRITE_MINICAR_UP = 0x10C62;
const uint32_t SPRITE_MINICAR_DOWN = 0x10C6C;

// Convert route information to piece index to colour
const uint32_t MAP_ROUTE_LOOKUP = 0x3636;

// X and Y Movement table for Minicar
const uint32_t MAP_MOVEMENT_LEFT = 0x3A34;
const uint32_t MAP_MOVEMENT_RIGHT = 0x3AB4;

// ----------------------------------------------------------------------------
// Best OutRunners Data
// ----------------------------------------------------------------------------

// Tiles for minicars that run across the text layer
const uint32_t TILES_MINICARS1 = 0xD62A;
const uint32_t TILES_MINICARS2 = 0xD670; // smoke tiles

// Default Score Data
const uint32_t DEFAULT_SCORES  = 0xD676;

// Alphabet characters for initial entry
const uint32_t TILES_ALPHABET  = 0xD5A4;

// ----------------------------------------------------------------------------
// Music Selection Data
// ----------------------------------------------------------------------------

// Palette to cycle the graphical equalizer on the radio
const uint32_t MUSIC_EQ_PAL = 0xCCAA;

// ----------------------------------------------------------------------------
// Data
// ----------------------------------------------------------------------------

// X,Y Offsets of passenger 1 (Man) from car
//
// Format 
//
// Word 1: Frame 1 X Offset
// Word 2: Frame 1 Y Offset 
// Word 3: Frame 1 X Offset H-FLIP
// Word 4: Frame 1 Y Offset H-FLIP
//
// Word 5: Frame 2 X Offset
// Word 6: Frame 2 Y Offset
//
// etc.
const uint32_t PASS1_OFFSET = 0xA6FC;
const uint32_t PASS2_OFFSET = 0xA75C;

// Map stage ID to number 0-14
const uint32_t MAP_STAGE_ID = 0xDDB4;

// Width / Height Table for sprites
const uint32_t WH_TABLE = 0x20000;

// Movement Lookup Table
// Used by:
// - Logo Bird Animation In Attract Mode
// - Car Flip In Crash Scenario
//
// Entries 
// 0     - 0xFF : X Position
// 0x100 - 0x1FF: Y Position
const uint32_t DATA_MOVEMENT = 0x30800;