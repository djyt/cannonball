#pragma once

#include <set>
#include <iostream>

struct video_settings_t
{
    const static int WINDOW = 0;
    const static int FULLSCREEN = 1;

    int mode;
    int scale;
    int stretch;
};

class Config
{
public:
    video_settings_t video_settings;

    Config(void);
    ~Config(void);

    void init();
    void load(const std::string &filename);
    void save(const std::string &filename);

private:
};

