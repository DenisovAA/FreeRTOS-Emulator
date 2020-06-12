/**
 * @file TUM_Draw.h
 * @author Alex Hoffman
 * @date 27 August 2019
 * @brief A SDL2 based library to implement work queue based drawing of graphical
 * elements. Allows for drawing using SDL2 from multiple threads.
 *
 * @mainpage FreeRTOS Simulator Graphical Library
 *
 * @section intro_sec About this library
 *
 * This basic API aims to provide a simple method for drawing
 * graphical objects onto a screen in a thread-safe and consistent fashion.
 * The library is built on top of the widely used SDL2 graphics, text and sound
 * libraries. The core of the library is the functionality found in @ref tum_draw
 * with extra features such as event handling and sound found in the auxiliary
 * files.
 *
 * @section licence_sec Licence
 * @verbatim
 ----------------------------------------------------------------------
 Copyright (C) Alexander Hoffman, 2019
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ----------------------------------------------------------------------
 @endverbatim
 */

#ifndef __MENUS_H_
#define __MENUS_H_

#define MAIN_LOGO_FILENAME "../resources/sprites/mainLogo.png"

#define MAIN_MENU_OPTIONS 4

void DrawIntro (void);

void DrawMainMenu (void);

#endif