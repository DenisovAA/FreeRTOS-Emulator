#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "TUM_Ball.h"
#include "TUM_Draw.h"
#include "TUM_Font.h"
#include "TUM_Event.h"
#include "TUM_Sound.h"
#include "TUM_Utils.h"

#include "auxiliary.h"
#include "menus.h"

#define mainGENERIC_PRIORITY ( tskIDLE_PRIORITY )
#define mainGENERIC_STACK_SIZE ( (unsigned short)2560 )

#define DEBOUNCE_DELAY ((TickType_t) 100)
/* Keyboard LUT macro */
#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR

/* Screen updating period */
#define FRAMERATE_PERIOD 20;
#define FPS_FONT "IBMPlexSans-Bold.ttf"
#define FPS_AVERAGE_COUNT 50

/* Screen swap control handlers */
static TaskHandle_t BufferSwap = NULL;
static SemaphoreHandle_t ScreenLock = NULL;
static SemaphoreHandle_t DrawSignal = NULL;

/* Keyboard control handler */
typedef struct buttons_buffer {
    unsigned char current_state[SDL_NUM_SCANCODES];
    unsigned char buttons[SDL_NUM_SCANCODES];
    unsigned char last_buttons_state[SDL_NUM_SCANCODES];
    TickType_t debounce_delay[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;
} buttons_buffer_t;

static buttons_buffer_t keyboard = { 0 };

/* Game screens */
static TaskHandle_t GameScreen = NULL;

void vDrawFPS(void)
{
    static unsigned int periods[FPS_AVERAGE_COUNT] = { 0 };
    static unsigned int periods_total = 0;
    static unsigned int index = 0;
    static unsigned int average_count = 0;
    static TickType_t xLastWakeTime = 0, prevWakeTime = 0;
    static char str[10] = { 0 };
    static int text_width;
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

    if (!tumGetTextSize((char *)str, &text_width, NULL))
        checkDraw(tumDrawText(str, SCREEN_WIDTH - text_width - 10,
                              SCREEN_HEIGHT - DEFAULT_FONT_SIZE * 1.5,
                              Skyblue),
                  __FUNCTION__);

    tumFontSelectFontFromHandle(cur_font);
    tumFontPutFontHandle(cur_font);
}

void xGetKeyboardInput (void)
{
    if (xSemaphoreTake(keyboard.lock, 0) == pdTRUE) {
        xQueueReceive(buttonInputQueue, &keyboard.current_state, 0);
        /*
        for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        {
            keyboard.buttons[i] = 0;

            if (keyboard.current_state[i] != keyboard.last_buttons_state[i]) {
                keyboard.buttons[i] = keyboard.current_state[i];
            } 

            keyboard.last_buttons_state[i] = keyboard.current_state[i];
        }
        */

        for (int i = 0; i < SDL_NUM_SCANCODES; i++)
        {
            if (keyboard.current_state[i] != keyboard.last_buttons_state[i]) {
                keyboard.debounce_delay[i] = xTaskGetTickCount();
            }

            if ((xTaskGetTickCount() - keyboard.debounce_delay[i]) > DEBOUNCE_DELAY) {
                if (keyboard.current_state[i] != keyboard.buttons[i]) {
                    keyboard.buttons[i] = keyboard.current_state[i];
                }
            }
            keyboard.last_buttons_state[i] = keyboard.current_state[i];
        }
        
        xSemaphoreGive(keyboard.lock);
    }
}

void vBufferSwap (void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    const TickType_t frameratePeriod = 20;

    tumDrawBindThread(); // Setup Rendering handle with correct GL context

    while (1) {
        if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE) {
            tumDrawUpdateScreen();
            tumEventFetchEvents(FETCH_EVENT_BLOCK);
            xSemaphoreGive(ScreenLock);
            xSemaphoreGive(DrawSignal);
            vTaskDelayUntil(&xLastWakeTime,
                            pdMS_TO_TICKS(frameratePeriod));
        }
    }
}

void vGameScreen (void *pvParameters)
{
    int counter[4] = { 0 };
    char note_string[256] = { 0 };

    while(1) {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE) {
                tumEventFetchEvents(FETCH_EVENT_BLOCK | FETCH_EVENT_NO_GL_CHECK);

                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                // Clear screen
                checkDraw(tumDrawClear(Black), __FUNCTION__);
                
                //DrawIntro();
                DrawMainMenu();
                vDrawBorders();
                vDrawFPS();
                xGetKeyboardInput();

                // Simple notepad
                if (xSemaphoreTake(keyboard.lock, 0) == pdTRUE) {
                    counter[0] += keyboard.buttons[KEYCODE(A)];
                    counter[1] += keyboard.buttons[KEYCODE(B)];
                    counter[2] += keyboard.buttons[KEYCODE(C)];
                    counter[3] += keyboard.buttons[KEYCODE(D)];
                    xSemaphoreGive(keyboard.lock);
                }
                
                sprintf(note_string, "A: %d B: %d C: %d D: %d", 
                        counter[0], counter[1], counter[2], counter[3]);
                tumDrawText(note_string, 0, 0, White);

                xSemaphoreGive(ScreenLock);   
            }
    }
}

int main(int argc, char *argv[])
{
    char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);
    
    printf("Initializing...\n");

    if(tumDrawInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize drawing");
        goto err_init_drawing;
    }

    if(tumEventInit()) {
        PRINT_ERROR("Failed to initialize events");
        goto err_init_events;
    }

    // Keyboard mutex
    keyboard.lock = xSemaphoreCreateMutex();
    if (!keyboard.lock) {
        PRINT_ERROR("Failed to create keyboard mutex");
    }

    // Screen update mechanism
    if (xTaskCreate(vBufferSwap, "BufferSwapTask", mainGENERIC_STACK_SIZE,
                    NULL, configMAX_PRIORITIES, &BufferSwap) != pdPASS) {
        PRINT_TASK_ERROR("BufferSwapTask");
        goto err_buffer_swap;            
        }
    DrawSignal = xSemaphoreCreateBinary();
    if (!DrawSignal) {
        PRINT_ERROR("Failed to create DrawSignal");
        goto err_draw_signal;
    }
    ScreenLock = xSemaphoreCreateMutex();
    if (!ScreenLock) {
        PRINT_ERROR("Failed to create ScreenLock");
        goto err_screen_lock;
    }

    // Game Screen Task
    xTaskCreate(vGameScreen, "GameScreen", mainGENERIC_STACK_SIZE,
                NULL, configMAX_PRIORITIES - 1, &GameScreen);

    vTaskStartScheduler();

    return EXIT_SUCCESS;

    err_screen_lock:
        vSemaphoreDelete(DrawSignal);
    err_draw_signal:
        vTaskDelete(BufferSwap);
    err_buffer_swap:
        tumEventExit();
    err_init_events:
        tumDrawExit();
    err_init_drawing:
        return EXIT_FAILURE;
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vApplicationIdleHook(void)
{
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    /* Makes the process more agreeable when using the Posix simulator. */
    xTimeToSleep.tv_sec = 1;
    xTimeToSleep.tv_nsec = 0;
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}
