/**
 * @file figuresCoordinates.c
 * @author Aleksandr Denisov
 * @date 15 May
 * @brief Calculates coordinates for tumDrawTriangle and tumDrawFilledBox finctions
 */
#include "exercise2.h"

int triangleCoordinates(coord_t *points, coord_t center_point, unsigned short tri_height)
{
        points[0].x = center_point.x - tri_height / tan60deg;
        points[0].y = center_point.y + tri_height / 2;
        points[1].x = center_point.x;
        points[1].y = center_point.y - tri_height /  2 ;
        points[2].x = center_point.x + tri_height / tan60deg;
        points[2].y = points[0].y;
        return 0;
}

int squareCoordinates(coord_t *points, coord_t center_point, unsigned short side_length)
{
        points[0].x = center_point.x - side_length / 2;
        points[0].y = center_point.y - side_length / 2;
        return 0;
}

int rotatePoint(coord_t *point, coord_t origin, coord_t center_point,
                float *current_angle, float angle_step)
{
        if (*current_angle > 360)
                *current_angle = 0;

        float sind = sin(*current_angle * PI / 180);
        float cosd = cos(*current_angle * PI / 180);
        point[0].x = cosd * (origin.x - center_point.x) - sind 
                        * (origin.y - center_point.y) + center_point.x;
        point[0].y = sind * (origin.x - center_point.x) + cosd
                        * (origin.y - center_point.y) + center_point.y;
        
        *current_angle = *current_angle + angle_step;
    return 0;
}

int moveStringHorizontaly(short *x, unsigned short x_offset, unsigned short border,
                        char *direction, unsigned int string_width, unsigned short step)
{
        short border_offset = SCREEN_WIDTH / 2 - x_offset;

        if (*direction == 1)
                *x = *x + step;
        else *x = *x - step;

        if (*x > SCREEN_WIDTH - string_width - border - border_offset) {
                *x = SCREEN_WIDTH - string_width - border - border_offset;
                *direction = 0;
        } 
        else if (*x < border - border_offset) {
                *x = border - border_offset;
                *direction = 1;
        }
        return 0;
}

void drawHelpText(unsigned int font_size)
{
    static char quit_string[QUIT_STRING_LENGTH] = "[Q]uit";
    static char change_string[CHANGE_STRING_LENGTH] = "Chang[E]";
    static int quit_string_width, change_string_width;
    
    if (!tumGetTextSize((char *)quit_string, &quit_string_width, NULL))
        tumDrawText(quit_string, SCREEN_WIDTH - BORDER_FOR_TEXT
                            - quit_string_width, font_size, Black);

    if (!tumGetTextSize((char *)change_string, &change_string_width, NULL))
        tumDrawText(change_string, SCREEN_WIDTH - BORDER_FOR_TEXT
                            - change_string_width, font_size * 2.5, Black);
}

void screenInspiration(coord_t *point, coord_t origin)
{
    short delta_x = origin.x - tumEventGetMouseX();
    short delta_y = origin.y - tumEventGetMouseY();
 
    delta_x = delta_x / 10;
    delta_y = delta_y / 10;

    point[0].x = origin.x - delta_x;
    point[0].y = origin.y - delta_y;
}

char debounceButton(char current_state, char *previous_state)
{
        char result = 0;
        if (current_state && !(*previous_state)) {
                result = 1;
                *previous_state = 1;
        }
        else if (!current_state)
                *previous_state = 0;

        return result;
}

#define FPS_AVERAGE_COUNT 50
#define FPS_FONT "IBMPlexSans-Bold.ttf"

void vDrawFPS(void)
{
    static unsigned int periods[FPS_AVERAGE_COUNT] = { 0 };
    static unsigned int periods_total = 0;
    static unsigned int index = 0;
    static unsigned int average_count = 0;
    static TickType_t xLastWakeTime = 0, prevWakeTime = 0;
    static char str[10] = { 0 };
    static int text_width;
     static int text_heigth;
    int fps = 0;
    font_handle_t cur_font = tumFontGetCurFontHandle();

    xLastWakeTime = xTaskGetTickCount();

    if (prevWakeTime != xLastWakeTime) {
        periods[index] =
            configTICK_RATE_HZ / (xLastWakeTime - prevWakeTime);
        prevWakeTime = xLastWakeTime;
    }
    else {
        periods[index] = 0;
    }

    periods_total += periods[index];

    if (index == (FPS_AVERAGE_COUNT - 1)) {
        index = 0;
    }
    else {
        index++;
    }

    if (average_count < FPS_AVERAGE_COUNT) {
        average_count++;
    }
    else {
        periods_total -= periods[index];
    }

    fps = periods_total / average_count;

    tumFontSelectFontFromName(FPS_FONT);

    sprintf(str, "FPS: %2d", fps);

    if (!tumGetTextSize((char *)str, &text_width, &text_heigth))
        checkDraw(tumDrawText(str, SCREEN_WIDTH - text_width - 10,
                              SCREEN_HEIGHT - text_heigth * 1.5,
                              Skyblue),
                  __FUNCTION__);

    tumFontSelectFontFromHandle(cur_font);
    tumFontPutFontHandle(cur_font);
}