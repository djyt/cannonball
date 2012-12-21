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

void Config::load(const std::string &filename)
{
    // Create empty property tree object
    using boost::property_tree::ptree;
    ptree pt;
    
    // Load XML file and put its contents in property tree. 
    // No namespace qualification is needed, because of Koenig 
    // lookup on the second argument. If reading fails, exception
    // is thrown.
    try
    {
        read_xml(filename, pt, boost::property_tree::xml_parser::trim_whitespace);
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }

    // ------------------------------------------------------------------------
    // Video Settings
    // ------------------------------------------------------------------------

    // Default is 60 fps
    video.fps        = pt.get("video.fps", 2);
    // Enable Widescreen Mode
    video.widescreen = pt.get("video.widescreen", 0);
    // Video Mode: Default is Windowed
    video.mode       = pt.get("video.mode", 0);
    // Video Scale: Default is 1x
    video.scale      = pt.get("video.window.scale", 1);
    // Stretch in full-screen mode
    video.stretch    = pt.get("video.fullscreen.stretch", 0);
          
    // Set core FPS to 30fps or 60fps
    fps       = video.fps == 0 ? 30 : 60;
    
    // Original game ticks sprites at 30fps but background scroll at 60fps
    tick_fps = video.fps < 2 ? 30 : 60;

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
    // Engine Settings
    // ------------------------------------------------------------------------

    engine.dip_time      = pt.get("engine.time", 0);
    engine.dip_traffic   = pt.get("engine.traffic", 1);
    engine.dip_advertise = pt.get("engine.advertise", 1);
    engine.gear          = pt.get("engine.gear", 0);  
    
    engine.freeze_timer    = engine.dip_time == 4;
    engine.disable_traffic = engine.dip_traffic == 4;
    engine.dip_time    &= 3;
    engine.dip_traffic &= 3;
    
    // Additional Level Objects
    engine.level_objects = pt.get("engine.levelobjects", 1);
}

void Config::save(const std::string &filename)
{

    // Create empty property tree object
    /*using boost::property_tree::ptree;
    ptree pt;

    // Put log filename in property tree
    pt.put("debug.filename", m_file);

    // Put debug level in property tree
    pt.put("debug.level", m_level);

    // Iterate over modules in set and put them in property 
    // tree. Note that put function places new key at the
    // end of list of keys. This is fine in most of the
    // situations. If you want to place item at some other
    // place (i.e. at front or somewhere in the middle),
    // this can be achieved using combination of insert 
    // and put_value functions
    //BOOST_FOREACH(const std::string &name, m_modules)
    //    pt.put("debug.modules.module", name, true);
    
    // Write property tree to XML file
    write_xml(filename, pt);*/
}

void Config::load_scores(const std::string &filename)
{
    // Create empty property tree object
    using boost::property_tree::ptree;
    ptree pt;

    try
    {
        read_xml(filename, pt, boost::property_tree::xml_parser::trim_whitespace);
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

void Config::save_scores(const std::string &filename)
{
    // Create empty property tree object
    using boost::property_tree::ptree;
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
    
    boost::property_tree::xml_writer_settings<char> settings('\t', 1);
    
    try
    {
        write_xml(filename, pt, std::locale(), settings);
    }
    catch (std::exception &e)
    {
        std::cout << "Error saving hiscores: " << e.what() << "\n";
    }
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