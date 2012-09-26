#pragma once

#include "outrun.hpp"
#include "oanimsprite.hpp"

class OAnimSeq
{
public:
    // Man at line with start flag
    oanimsprite anim_flag;

    // Ferrari Animation Sequence
    oanimsprite anim_ferrari;               // 1

    // Passenger Animation Sequences
    oanimsprite anim_pass1;                 // 2
    oanimsprite anim_pass2;                 // 3

    // End Sequence Stuff
    oanimsprite anim_obj1;                  // 4
    oanimsprite anim_obj2;                  // 5
    oanimsprite anim_obj3;                  // 6
    oanimsprite anim_obj4;                  // 7
    oanimsprite anim_obj5;                  // 8
    oanimsprite anim_obj6;                  // 9
    oanimsprite anim_obj7;                  // 10
    oanimsprite anim_obj8;                  // 10

    // End sequence to display (0-4)
    uint8_t end_seq;

    OAnimSeq(void);
    ~OAnimSeq(void);

    //void init(oentry*, oentry*, oentry*, oentry*);
    void init(oentry*);
    void flag_seq();
    void ferrari_seq();
    void anim_seq_intro(oanimsprite*);
    void init_end_seq();
    void tick_end_seq();

private:
    // End Sequence Animation Position
    int16_t seq_pos;

    // End Sequence State (0 = Init, 1 = Tick)
    uint8_t end_seq_state;

    // Used for Ferrari End Animation Sequence
    bool ferrari_stopped;

    void init_end_sprites();
    void tick_ferrari();
    void anim_seq_outro(oanimsprite*);
    void anim_seq_shadow(oanimsprite*, oanimsprite*);
    void anim_seq_outro_ferrari();
    bool read_anim_data(oanimsprite*);
};

extern OAnimSeq oanimseq;
