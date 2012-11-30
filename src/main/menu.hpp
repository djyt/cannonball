#pragma once

#include <vector>

#include "stdint.hpp"
#include "engine/osprites.hpp"
#include "engine/ologo.hpp"

class Menu
{
public:
    Menu(void);
    ~Menu(void);

    void init();
    void tick();
    void draw_menu_options();

private:
    std::vector<std::string> menu_main;
};

