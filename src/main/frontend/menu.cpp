#include "menu.hpp"


Menu::Menu(void)
{
}


Menu::~Menu(void)
{
}

void Menu::init()
{
    otiles.setup_palette_default();
    video.clear_text_ram();
    osprites.disable_sprites();
    ologo.enable(-60);

    // Create Menus
    menu_main.push_back("PLAY GAME");
    menu_main.push_back("SETTINGS");
    menu_main.push_back("EXIT");
}

void Menu::tick()
{
    // Do Text
    draw_menu_options();
    //
    
    // Do Animated Logo
    ologo.tick();
    osprites.sprite_copy();

	if (osprites.do_sprite_swap)
	{
		osprites.do_sprite_swap = false;
		video.sprite_layer->swap();
		// Do Sprite RAM Swap and copy new palette data if necessary
		osprites.copy_palette_data();
	}
}

void Menu::draw_menu_options()
{
    int8_t x = 0;
    int8_t y = 20;

    for (int i = 0; i < (int) menu_main.size(); i++)
    {
        // Centre the menu option
        x = 20 - (menu_main[i].length() >> 1);
        ohud.blit_text_new(x, y, menu_main[i].c_str());
        y += 2;
    }
}

// ------------------------------------------------------------------------------------------------
// Menu Options
// ------------------------------------------------------------------------------------------------


