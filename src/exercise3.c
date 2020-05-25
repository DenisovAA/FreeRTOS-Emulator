#include "TUM_Draw.h"

int drawLed(coord_t position, unsigned short radius, 
            unsigned int color, char state)
{
    if (state)
        tumDrawCircle(position.x, position.y, radius, color);
    return 0;
}