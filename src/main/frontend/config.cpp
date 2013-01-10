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
#include "engine/audio/osoundint.hpp"

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

    // ------------------------------------------------------------------------
    // Menu Settings
    // ------------------------------------------------------------------------

    menu.enabled           = pt_config.get("menu.enabled",   1);
    menu.road_scroll_speed = pt_config.get("menu.roadspeed", 50);

    // ------------------------------------------------------------------------
    // Video Settings
    // ------------------------------------------------------------------------
   
    video.mode       = pt_config.get("video.mode",               0); // Video Mode: Default is Windowed 
    video.scale      = pt_config.get("video.window.scale",       2); // Video Scale: Default is 2x    
    video.stretch    = pt_config.get("video.fullscreen.stretch", 0); // Stretch in full-screen mode
    video.scanlines  = pt_config.get("video.scanlines",          0); // Scanlines
    video.fps        = pt_config.get("video.fps",                2); // Default is 60 fps   
    video.widescreen = pt_config.get("video.widescreen",         1); // Enable Widescreen Mode   
          
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
    sound.enabled   = pt_config.get("sound.enable",    1);
    sound.advertise = pt_config.get("sound.advertise", 1);

    // Custom Music
    for (int i = 0; i < 4; i++)
    {
        std::string xmltag = "sound.custom_music.track";
        xmltag += to_string(i+1);  

        sound.custom_music[i].enabled = pt_config.get(xmltag + ".<xmlattr>.enabled", 0);
        sound.custom_music[i].title   = pt_config.get(xmltag + ".title", "TRACK " +to_string(i+1));
        sound.custom_music[i].filename= pt_config.get(xmltag + ".filename", "track"+to_string(i+1)+".wav");
    }

    // ------------------------------------------------------------------------
    // Controls
    // ------------------------------------------------------------------------
    controls.gear          = pt_config.get("controls.gear", 0);
    controls.steer_speed   = pt_config.get("controls.steerspeed", 3);
    controls.pedal_speed   = pt_config.get("controls.pedalspeed", 4);
    controls.keyconfig[0]  = pt_config.get("controls.keyconfig.up",    273);
    controls.keyconfig[1]  = pt_config.get("controls.keyconfig.down",  274);
    controls.keyconfig[2]  = pt_config.get("controls.keyconfig.left",  276);
    controls.keyconfig[3]  = pt_config.get("controls.keyconfig.right", 275);
    controls.keyconfig[4]  = pt_config.get("controls.keyconfig.acc",   122);
    controls.keyconfig[5]  = pt_config.get("controls.keyconfig.brake", 120);
    controls.keyconfig[6]  = pt_config.get("controls.keyconfig.gear",  32);
    controls.keyconfig[7]  = pt_config.get("controls.keyconfig.start", 49);
    controls.keyconfig[8]  = pt_config.get("controls.keyconfig.coin",  53);
    controls.keyconfig[9]  = pt_config.get("controls.keyconfig.menu",  286);
    controls.padconfig[0]  = pt_config.get("controls.padconfig.acc", 0);
    controls.padconfig[1]  = pt_config.get("controls.padconfig.brake", 1);
    controls.padconfig[2]  = pt_config.get("controls.padconfig.gear", 2);
    controls.padconfig[3]  = pt_config.get("controls.padconfig.start", 3);
    controls.padconfig[4]  = pt_config.get("controls.padconfig.coin", 4);
    controls.padconfig[5]  = pt_config.get("controls.padconfig.menu", 5);

    // ------------------------------------------------------------------------
    // Engine Settings
    // ------------------------------------------------------------------------

    engine.dip_time      = pt_config.get("engine.time",    0);
    engine.dip_traffic   = pt_config.get("engine.traffic", 1);
    
    engine.freeze_timer    = engine.dip_time == 4;
    engine.disable_traffic = engine.dip_traffic == 4;
    engine.dip_time    &= 3;
    engine.dip_traffic &= 3;

    engine.jap           = pt_config.get("engine.japanese_tracks", 0);
    
    // Additional Level Objects
    engine.level_objects = pt_config.get("engine.levelobjects", 1);
    engine.randomgen     = pt_config.get("engine.randomgen",    1);

    // ------------------------------------------------------------------------
    // Time Trial Mode
    // ------------------------------------------------------------------------

    ttrial.laps    = pt_config.get("time_trial.laps",    5);
    ttrial.traffic = pt_config.get("time_trial.traffic", 3);
}

bool Config::save(const std::string &filename)
{
    // Save stuff
    pt_config.put("video.fps",                video.fps);
    pt_config.put("video.widescreen",         video.widescreen);
    pt_config.put("video.mode",               video.mode);
    pt_config.put("video.window.scale",       video.scale);
    pt_config.put("video.fullscreen.stretch", video.stretch);

    pt_config.put("sound.enable",    sound.enabled);
    pt_config.put("sound.advertise", sound.advertise);

    pt_config.put("controls.gear",            controls.gear);
    pt_config.put("controls.steerspeed",      controls.steer_speed);
    pt_config.put("controls.pedalspeed",      controls.pedal_speed);
    pt_config.put("controls.keyconfig.up",    controls.keyconfig[0]);
    pt_config.put("controls.keyconfig.down",  controls.keyconfig[1]);
    pt_config.put("controls.keyconfig.left",  controls.keyconfig[2]);
    pt_config.put("controls.keyconfig.right", controls.keyconfig[3]);
    pt_config.put("controls.keyconfig.acc",   controls.keyconfig[4]);
    pt_config.put("controls.keyconfig.brake", controls.keyconfig[5]);
    pt_config.put("controls.keyconfig.gear",  controls.keyconfig[6]);
    pt_config.put("controls.keyconfig.start", controls.keyconfig[7]);
    pt_config.put("controls.keyconfig.coin",  controls.keyconfig[8]);
    pt_config.put("controls.keyconfig.menu",  controls.keyconfig[9]);
    pt_config.put("controls.padconfig.acc",   controls.padconfig[0]);
    pt_config.put("controls.padconfig.brake", controls.padconfig[1]);
    pt_config.put("controls.padconfig.gear",  controls.padconfig[2]);
    pt_config.put("controls.padconfig.start", controls.padconfig[3]);
    pt_config.put("controls.padconfig.coin",  controls.padconfig[4]);
    pt_config.put("controls.padconfig.menu",  controls.padconfig[5]);

    pt_config.put("engine.time", engine.freeze_timer ? 4 : engine.dip_time);
    pt_config.put("engine.traffic", engine.disable_traffic ? 4 : engine.dip_traffic);
    pt_config.put("engine.japanese_tracks", engine.jap);
    pt_config.put("engine.levelobjects", engine.level_objects);

    pt_config.put("time_trial.laps",    ttrial.laps);
    pt_config.put("time_trial.traffic", ttrial.traffic);

    ttrial.laps    = pt_config.get("time_trial.laps",    5);
    ttrial.traffic = pt_config.get("time_trial.traffic", 3);


    // Tab space 1
    boost::property_tree::xml_writer_settings<char> settings('\t', 1);

    try
    {
        write_xml(filename, pt_config, std::locale(), settings);
    }
    catch (std::exception &e)
    {
        std::cout << "Error saving config: " << e.what() << "\n";
        return false;
    }
    return true;
}

std::string filename_scores     = "hiscores.xml";
std::string filename_scores_jap = "hiscores_jap.xml";

void Config::load_scores()
{
    // Create empty property tree object
    ptree pt;

    try
    {
        read_xml(engine.jap ? filename_scores_jap : filename_scores, pt, boost::property_tree::xml_parser::trim_whitespace);
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
        write_xml(engine.jap ? filename_scores_jap : filename_scores, pt, std::locale(), settings);
    }
    catch (std::exception &e)
    {
        std::cout << "Error saving hiscores: " << e.what() << "\n";
    }
}

bool Config::clear_scores()
{
    // Init Default Hiscores
    ohiscore.init_def_scores();
    // Remove hiscore xml file if it exists
    return remove(engine.jap ? filename_scores_jap.c_str() : filename_scores.c_str()) == 0; 
}

void Config::set_fps(int fps)
{
    video.fps = fps;
    // Set core FPS to 30fps or 60fps
    this->fps = video.fps == 0 ? 30 : 60;
    
    // Original game ticks sprites at 30fps but background scroll at 60fps
    tick_fps  = video.fps < 2 ? 30 : 60;

    cannonball::frame_ms = (1000 / this->fps);

    #ifdef COMPILE_SOUND_CODE
    if (sound.enabled)
        cannonball::audio.stop_audio();
    osoundint.init();
    if (sound.enabled)
        cannonball::audio.start_audio();
    #endif
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