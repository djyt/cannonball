/***************************************************************************
    Music Selection Screen.

    This is a combination of a tilemap and overlayed sprites.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "outrun.hpp"

class RomLoader;

class OMusic
{
public:
    OMusic(void);
    ~OMusic(void);

    bool load_widescreen_map(std::string path);
    void enable();
    void disable();
    void tick();
    void blit();
    void check_start();
    void play_music(int index = -1);
    void cycle_music();

private:
    // Modified Widescreen version of the Music Select Tilemap
    RomLoader* tilemap;
    // Additional Widescreen tiles
    RomLoader* tile_patch;

    // Next track to play
    music_t* next_track;

    // Music Track Selected By Player
    uint8_t music_selected;

    // Total tracks to include in music select (> 3 means user has added extra ones)
    int total_tracks;

    // Enahcned: Current Cursor Position
    int cursor_pos;

    uint16_t entry_start;

    // Used to preview music track
    int16_t last_music_selected;
    int8_t preview_counter;

    const static short HAND_LEFT = 0, HAND_CENTRE = 1, HAND_RIGHT = 2;
    
	void setup_sprite1();
	void setup_sprite2();
	void setup_sprite3();
	void setup_sprite4();
	void setup_sprite5();
    void tick_original(oentry*, oentry*, oentry*);
    void tick_enhanced(oentry*, oentry*, oentry*);
    void set_hand(short, oentry*, oentry*, oentry*);
    void blit_music_select();
};

extern OMusic omusic;

