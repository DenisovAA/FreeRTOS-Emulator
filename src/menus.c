#include "auxiliary.h"
#include "menus.h"


void DrawIntro (void)
{
    image_handle_t MainLogo = NULL;

    MainLogo = tumDrawLoadImage(MAIN_LOGO_FILENAME);

    static int image_width;

    if ((image_width = tumDrawGetLoadedImageWidth(MainLogo)) != -1)
        checkDraw(tumDrawLoadedImage(MainLogo, 0,
                                     0),
                  __FUNCTION__);
    else {
        fprintf(stderr,
                "Failed to get size of image '%s', does it exist?\n",
                MAIN_LOGO_FILENAME);
    }
}

void DrawMainMenu (void)
{
    static char main_menu_text[MAIN_MENU_OPTIONS][30] = { 0 };
    static int main_menu_width[MAIN_MENU_OPTIONS] = { 0 };

    ssize_t prev_font_size = tumFontGetCurFontSize();
    ssize_t current_font_size = 20;

    sprintf(main_menu_text[0], "One player");
    sprintf(main_menu_text[1], "Two players");
    sprintf(main_menu_text[2], "Score table");
    sprintf(main_menu_text[3], "Quit");

    tumFontSetSize(current_font_size);

    for (int i=0;i<MAIN_MENU_OPTIONS;i++) {
        if (!tumGetTextSize((char *)main_menu_text[i], &main_menu_width[i], NULL))
            checkDraw(tumDrawText(main_menu_text[i],
                                  SCREEN_WIDTH / 2 - main_menu_width[i] / 2,
                                  SCREEN_HEIGHT / 2 + (i * 2) * current_font_size,
                                  White),
                                  __FUNCTION__);
    }
    tumFontSetSize(prev_font_size);
}

void ChooseOption (short max_value, short *current_value, char dir)
{

}