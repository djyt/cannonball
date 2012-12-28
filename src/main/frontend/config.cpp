/***************************************************************************
    XML Configuration File Handling.

    Load Settings.
    Load & Save Hi-Scores.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

// see: http://www.boost.org/doc/libs/1_52_0/doc/html/boost_propertytree/tutorial.html

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <iostream>

#include "main.hpp"
#include "config.hpp"
#include "globals.hpp"

#include "engine/ohiscore.hpp"

Config config;

Config::Config(void)
{
}


Config::~Config(void)
{
}

void Config::init()
{

}

using boost::property_tree::ptree;
ptree pt_config;

void Config::load(const std::string &filename)
{
    // Load XML file and put its contents in property tree. 
    // No namespace qualification is needed, because of Koenig 
    // lookup on the second argument. If reading fails, exception
    // is thrown.
    try
    {
        read_xml(filename, pt_config, boost::property_tree::xml_parser::trim_whitespace);
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }

    use_menu = pt_config.get("menu", 1);

    // ------------------------------------------------------------------------
    // Video Settings
    // ------------------------------------------------------------------------

    // Default is 60 fps
    video.fps        = pt_config.get("video.fps", 2);
    // Enable Widescreen Mode
    video.widescreen = pt_config.get("video.widescreen", 0);
    // Video Mode: Default is Windowed
    video.mode       = pt_config.get("video.mode", 0);
    // Video Scale: Default is 1x
    video.scale      = pt_config.get("video.window.scale", 1);
    // Stretch in full-screen mode
    video.stretch    = pt_config.get("video.fullscreen.stretch", 0);
          
    set_fps(video.fps);

    if (video.widescreen)
    {
        s16_width = S16_WIDTH_WIDE;
        s16_x_off = (S16_WIDTH_WIDE - S16_WIDTH) / 2;
    }
    else
    {
        s16_width = S16_WIDTH;
        s16_x_off = 0;
    }

    // ------------------------------------------------------------------------
    // Sound Settings
    // ------------------------------------------------------------------------
    sound.enabled = pt_config.get("sound.enable", 1);
    sound.advertise = pt_config.get("sound.advertise", 1);

    // ------------------------------------------------------------------------
    // Controls
    // ------------------------------------------------------------------------
    controls.gear          = pt_config.get("controls.gear", 0);
    controls.steer_speed   = pt_config.get("controls.steerspeed", 3);
    controls.pedal_speed   = pt_config.get("controls.pedalspeed", 4);

    // ------------------------------------------------------------------------
    // Engine Settings
    // ------------------------------------------------------------------------

    engine.dip_time      = pt_config.get("engine.time", 0);
    engine.dip_traffic   = pt_config.get("engine.traffic", 1);
    
    engine.freeze_timer    = engine.dip_time == 4;
    engine.disable_traffic = engine.dip_traffic == 4;
    engine.dip_time    &= 3;
    engine.dip_traffic &= 3;
    
    // Additional Level Objects
    engine.level_objects = pt_config.get("engine.levelobjects", 1);
}

void Config::save(const std::string &filename)
{
    // Save stuff
    pt_config.put("video.fps", video.fps);
    pt_config.put("video.widescreen", video.widescreen);
    pt_config.put("video.mode", video.mode);
    pt_config.put("video.window.scale", video.scale);
    pt_config.put("video.fullscreen.stretch", video.stretch);

    pt_config.put("sound.enable", sound.enabled);
    pt_config.put("sound.advertise", sound.advertise);

    pt_config.put("controls.gear", controls.gear);
    pt_config.put("controls.steerspeed", controls.steer_speed);
    pt_config.put("controls.pedalspeed", controls.pedal_speed);

    pt_config.put("engine.time", engine.freeze_timer ? 4 : engine.dip_time);
    pt_config.put("engine.traffic", engine.disable_traffic ? 4 : engine.dip_traffic);
    pt_config.put("engine.levelobjects", engine.level_objects);

    // Tab space 1
    boost::property_tree::xml_writer_settings<char> settings('\t', 1);

    try
    {
        write_xml(filename, pt_config, std::locale(), settings);
    }
    catch (std::exception &e)
    {
        std::cout << "Error saving config: " << e.what() << "\n";
    }
}

std::string filename_scores = "hiscores.xml";

void Config::load_scores()
{
    // Create empty property tree object
    ptree pt;

    try
    {
        read_xml(filename_scores, pt, boost::property_tree::xml_parser::trim_whitespace);
    }
    catch (std::exception &e)
    {
        e.what();
        return;
    }
    
    for (int i = 0; i < ohiscore.NO_SCORES; i++)
    {
        score_entry* e = &ohiscore.scores[i];
        
        std::string xmltag = "score";
        xmltag += to_string(i);  
    
        e->score    = from_hex_string(pt.get<std::string>(xmltag + ".score",    "0"));
        e->initial1 = pt.get(xmltag + ".initial1", ".")[0];
        e->initial2 = pt.get(xmltag + ".initial2", ".")[0];
        e->initial3 = pt.get(xmltag + ".initial3", ".")[0];
        e->maptiles = from_hex_string(pt.get<std::string>(xmltag + ".maptiles", "20202020"));
        e->time     = from_hex_string(pt.get<std::string>(xmltag + ".time"    , "0")); 

        if (e->initial1 == '.') e->initial1 = 0x20;
        if (e->initial2 == '.') e->initial2 = 0x20;
        if (e->initial3 == '.') e->initial3 = 0x20;
    }
}

void Config::save_scores()
{
    // Create empty property tree object
    ptree pt;
        
    for (int i = 0; i < ohiscore.NO_SCORES; i++)
    {
        score_entry* e = &ohiscore.scores[i];
    
        std::string xmltag = "score";
        xmltag += to_string(i);    
        
        pt.put(xmltag + ".score",    to_hex_string(e->score));
        pt.put(xmltag + ".initial1", e->initial1 == 0x20 ? "." : to_string(e->initial1)); // use . to represent space
        pt.put(xmltag + ".initial2", e->initial2 == 0x20 ? "." : to_string(e->initial2));
        pt.put(xmltag + ".initial3", e->initial3 == 0x20 ? "." : to_string(e->initial3));
        pt.put(xmltag + ".maptiles", to_hex_string(e->maptiles));
        pt.put(xmltag + ".time",     to_hex_string(e->time));
    }
    
    // Tab space 1
    boost::property_tree::xml_writer_settings<char> settings('\t', 1);
    
    try
    {
        write_xml(filename_scores, pt, std::locale(), settings);
    }
    catch (std::exception &e)
    {
        std::cout << "Error saving hiscores: " << e.what() << "\n";
    }
}

void Config::clear_scores()
{
    ohiscore.init_def_scores();      // Init Default Hiscores
    remove(filename_scores.c_str()); // Remove hiscore xml file if it exists
}

void Config::set_fps(int fps)
{
    video.fps = fps;
    // Set core FPS to 30fps or 60fps
    this->fps = video.fps == 0 ? 30 : 60;
    
    // Original game ticks sprites at 30fps but background scroll at 60fps
    tick_fps  = video.fps < 2 ? 30 : 60;

    cannonball::frame_ms = (1000 / this->fps);
}

// Convert value to string
template<class T>
std::string Config::to_string(T i)
{
    std::stringstream ss;
    ss << i;
    return ss.str();
}

// Convert value to string
template<class T>
std::string Config::to_hex_string(T i)
{
    std::stringstream ss;
    ss << std::hex << i;
    return ss.str();
}

// Convert hex string to unsigned int
uint32_t Config::from_hex_string(std::string s)
{
    unsigned int x;   
    std::stringstream ss;
    ss << std::hex << s;
    ss >> x;
    // output it as a signed type
    return static_cast<unsigned int>(x);
}