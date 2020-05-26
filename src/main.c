#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "TUM_Ball.h"
#include "TUM_Draw.h"
#include "TUM_Font.h"
#include "TUM_Event.h"
#include "TUM_Sound.h"
#include "TUM_Utils.h"

#include "exercise2.h"

/* FreeRTOS Config */
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)
#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)

/* Keyboard LUT macro */
#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR

#define TICKS_TO_SHOW 15
#define TASKS_TO_SHOW 4

static TaskHandle_t Alpha = NULL;
static TaskHandle_t Beta = NULL;
static TaskHandle_t Gamma = NULL;
static TaskHandle_t Delta = NULL;
static TaskHandle_t OutputTask = NULL;

const TickType_t AlphaPeriod = 1;
const TickType_t BetaPeriod = 2;
const TickType_t DeltaPeriod = 4;

static SemaphoreHandle_t GammaSignal = NULL;

static QueueHandle_t ResultQueue = NULL;

typedef struct ticks_table
{
    unsigned char values[TICKS_TO_SHOW][TASKS_TO_SHOW];
    TickType_t start_tick;
    TickType_t stop_tick;
    unsigned short counter;
    SemaphoreHandle_t lock;
} ticks_table_t;

ticks_table_t output = { 0 };

#define PRINT_TASK_ERROR(task) PRINT_ERROR("Failed to print task ##task");

void stopFunc (void)
{
    printf("Stop\n");
}

void vAlpha (void *pvParameters)
{
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();

    static unsigned short index = 0;
    static unsigned char res = 0;
    static unsigned char is_finished = 0;

    static unsigned char task_return = 1;

    while (1) {
        vTaskDelayUntil(&last_wake_time, AlphaPeriod);
        if (xSemaphoreTake(output.lock, 0) == pdTRUE) {
            if (ResultQueue) {
                xQueueSend(ResultQueue, &task_return, 0);
                index = 0;
                while (xQueueReceive(ResultQueue, &res, 0) == pdTRUE) {
                    output.values[output.counter][index] = res;
                    index += 1;
                }
                xQueueReset(ResultQueue);
            }
            output.counter += 1;
            if (output.counter > TICKS_TO_SHOW - 1) {
                is_finished = 1;
                output.counter = 0;
            }
            xSemaphoreGive(output.lock);
        }
        if (is_finished) {
            xTaskNotifyGive(OutputTask);
        }
    }
}

void vBeta (void *pvParameters)
{
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();
    static unsigned char task_return = 2;

    while (1) {
        vTaskDelayUntil(&last_wake_time, BetaPeriod);
      
        xSemaphoreGive(GammaSignal);
      
        if (ResultQueue) {
            xQueueSend(ResultQueue, &task_return, 0);
        }
    }
}

void vGamma (void *pvParameters)
{
    static unsigned char task_return = 3;

    while (1)
        if (GammaSignal)
            if (xSemaphoreTake(GammaSignal, portMAX_DELAY) == pdTRUE) {
                if (ResultQueue) {
                    xQueueSend(ResultQueue, &task_return, 0);
                } 
            }
}

void vDelta (void *pvParameters)
{
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();
    static unsigned char task_return = 4;

    while (1) {
        vTaskDelayUntil(&last_wake_time, DeltaPeriod);
        if (ResultQueue) {
            xQueueSend(ResultQueue, &task_return, 0);
        }
    }
}

void vOutputTask (void *pvParameters)
{
    while (1) {
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        vTaskSuspend(Alpha);
        vTaskSuspend(Beta);
        vTaskSuspend(Gamma);
        vTaskSuspend(Delta);
        if (xSemaphoreTake(output.lock, 0) == pdTRUE) {
            for (int i = 0; i < TICKS_TO_SHOW; i++)
            {
                printf("TICK #%d : ", i);
                for (int j = 0; j < TASKS_TO_SHOW; j++)
                {
                    printf("%d", output.values[i][j]);
                }
                printf("\n");
            }
            xSemaphoreGive(output.lock);    
        }
        //vTaskSuspend(OutputTask);
    }
}

int main (int argc, char *argv[])
{
    ResultQueue = xQueueCreate(TASKS_TO_SHOW, sizeof( unsigned char) );

    xTaskCreate(vAlpha, "Task1", mainGENERIC_STACK_SIZE, NULL, 1, &Alpha);

    xTaskCreate(vBeta, "Task2", mainGENERIC_STACK_SIZE, NULL, 2, &Beta);

    xTaskCreate(vGamma, "Task3", mainGENERIC_STACK_SIZE, NULL, 3, &Gamma);
    GammaSignal = xSemaphoreCreateBinary();

    xTaskCreate(vDelta, "Task4", mainGENERIC_STACK_SIZE, NULL, 4, &Delta);

    xTaskCreate(vOutputTask, "OutputTask", mainGENERIC_STACK_SIZE, NULL, 5, &OutputTask);
    output.lock = xSemaphoreCreateMutex();

    vTaskStartScheduler();
    
    return EXIT_SUCCESS;
}



/* --------------------------------------------------------------------- */
/* Static task allocation support */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* Timer allocation support */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
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