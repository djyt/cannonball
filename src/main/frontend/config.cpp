// see: http://www.boost.org/doc/libs/1_52_0/doc/html/boost_propertytree/tutorial.html

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "config.hpp"


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
    
    // Video Mode: Default is Windowed
    video_settings.mode    = pt.get("video.mode", video_settings_t::WINDOW);
    // Video Scale: Default is 1x
    video_settings.scale   = pt.get("video.window.scale", 1);
    // Stretch in full-screen mode
    video_settings.stretch = pt.get("video.fullscreen.stretch", 0);
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

