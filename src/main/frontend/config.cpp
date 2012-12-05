// see: http://www.boost.org/doc/libs/1_52_0/doc/html/boost_propertytree/tutorial.html

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "config.hpp"

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
    read_xml(filename, pt, boost::property_tree::xml_parser::trim_whitespace);

    // ------------------------------------------------------------------------
    // Video Settings
    // ------------------------------------------------------------------------

    // Video Mode: Default is Windowed
    video.mode    = pt.get("video.mode", 0);
    // Video Scale: Default is 1x
    video.scale   = pt.get("video.window.scale", 1);
    // Stretch in full-screen mode
    video.stretch = pt.get("video.fullscreen.stretch", 0);
    // Default is 60 fps
    video.fps     = pt.get("video.fps", 2);
           
    // Set core FPS to 30fps or 60fps
    fps       = video.fps == 0 ? 30 : 60;
    
    // Original game ticks sprites at 30fps but background scroll at 60fps
    tick_fps = video.fps < 2 ? 30 : 60;

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

/*int main()
{
    try
    {
        debug_settings ds;
        ds.load("debug_settings.xml");
        ds.save("debug_settings_out.xml");
        std::cout << "Success\n";
    }
    catch (std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n";
    }
    return 0;
}*/

