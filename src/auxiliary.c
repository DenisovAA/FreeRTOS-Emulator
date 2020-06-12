#include "auxiliary.h"

void checkDraw(unsigned char status, const char *msg)
{
    if (status) {
        if (msg)
            fprintf(stderr, "[ERROR] %s, %s\n", msg,
                    tumGetErrorMessage());
        else {
            fprintf(stderr, "[ERROR] %s\n", tumGetErrorMessage());
        }
    }
}


void vDrawBorders (void)
{
    checkDraw(tumDrawLine(0, SCREEN_HEIGHT - 1.5 * DEFAULT_FONT_SIZE,
                          SCREEN_WIDTH, SCREEN_HEIGHT - 1.5 * DEFAULT_FONT_SIZE,
                          1, White), __FUNCTION__);

    checkDraw(tumDrawLine(0, 1.5 * DEFAULT_FONT_SIZE,
                          SCREEN_WIDTH, 1.5 * DEFAULT_FONT_SIZE,
                          1, White), __FUNCTION__);
}

char vDebounceKey (void) {
    return 0;
}
