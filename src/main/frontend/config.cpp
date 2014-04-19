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
#include "setup.hpp"
#include "../utils.hpp"

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
    video.scanlines  = pt_config.get("video.scanlines",          0); // Scanlines
    video.fps        = pt_config.get("video.fps",                2); // Default is 60 fps
    video.fps_count  = pt_config.get("video.fps_counter",        0); // FPS Counter
    video.widescreen = pt_config.get("video.widescreen",         1); // Enable Widescreen Mode
    video.hires      = pt_config.get("video.hires",              0); // Hi-Resolution Mode
    video.filtering  = pt_config.get("video.filtering",          0); // Open GL Filtering Mode
          
    set_fps(video.fps);

    // ------------------------------------------------------------------------
    // Sound Settings
    // ------------------------------------------------------------------------
    sound.enabled     = pt_config.get("sound.enable",      1);
    sound.advertise   = pt_config.get("sound.advertise",   1);
    sound.preview     = pt_config.get("sound.preview",     1);
    sound.fix_samples = pt_config.get("sound.fix_samples", 1);

    // Custom Music
    for (int i = 0; i < 4; i++)
    {
        std::string xmltag = "sound.custom_music.track";
        xmltag += Utils::to_string(i+1);  

        sound.custom_music[i].enabled = pt_config.get(xmltag + ".<xmlattr>.enabled", 0);
        sound.custom_music[i].title   = pt_config.get(xmltag + ".title", "TRACK " +Utils::to_string(i+1));
        sound.custom_music[i].filename= pt_config.get(xmltag + ".filename", "track"+Utils::to_string(i+1)+".wav");
    }

    // ------------------------------------------------------------------------
    // CannonBoard Settings
    // ------------------------------------------------------------------------
    cannonboard.enabled = pt_config.get("cannonboard.<xmlattr>.enabled", 0);
    cannonboard.port    = pt_config.get("cannonboard.port", "COM6");
    cannonboard.baud    = pt_config.get("cannonboard.baud", 57600);
    cannonboard.debug   = pt_config.get("cannonboard.debug", 0);
    cannonboard.cabinet = pt_config.get("cannonboard.cabinet", 0);

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
    controls.keyconfig[6]  = pt_config.get("controls.keyconfig.gear1", 32);
    controls.keyconfig[7]  = pt_config.get("controls.keyconfig.gear2", 32);
    controls.keyconfig[8]  = pt_config.get("controls.keyconfig.start", 49);
    controls.keyconfig[9]  = pt_config.get("controls.keyconfig.coin",  53);
    controls.keyconfig[10] = pt_config.get("controls.keyconfig.menu",  286);
    controls.keyconfig[11] = pt_config.get("controls.keyconfig.view",  304);
    controls.padconfig[0]  = pt_config.get("controls.padconfig.acc",   0);
    controls.padconfig[1]  = pt_config.get("controls.padconfig.brake", 1);
    controls.padconfig[2]  = pt_config.get("controls.padconfig.gear1", 2);
    controls.padconfig[3]  = pt_config.get("controls.padconfig.gear2", 2);
    controls.padconfig[4]  = pt_config.get("controls.padconfig.start", 3);
    controls.padconfig[5]  = pt_config.get("controls.padconfig.coin",  4);
    controls.padconfig[6]  = pt_config.get("controls.padconfig.menu",  5);
    controls.padconfig[7]  = pt_config.get("controls.padconfig.view",  6);
    controls.analog        = pt_config.get("controls.analog.<xmlattr>.enabled", 0);
    controls.pad_id        = pt_config.get("controls.pad_id", 0);
    controls.axis[0]       = pt_config.get("controls.analog.axis.wheel", 0);
    controls.axis[1]       = pt_config.get("controls.analog.axis.accel", 2);
    controls.axis[2]       = pt_config.get("controls.analog.axis.brake", 3);
    controls.asettings[0]  = pt_config.get("controls.analog.wheel.zone", 75);
    controls.asettings[1]  = pt_config.get("controls.analog.wheel.dead", 0);
    controls.asettings[2]  = pt_config.get("controls.analog.pedals.dead", 0);
    
    controls.haptic        = pt_config.get("controls.analog.haptic.<xmlattr>.enabled", 0);
    controls.max_force     = pt_config.get("controls.analog.haptic.max_force", 9000);
    controls.min_force     = pt_config.get("controls.analog.haptic.min_force", 8500);
    controls.force_duration= pt_config.get("controls.analog.haptic.force_duration", 20);

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
    engine.prototype     = pt_config.get("engine.prototype",       0);
    
    // Additional Level Objects
    engine.level_objects   = pt_config.get("engine.levelobjects", 1);
    engine.randomgen       = pt_config.get("engine.randomgen",    1);
    engine.fix_bugs_backup = 
    engine.fix_bugs        = pt_config.get("engine.fix_bugs",     1) != 0;
    engine.layout_debug    = pt_config.get("engine.layout_debug", 0) != 0;
    engine.new_attract     = pt_config.get("engine.new_attract", 1) != 0;

    // ------------------------------------------------------------------------
    // Time Trial Mode
    // ------------------------------------------------------------------------

    ttrial.laps    = pt_config.get("time_trial.laps",    5);
    ttrial.traffic = pt_config.get("time_trial.traffic", 3);

    cont_traffic   = pt_config.get("continuous.traffic", 3);
}

bool Config::save(const std::string &filename)
{
    // Save stuff
    pt_config.put("video.mode",               video.mode);
    pt_config.put("video.window.scale",       video.scale);
    pt_config.put("video.scanlines",          video.scanlines);
    pt_config.put("video.fps",                video.fps);
    pt_config.put("video.widescreen",         video.widescreen);
    pt_config.put("video.hires",              video.hires);

    pt_config.put("sound.enable",             sound.enabled);
    pt_config.put("sound.advertise",          sound.advertise);
    pt_config.put("sound.preview",            sound.preview);
    pt_config.put("sound.fix_samples",        sound.fix_samples);

    pt_config.put("controls.gear",            controls.gear);
    pt_config.put("controls.steerspeed",      controls.steer_speed);
    pt_config.put("controls.pedalspeed",      controls.pedal_speed);
    pt_config.put("controls.keyconfig.up",    controls.keyconfig[0]);
    pt_config.put("controls.keyconfig.down",  controls.keyconfig[1]);
    pt_config.put("controls.keyconfig.left",  controls.keyconfig[2]);
    pt_config.put("controls.keyconfig.right", controls.keyconfig[3]);
    pt_config.put("controls.keyconfig.acc",   controls.keyconfig[4]);
    pt_config.put("controls.keyconfig.brake", controls.keyconfig[5]);
    pt_config.put("controls.keyconfig.gear1", controls.keyconfig[6]);
    pt_config.put("controls.keyconfig.gear2", controls.keyconfig[7]);
    pt_config.put("controls.keyconfig.start", controls.keyconfig[8]);
    pt_config.put("controls.keyconfig.coin",  controls.keyconfig[9]);
    pt_config.put("controls.keyconfig.menu",  controls.keyconfig[10]);
    pt_config.put("controls.keyconfig.view",  controls.keyconfig[11]);
    pt_config.put("controls.padconfig.acc",   controls.padconfig[0]);
    pt_config.put("controls.padconfig.brake", controls.padconfig[1]);
    pt_config.put("controls.padconfig.gear1", controls.padconfig[2]);
    pt_config.put("controls.padconfig.gear2", controls.padconfig[3]);
    pt_config.put("controls.padconfig.start", controls.padconfig[4]);
    pt_config.put("controls.padconfig.coin",  controls.padconfig[5]);
    pt_config.put("controls.padconfig.menu",  controls.padconfig[6]);
    pt_config.put("controls.padconfig.view",  controls.padconfig[7]);
    pt_config.put("controls.analog.<xmlattr>.enabled", controls.analog);

    pt_config.put("engine.time", engine.freeze_timer ? 4 : engine.dip_time);
    pt_config.put("engine.traffic", engine.disable_traffic ? 4 : engine.dip_traffic);
    pt_config.put("engine.japanese_tracks", engine.jap);
    pt_config.put("engine.prototype", engine.prototype);
    pt_config.put("engine.levelobjects", engine.level_objects);
    pt_config.put("engine.new_attract", engine.new_attract);

    pt_config.put("time_trial.laps",    ttrial.laps);
    pt_config.put("time_trial.traffic", ttrial.traffic);
    pt_config.put("continuous.traffic", cont_traffic), 

    ttrial.laps    = pt_config.get("time_trial.laps",    5);
    ttrial.traffic = pt_config.get("time_trial.traffic", 3);
    cont_traffic   = pt_config.get("continuous.traffic", 3);


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

void Config::load_scores(const std::string &filename)
{
    // Create empty property tree object
    ptree pt;

    try
    {
        read_xml(engine.jap ? filename + "_jap.xml" : filename + ".xml" , pt, boost::property_tree::xml_parser::trim_whitespace);
    }
    catch (std::exception &e)
    {
        e.what();
        return;
    }
    
    // Game Scores
    for (int i = 0; i < ohiscore.NO_SCORES; i++)
    {
        score_entry* e = &ohiscore.scores[i];
        
        std::string xmltag = "score";
        xmltag += Utils::to_string(i);  
    
        e->score    = Utils::from_hex_string(pt.get<std::string>(xmltag + ".score",    "0"));
        e->initial1 = pt.get(xmltag + ".initial1", ".")[0];
        e->initial2 = pt.get(xmltag + ".initial2", ".")[0];
        e->initial3 = pt.get(xmltag + ".initial3", ".")[0];
        e->maptiles = Utils::from_hex_string(pt.get<std::string>(xmltag + ".maptiles", "20202020"));
        e->time     = Utils::from_hex_string(pt.get<std::string>(xmltag + ".time"    , "0")); 

        if (e->initial1 == '.') e->initial1 = 0x20;
        if (e->initial2 == '.') e->initial2 = 0x20;
        if (e->initial3 == '.') e->initial3 = 0x20;
    }
}

void Config::save_scores(const std::string &filename)
{
    // Create empty property tree object
    ptree pt;
        
    for (int i = 0; i < ohiscore.NO_SCORES; i++)
    {
        score_entry* e = &ohiscore.scores[i];
    
        std::string xmltag = "score";
        xmltag += Utils::to_string(i);    
        
        pt.put(xmltag + ".score",    Utils::to_hex_string(e->score));
        pt.put(xmltag + ".initial1", e->initial1 == 0x20 ? "." : Utils::to_string((char) e->initial1)); // use . to represent space
        pt.put(xmltag + ".initial2", e->initial2 == 0x20 ? "." : Utils::to_string((char) e->initial2));
        pt.put(xmltag + ".initial3", e->initial3 == 0x20 ? "." : Utils::to_string((char) e->initial3));
        pt.put(xmltag + ".maptiles", Utils::to_hex_string(e->maptiles));
        pt.put(xmltag + ".time",     Utils::to_hex_string(e->time));
    }
    
    // Tab space 1
    boost::property_tree::xml_writer_settings<char> settings('\t', 1);
    
    try
    {
        write_xml(engine.jap ? filename + "_jap.xml" : filename + ".xml", pt, std::locale(), settings);
    }
    catch (std::exception &e)
    {
        std::cout << "Error saving hiscores: " << e.what() << "\n";
    }
}

void Config::load_tiletrial_scores()
{
    const std::string filename = FILENAME_TTRIAL;

    // Counter value that represents 1m 15s 0ms
    static const uint16_t COUNTER_1M_15 = 0x11D0;

    // Create empty property tree object
    ptree pt;

    try
    {
        read_xml(engine.jap ? filename + "_jap.xml" : filename + ".xml" , pt, boost::property_tree::xml_parser::trim_whitespace);
    }
    catch (std::exception &e)
    {
        for (int i = 0; i < 15; i++)
            ttrial.best_times[i] = COUNTER_1M_15;

        e.what();
        return;
    }

    // Time Trial Scores
    for (int i = 0; i < 15; i++)
    {
        ttrial.best_times[i] = pt.get("time_trial.score" + Utils::to_string(i), COUNTER_1M_15);
    }
}

void Config::save_tiletrial_scores()
{
    const std::string filename = FILENAME_TTRIAL;

    // Create empty property tree object
    ptree pt;

    // Time Trial Scores
    for (int i = 0; i < 15; i++)
    {
        pt.put("time_trial.score" + Utils::to_string(i), ttrial.best_times[i]);
    }

    // Tab space 1
    boost::property_tree::xml_writer_settings<char> settings('\t', 1);
    
    try
    {
        write_xml(engine.jap ? filename + "_jap.xml" : filename + ".xml", pt, std::locale(), settings);
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

    int clear = 0;

    // Remove XML files if they exist
    clear += remove(std::string(FILENAME_SCORES).append(".xml").c_str());
    clear += remove(std::string(FILENAME_SCORES).append("_jap.xml").c_str());
    clear += remove(std::string(FILENAME_TTRIAL).append(".xml").c_str());
    clear += remove(std::string(FILENAME_TTRIAL).append("_jap.xml").c_str());
    clear += remove(std::string(FILENAME_CONT).append(".xml").c_str());
    clear += remove(std::string(FILENAME_CONT).append("_jap.xml").c_str());

    // remove returns 0 on success
    return clear == 6;
}

void Config::set_fps(int fps)
{
    video.fps = fps;
    // Set core FPS to 30fps or 60fps
    this->fps = video.fps == 0 ? 30 : 60;
    
    // Original game ticks sprites at 30fps but background scroll at 60fps
    tick_fps  = video.fps < 2 ? 30 : 60;

    cannonball::frame_ms = 1000.0 / this->fps;

    #ifdef COMPILE_SOUND_CODE
    if (config.sound.enabled)
        cannonball::audio.stop_audio();
    osoundint.init();
    if (config.sound.enabled)
        cannonball::audio.start_audio();
    #endif
}