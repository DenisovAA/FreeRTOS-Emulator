/**
 * @file figureCoordinates.h
 * @author Aleksandr Denisov
 * @date 15 May 2020
 * @brief Additional functions for Exercise 2
 */
#ifndef __EX2_H_
#define __EX2_H_

#define PI 3.1416
#define tan60deg 1.7321

#ifndef __TUM_DRAW_H__
#include "TUM_Draw.h"
#endif

#ifndef __TUM_EVENT_H__
#include "TUM_Event.h"
#endif

#ifndef __TUM_FONT_H__
#include "TUM_Font.h"
#endif

#ifndef _MATH_H
#include <math.h>
#endif

#ifndef _STDDEF_H
#include <stddef.h>
#endif

#define QUIT_STRING_LENGTH 6
#define CHANGE_STRING_LENGTH 8
#define BORDER_FOR_TEXT 20

/**
 * @brief Derives a tops (points) of equilateral triangle with specified
 * center point and triangle height
 *
 * @param points A pointer to result triangle tops 
 * @param center_point Center of triangle height
 * @param tri_height Desired triangle height 
 * 
 * @return 0 on success
 */
int triangleCoordinates(coord_t *points, coord_t center_point, unsigned short tri_height);

/**
 * @brief Derives a coordinates of top left corner of square with specified
 * center point and side length
 *
 * @param points A pointer to result coordinates of top left corner of box 
 * @param center_point Center of square
 * @param side_length Desired side length 
 * 
 * @return 0 on success
 */
int squareCoordinates(coord_t *points, coord_t center_point, unsigned short side_length);

/**
 * @brief Rotates point counter clock-wise around specified center point
 *
 * @param points A pointer to rotated point
 * @param origin Point origin position (w/o rotation)
 * @param center_point Point around which rotation performing
 * @param current_angle Pointer to current rotation angle value
 * @param angle_step Rotation step in degrees  
 * 
 * @return 0 on success
 */
int rotatePoint(coord_t *point, coord_t origin, coord_t center_point,
                float *current_angle, float angle_step);

/**
 * @brief Moves string in horizontal direction
 *
 * @param x Pointer to current x-axis string position 
 * @param x_offset Offset for left and right borders 
 * (Set 0 if screenInspiration is nit used)
 * @param border Left and right border of move
 * @param direction Pointer to current moving direction
 * if 0 - from left to right; if 1 - from right to left
 * @param string_width
 * @param step Moving step
 * 
 * @return 0 on success
 */
int moveStringHorizontaly(short *x, unsigned short x_offset, unsigned short border,
                        char *direction, unsigned int string_width, unsigned short step);

/**
 * @brief Draw [Q]uit and Chang[E] strings at screen
 *
 * @param font_size Desired font size
 * 
 * @return nothing
 */
void drawHelpText(unsigned int font_size);

/**
 * @brief Move point from origin to mouse direction 
 *
 * @param point Pointer to a point wich position is moved
 * @param origin Origin point place
 * 
 * @return nothing
 */
void screenInspiration(coord_t *point, coord_t origin);

/**
 * @brief Debounce value by simple "rising-edge" detection
 *
 * @param current_state Actuall current value
 * @param previous_state Pointer to last value state
 * 
 * @return debounced value
 */
char debounceButton(char current_state, char *previous_state);

/**
 * @brief Draw the current FPS value at the bottom-right corner
 * of the screen
 *
 * @return nothing
 */
void vDrawFPS(void);

#endif