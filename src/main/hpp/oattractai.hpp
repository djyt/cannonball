#pragma once

#include "outrun.hpp"

class OAttractAI
{
public:
    OAttractAI(void);
    ~OAttractAI(void);

    void tick_ai();

    void check_road_bonus();
    void set_steering_bonus();

private:
    void check_road();
    void set_steering();
};

extern OAttractAI oattractai;
