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
#include "../utils.hpp"

#include "engine/ohiscore.hpp"
#include "engine/audio/osoundint.hpp"

// api change in boost 1.56
#include <boost/version.hpp>
#if (BOOST_VERSION >= 105600)
typedef boost::property_tree::xml_writer_settings<std::string> xml_writer_settings;
#else
typedef boost::property_tree::xml_writer_settings<char> xml_writer_settings;
#endif

Config config;

Config::Config(void)
{
    data.cfg_file = "./config.xml";
}


Config::~Config(void)
{
}


// Set Path to load and save config to
void Config::set_config_file(const std::string& file)
{
    data.cfg_file = file;
}

using boost::property_tree::ptree;
ptree pt_config;

void Config::load()
{
    // Load XML file and put its contents in property tree. 
    // No namespace qualification is needed, because of Koenig 
    // lookup on the second argument. If reading fails, exception
    // is thrown.
    try
    {
        read_xml(data.cfg_file, pt_config, boost::property_tree::xml_parser::trim_whitespace);
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }

    // ------------------------------------------------------------------------
    // Data Settings
    // ------------------------------------------------------------------------
    data.rom_path         = pt_config.get("data.rompath", "roms/");   // Path to ROMs
    data.save_path        = pt_config.get("data.savepath", "./");     // Path to Save Data

    data.file_scores      = data.save_path + "hiscores.xml";
    data.file_scores_jap  = data.save_path + "hiscores_jap.xml";
    data.file_ttrial      = data.save_path + "hiscores_timetrial.xml";
    data.file_ttrial_jap  = data.save_path + "hiscores_timetrial_jap.xml";
    data.file_cont        = data.save_path + "hiscores_continuous.xml";
    data.file_cont_jap    = data.save_path + "hiscores_continuous_jap.xml";

    // ------------------------------------------------------------------------
    // Menu Settings
    // ------------------------------------------------------------------------

    menu.enabled           = pt_config.get("menu.enabled",   1);
    menu.road_scroll_speed = pt_config.get("menu.roadspeed", 50);

    // ------------------------------------------------------------------------
    // Video Settings
    // ------------------------------------------------------------------------
   
    video.mode       = pt_config.get("video.mode",               2); // Video Mode: Default is Full Screen 
    video.scale      = pt_config.get("video.window.scale",       2); // Video Scale: Default is 2x    
    video.scanlines  = pt_config.get("video.scanlines",          0); // Scanlines
    video.fps        = pt_config.get("video.fps",                2); // Default is 60 fps
    video.fps_count  = pt_config.get("video.fps_counter",        0); // FPS Counter
    video.widescreen = pt_config.get("video.widescreen",         1); // Enable Widescreen Mode
    video.hires      = pt_config.get("video.hires",              0); // Hi-Resolution Mode
    video.filtering  = pt_config.get("video.filtering",          0); // Open GL Filtering Mode
    video.vsync      = pt_config.get("video.vsync",              1); // Use V-Sync where available (e.g. Open GL)

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
    // SMARTYPI Settings
    // ------------------------------------------------------------------------
    smartypi.enabled = pt_config.get("smartypi.<xmlattr>.enabled", 0);
    smartypi.ouputs  = pt_config.get("smartypi.outputs", 1);
    smartypi.cabinet = pt_config.get("smartypi.cabinet", 1);

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

    engine.freeplay      = pt_config.get("engine.freeplay",        0) != 0;
    engine.jap           = pt_config.get("engine.japanese_tracks", 0);
    engine.prototype     = pt_config.get("engine.prototype",       0);
    
    // Additional Level Objects
    engine.level_objects   = pt_config.get("engine.levelobjects", 1);
    engine.randomgen       = pt_config.get("engine.randomgen",    1);
    engine.fix_bugs_backup = 
    engine.fix_bugs        = pt_config.get("engine.fix_bugs",     1) != 0;
    engine.fix_timer       = pt_config.get("engine.fix_timer",    0) != 0;
    engine.layout_debug    = pt_config.get("engine.layout_debug", 0) != 0;
    engine.new_attract     = pt_config.get("engine.new_attract", 1) != 0;

    // ------------------------------------------------------------------------
    // Time Trial Mode
    // ------------------------------------------------------------------------

    ttrial.laps    = pt_config.get("time_trial.laps",    5);
    ttrial.traffic = pt_config.get("time_trial.traffic", 3);

    cont_traffic   = pt_config.get("continuous.traffic", 3);
}

bool Config::save()
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

    if (config.smartypi.enabled)
        pt_config.put("smartypi.cabinet",     config.smartypi.cabinet);

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
    pt_config.put("controls.analog.axis.accel", controls.axis[1]);
    pt_config.put("controls.analog.axis.brake", controls.axis[2]);

    pt_config.put("engine.freeplay",       (int) engine.freeplay);
    pt_config.put("engine.time",            engine.freeze_timer ? 4 : engine.dip_time);
    pt_config.put("engine.traffic",         engine.disable_traffic ? 4 : engine.dip_traffic);
    pt_config.put("engine.japanese_tracks", engine.jap);
    pt_config.put("engine.prototype",       engine.prototype);
    pt_config.put("engine.levelobjects",    engine.level_objects);
    pt_config.put("engine.fix_bugs",        (int) engine.fix_bugs);
    pt_config.put("engine.fix_timer",       (int) engine.fix_timer);
    pt_config.put("engine.new_attract",     engine.new_attract);

    pt_config.put("time_trial.laps",    ttrial.laps);
    pt_config.put("time_trial.traffic", ttrial.traffic);
    pt_config.put("continuous.traffic", cont_traffic), 

    ttrial.laps    = pt_config.get("time_trial.laps",    5);
    ttrial.traffic = pt_config.get("time_trial.traffic", 3);
    cont_traffic   = pt_config.get("continuous.traffic", 3);

    try
    {
        write_xml(data.cfg_file, pt_config, std::locale(), xml_writer_settings('\t', 1)); // Tab space 1
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }
    return true;
}

void Config::load_scores(bool original_mode)
{
    std::string filename;

    if (original_mode)
        filename = engine.jap ? data.file_scores_jap : data.file_scores;
    else
        filename = engine.jap ? data.file_cont_jap : data.file_cont;

    // Create empty property tree object
    ptree pt;

    try
    {
        read_xml(filename , pt, boost::property_tree::xml_parser::trim_whitespace);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
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

void Config::save_scores(bool original_mode)
{
    std::string filename;

    if (original_mode)
        filename = engine.jap ? data.file_scores_jap : data.file_scores;
    else
        filename = engine.jap ? data.file_cont_jap : data.file_cont;

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
    
    try
    {
        write_xml(filename, pt, std::locale(), xml_writer_settings('\t', 1)); // Tab space 1
    }
    catch (std::exception &e)
    {
        std::cout << "Error saving hiscores: " << e.what() << "\n";
    }
}

void Config::load_tiletrial_scores()
{
    // Counter value that represents 1m 15s 0ms
    static const uint16_t COUNTER_1M_15 = 0x11D0;

    // Create empty property tree object
    ptree pt;

    try
    {
        read_xml(engine.jap ? config.data.file_ttrial_jap : config.data.file_ttrial, pt, boost::property_tree::xml_parser::trim_whitespace);
    }
    catch (std::exception &e)
    {
        for (int i = 0; i < 15; i++)
            ttrial.best_times[i] = COUNTER_1M_15;

        std::cout << e.what();
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
    // Create empty property tree object
    ptree pt;

    // Time Trial Scores
    for (int i = 0; i < 15; i++)
    {
        pt.put("time_trial.score" + Utils::to_string(i), ttrial.best_times[i]);
    }

    try
    {
        write_xml(engine.jap ? config.data.file_ttrial_jap : config.data.file_ttrial, pt, std::locale(), xml_writer_settings('\t', 1)); // Tab space 1
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
    clear += remove(data.file_scores.c_str());
    clear += remove(data.file_scores_jap.c_str());
    clear += remove(data.file_ttrial.c_str());
    clear += remove(data.file_ttrial_jap.c_str());
    clear += remove(data.file_cont.c_str());
    clear += remove(data.file_cont_jap.c_str());

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

    if (config.sound.enabled)
        cannonball::audio.stop_audio();
    osoundint.init();
    if (config.sound.enabled)
        cannonball::audio.start_audio();
}

// Inc time setting from menu
void Config::inc_time()
{
    if (engine.dip_time == 3)
    {
        if (!engine.freeze_timer)
            engine.freeze_timer = 1;
        else
        {
            engine.dip_time = 0;
            engine.freeze_timer = 0;
        }
    }
    else
        engine.dip_time++;
}

// Inc traffic setting from menu
void Config::inc_traffic()
{
    if (engine.dip_traffic == 3)
    {
        if (!engine.disable_traffic)
            engine.disable_traffic = 1;
        else
        {
            engine.dip_traffic = 0;
            engine.disable_traffic = 0;
        }
    }
    else
        engine.dip_traffic++;
}