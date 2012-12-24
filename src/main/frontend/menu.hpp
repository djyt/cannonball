/***************************************************************************
    Front End Menu System.

    This file is part of Cannonball. 
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include <vector>
#include "stdint.hpp"

class Menu
{
public:
    Menu(void);
    ~Menu(void);

    void init();
    void tick();
    void draw_menu_options();

private:
    uint32_t frame;

    // Cursor
    int16_t cursor;

    // Stores whether this is a textual menu (i.e. no options that can be chosen)
    bool is_text_menu;

    // Determine whether we should save the settings file
    bool save_settings;

    // Used to control the horizon pan effect
    uint16_t horizon_pos;

    std::vector<std::string>* menu_selected;
    std::vector<std::string> menu_main;
    std::vector<std::string> menu_about;
    std::vector<std::string> menu_settings;
    std::vector<std::string> menu_video;
    std::vector<std::string> menu_sound;
    std::vector<std::string> menu_controls;
    std::vector<std::string> menu_engine;
    std::vector<std::string> menu_clearscores;
    

    void tick_menu();
    void set_menu(std::vector<std::string>*);
    void refresh_menu();
    void set_menu_text(std::string s1, std::string s2);
};