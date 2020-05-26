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

/* Screen updating period */
#define FRAMERATE_PERIOD 20;

/* StateMachine Config */ 
#define STATE_QUEUE_LENGTH 1
#define STATE_COUNT        3
#define STATE_ONE          0
#define STATE_TWO          1
#define STATE_THREE        2
#define NEXT_TASK          0
#define PREV_TASK          1
#define STARTING_STATE STATE_ONE
#define STATE_DEBOUNCE_DELAY 100

const unsigned char next_state_signal = NEXT_TASK;
const unsigned char prev_state_signal = PREV_TASK;

/* Led control constants */
#define FIRST_LED_BLINK_PERIOD 500
#define SECOND_LED_BLINK_PERIOD 250
#define LED_SIZE 15

/* Timers constants */
#define RESET_TIMER_PERIOD 15000

/* Counter constants */
#define COUNTER_PERIOD 1000

/* Initialization tasks handlers */
static TaskHandle_t SwapBuffers = NULL;
static TaskHandle_t SecondExercise = NULL;
static TaskHandle_t ThirdExercise = NULL;
static TaskHandle_t FourthExercise = NULL;
static TaskHandle_t StateMachine = NULL;
static TaskHandle_t SecondLedControl = NULL;
static TaskHandle_t ButtonPressedL = NULL;
static TaskHandle_t ButtonPressedK = NULL;
static TaskHandle_t SimpleCounter = NULL;

/* Static task vFirstLedControl variables */
StaticTask_t xFirstLedControlBuffer;
StackType_t xFirstLedControlStack[2 * mainGENERIC_STACK_SIZE]; 
TaskHandle_t FirstLedControlStatic = NULL;

/* Initialization Timers */
static TimerHandle_t ResetTimer = NULL;

/* Initialization MUTEXes handlers */
static SemaphoreHandle_t DrawSignal = NULL;

/* Initialization Semaphores handlers */
static SemaphoreHandle_t ScreenLock = NULL;
static SemaphoreHandle_t PressedSignalL = NULL;

/* Initialization queues handlers */
static QueueHandle_t StateQueue = NULL;

/* Exercise 4 */
#define TICKS_TO_SHOW 15
#define TASKS_TO_SHOW 4

// Task 1 handler (prority 1)
static TaskHandle_t Alpha = NULL;
// Task 2 handler (prority 2)
static TaskHandle_t Beta = NULL;
// Task 3 handler (prority 3)
static TaskHandle_t Gamma = NULL;
// Task 4 handler (prority 4)
static TaskHandle_t Delta = NULL;

// Tasks periods
const TickType_t AlphaPeriod = 1;
const TickType_t BetaPeriod = 2;
const TickType_t DeltaPeriod = 4;

// Output queue
static QueueHandle_t ResultQueue = NULL;

// Binary semaphore for Task 3 notification
static SemaphoreHandle_t GammaSignal = NULL;

/* Shared structures */
// Handles the keyboard buttons pressed
typedef struct buttons_buffer {
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;
} buttons_buffer_t;

// Handles the "LEDs" states for exercise 3
typedef struct led {
    char state;
    SemaphoreHandle_t lock;
} led_t;

// Handles the button counters for exercise 3
typedef struct buttons_counter {
    unsigned short value;
    SemaphoreHandle_t lock;
} buttons_counter_t;

// Handles the ticks-tasks table for exercise 3
typedef struct ticks_table
{
    char values[TICKS_TO_SHOW][TASKS_TO_SHOW];
    TickType_t start_tick;
    TickType_t stop_tick;
    unsigned short counter;
    SemaphoreHandle_t lock;
} ticks_table_t;

static buttons_buffer_t buttons = { 0 };
static led_t first_led = { 0 };
static led_t second_led = { 0 };
static buttons_counter_t button_L = { 0 };
static buttons_counter_t button_K = { 0 };
static buttons_counter_t simple_counter = { 0 };
static ticks_table_t output = { 0 };

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

/*
 * Changes the state, either forwards of backwards
 */
void changeState(volatile unsigned char *state, unsigned char forwards)
{
    switch (forwards) {
        case NEXT_TASK:
            if (*state == STATE_COUNT - 1) {
                *state = 0;
            }
            else {
                (*state)++;
            }
            break;
        case PREV_TASK:
            if (*state == 0) {
                *state = STATE_COUNT - 1;
            }
            else {
                (*state)--;
            }
            break;
        default:
            break;
    }
}

/*
 * Example basic state machine with sequential states
 */
void basicSequentialStateMachine(void *pvParameters)
{
    unsigned char current_state = STARTING_STATE; // Default state
    unsigned char state_changed =
        1; // Only re-evaluate state if it has changed
    unsigned char input = 0;

    const int state_change_period = STATE_DEBOUNCE_DELAY;

    TickType_t last_change = xTaskGetTickCount();

    while (1) {
        if (state_changed) {
            goto initial_state;
        }

        // Handle state machine input
        if (StateQueue)
            if (xQueueReceive(StateQueue, &input, portMAX_DELAY) ==
                pdTRUE)
                if (xTaskGetTickCount() - last_change >
                    state_change_period) {
                    changeState(&current_state, input);
                    state_changed = 1;
                    last_change = xTaskGetTickCount();
                }

initial_state:
        // Handle current state
        if (state_changed) {
            switch (current_state) {
                case STATE_ONE:
                    if (ThirdExercise) {
                        vTaskSuspend(ThirdExercise);
                        vTaskSuspend(FirstLedControlStatic);
                        vTaskSuspend(SecondLedControl);
                    }
                    if (SecondExercise) {
                        vTaskResume(SecondExercise);
                    }
                    if (FourthExercise) {
                        vTaskSuspend(FourthExercise);
                    }
                    break;
                case STATE_TWO:
                    if (SecondExercise) {
                        vTaskSuspend(SecondExercise);
                    }
                    if (ThirdExercise) {
                        vTaskResume(ThirdExercise);
                        vTaskResume(FirstLedControlStatic);
                        vTaskResume(SecondLedControl);
                        vTaskResume(ButtonPressedL);
                    }
                    if (FourthExercise) {
                        vTaskSuspend(FourthExercise);
                    }
                    break;
                case STATE_THREE:
                    if (ThirdExercise) {
                        vTaskSuspend(ThirdExercise);
                        vTaskSuspend(FirstLedControlStatic);
                        vTaskSuspend(SecondLedControl);
                    }
                    if (SecondExercise) {
                        vTaskSuspend(SecondExercise);
                    } 
                    if (FourthExercise) {
                        vTaskResume(FourthExercise);   
                    }
                    break;
                default:
                    break;
            }
            state_changed = 0;
        }
    }
}


static int vCheckStateInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        if (buttons.buttons[KEYCODE(E)]) {
            buttons.buttons[KEYCODE(E)] = 0;
            if (StateQueue) {
                xSemaphoreGive(buttons.lock);
                xQueueSend(StateQueue, &next_state_signal, 0);
                return -1;
            }
        }
        xSemaphoreGive(buttons.lock);
    }

    return 0;
}

void vSwapBuffers (void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    const TickType_t frameratePeriod = FRAMERATE_PERIOD;

    tumDrawBindThread(); // Setup Rendering handle with correct GL context

    while (1) {
        if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE) {
            tumDrawUpdateScreen();
            tumEventFetchEvents();
            xSemaphoreGive(ScreenLock);
            xSemaphoreGive(DrawSignal);
            vTaskDelayUntil(&xLastWakeTime,
                            pdMS_TO_TICKS(frameratePeriod));
        }
    }
}

void xGetButtonInput( void )
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}

void xButtonCounter(unsigned short *counter, char *debounce_values)
{
    if (tumEventGetMouseLeft() || tumEventGetMouseRight()) {
        counter[0] = 0;
        counter[1] = 0;
        counter[2] = 0;
        counter[3] = 0;
    }
    else
    {
        if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        counter[0] += debounceButton(buttons.buttons[KEYCODE(A)], &debounce_values[0]);
        counter[1] += debounceButton(buttons.buttons[KEYCODE(B)], &debounce_values[1]);
        counter[2] += debounceButton(buttons.buttons[KEYCODE(C)], &debounce_values[2]);
        counter[3] += debounceButton(buttons.buttons[KEYCODE(D)], &debounce_values[3]);
        xSemaphoreGive(buttons.lock);
        }
    }
}

void vDrawButtonsAndMouse(coord_t up_left_position, unsigned short *button_counter)
{
    static char str[100] = { 0 };

    sprintf(str, "Axis 1: %5d | Axis 2: %5d", tumEventGetMouseX(), tumEventGetMouseY());

    checkDraw(tumDrawText(str, up_left_position.x, up_left_position.y, Black),
              __FUNCTION__);

        sprintf(str, "A: %d | B: %d | C: %d | D: %d",
                button_counter[0],
                button_counter[1],
                button_counter[2],
                button_counter[3]);
        
        checkDraw(tumDrawText(str, up_left_position.x, up_left_position.y + 2 * DEFAULT_FONT_SIZE,
                             Black),
                  __FUNCTION__);
}

void vSecondExercise (void *pvParameters)
{
    coord_t screen_center = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2},
            triangle_tops[3],
            triangle_center = screen_center,
            circle_center,
            rotated_circle_center,
            square_center,
            rotated_square_center,
            square_top_left,

            above_string_position = {0, 0},
            below_string_position,

            button_counter_position;

    float circle_current_angle = 0,
          square_current_angle = 0,
          rotation_angle_step = 10;
    
    static unsigned short triangle_height = 30;
    static unsigned short spacing = 50;

    static char below_string[11] = "Exercise 2";
    static char above_string[11] = "Sample text";
    static int below_string_width = 0;
    static int above_string_width = 0;
    static unsigned short move_string_step = 1;
    char move_direction = 0;

    static unsigned short buttons_counter[4] = { 0 };
    static char buttons_debounce[4] = { 0 };

    while(1) {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE) {
                xGetButtonInput();
                xSemaphoreTake(ScreenLock, portMAX_DELAY);
                
                
                tumDrawClear(White);
                /* Calculates new center point of triangle w.r.t 
                   mouse position. Coordinates of another figures and 
                   text strings depend on the center of triangle */ 
                
                drawHelpText(DEFAULT_FONT_SIZE);
                vDrawFPS();
                screenInspiration(&triangle_center, screen_center);

                triangleCoordinates(triangle_tops, triangle_center, triangle_height);
                tumDrawTriangle(triangle_tops, Green);

                circle_center.x = triangle_center.x + spacing;
                circle_center.y = triangle_center.y;
                rotatePoint(&rotated_circle_center, circle_center, triangle_center,
                            &circle_current_angle, rotation_angle_step);
                tumDrawCircle(rotated_circle_center.x, rotated_circle_center.y,
                             triangle_height / 2, TUMBlue);
                
                square_center.x = triangle_center.x - spacing;
                square_center.y = triangle_center.y;
                rotatePoint(&rotated_square_center, square_center, triangle_center,
                            &square_current_angle, rotation_angle_step);
                squareCoordinates(&square_top_left, rotated_square_center, triangle_height);
                tumDrawFilledBox(square_top_left.x, square_top_left.y,
                                triangle_height, triangle_height, Red);

                /* String drawing */
                // Below string
                tumGetTextSize((char *)below_string, &below_string_width, NULL);
                below_string_position.x = triangle_center.x - below_string_width / 2;
                below_string_position.y = triangle_center.y + 2 * spacing;
                tumDrawText(below_string, below_string_position.x,
                                      below_string_position.y, Black);

                // Above string
                above_string_position.y = triangle_center.y - 2 * spacing;
                tumGetTextSize((char *)above_string, &above_string_width, NULL);
                moveStringHorizontaly(&above_string_position.x, triangle_center.x, 2 * above_string_width,
                                    &move_direction, above_string_width, move_string_step);
                tumDrawText(above_string, above_string_position.x,
                                      above_string_position.y, Black);

                // Counting for times A,B,C,D was pressed
                xButtonCounter(buttons_counter, buttons_debounce);
                button_counter_position.x = triangle_center.x - screen_center.x + DEFAULT_FONT_SIZE;
                button_counter_position.y = triangle_center.y - screen_center.y + DEFAULT_FONT_SIZE;
                vDrawButtonsAndMouse(button_counter_position, buttons_counter);
                xSemaphoreGive(ScreenLock);
                vCheckStateInput();
            }
    }
}
/* Third exercise with additional tasks and functions */
void vFirstLedControl(void *pvParameters)
{
    configASSERT( (uint32_t) pvParameters == 1UL );

    TickType_t last_state_change = xTaskGetTickCount();

    while(1) {
        if (xTaskGetTickCount() - last_state_change > FIRST_LED_BLINK_PERIOD) {
            if (xSemaphoreTake(first_led.lock, 0) == pdTRUE) {
                first_led.state = first_led.state ^ 1;
                xSemaphoreGive(first_led.lock);
            }   
            last_state_change = xTaskGetTickCount();
        }
    }
}

void vSecondLedControl(void *pvParameters)
{
    TickType_t last_state_change = xTaskGetTickCount();
    
    while(1) {
        if (xTaskGetTickCount() - last_state_change > SECOND_LED_BLINK_PERIOD) {
            if (xSemaphoreTake(second_led.lock, 0) == pdTRUE) {
                second_led.state = second_led.state ^ 1;
                xSemaphoreGive(second_led.lock);
            }   
            last_state_change = xTaskGetTickCount();
        }
    }
}

void vDrawLed(led_t led_object, coord_t position, int color) 
{
    if (xSemaphoreTake(led_object.lock, 0) == pdTRUE) {
        if (led_object.state)
            tumDrawCircle(position.x, position.y, LED_SIZE, color);
        xSemaphoreGive(led_object.lock);
    }
}

void vButtonPressedSemaphore(SemaphoreHandle_t semaphore, int button_code)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        if (buttons.buttons[button_code]) {
            buttons.buttons[button_code] = 0;
            xSemaphoreGive(semaphore);
        }
        xSemaphoreGive(buttons.lock);
    }
}

void vDrawButtonCounter (buttons_counter_t counter, coord_t string_position,
                        char *button_name, unsigned short *last_value) 
{   
    static char str[100] = { 0 };

    if (xSemaphoreTake(counter.lock, 0) == pdTRUE) {
        *last_value = counter.value;
        xSemaphoreGive(counter.lock);
    }
            
    sprintf(str, "%c was pressed %d times", *button_name, *last_value);
    tumDrawText(str,string_position.x, string_position.y ,Black);
}

void vButtonPressedL (void *pvParameters)
{
    while(1) {
        if (PressedSignalL)
            if (xSemaphoreTake(PressedSignalL, 0) == pdTRUE) {
                if (xSemaphoreTake(button_L.lock, 0) == pdTRUE) {
                    button_L.value += 1;
                    xSemaphoreGive(button_L.lock);
                }
            }
    }
}

void vButtonPressedK (void *pvParameters)
{
    while(1) {
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        if (xSemaphoreTake(button_K.lock, 0) == pdTRUE) {
            button_K.value += 1;
            xSemaphoreGive(button_K.lock);
        }
        xTaskNotifyGive(ThirdExercise);
    }
}

/* Timer reset counted numbers of buttons "L" and "K" was pressed */
void vResetTimerCallback (TimerHandle_t ResetTimer)
{
    if (xSemaphoreTake(button_L.lock, portMAX_DELAY) == pdTRUE) {
        button_L.value = 0;
        xSemaphoreGive(button_L.lock);
    }

    if (xSemaphoreTake(button_K.lock, portMAX_DELAY) == pdTRUE) {
        button_K.value = 0;
        xSemaphoreGive(button_K.lock);
    }
}

/* Counts "+1" every second. Starts and stop by button "Z"
   Default state: counting */
void vSimpleCounter (void *pvParameters)
{
    TickType_t last_wake_up = xTaskGetTickCount();

    while(1) {
        if (xTaskGetTickCount() - last_wake_up > COUNTER_PERIOD) {
            if (xSemaphoreTake(simple_counter.lock, 0) == pdTRUE) {
                simple_counter.value += 1;
                xSemaphoreGive(simple_counter.lock);
                last_wake_up = xTaskGetTickCount();
            }        
        }
    }
}

/* Shows value of SimpleCounter */
void vDrawSimpleCounter(buttons_counter_t counter, coord_t string_position,
                        char state, unsigned short *last_value)
{
    static char str[100] = { 0 };
    static unsigned int  state_color = Black;

    if (xSemaphoreTake(counter.lock, 0) == pdTRUE) {
        *last_value = counter.value;
        xSemaphoreGive(counter.lock);
    }

    if (state == 1)
        state_color = Green;
    else
        state_color = Red;

    sprintf(str, "counter value: %d", *last_value);
    tumDrawText(str,string_position.x, string_position.y, state_color);

}
void vThirdExercise (void *pvParameters)
{
    static coord_t first_led_position = {SCREEN_WIDTH / 2 - 2 * LED_SIZE, SCREEN_HEIGHT / 2},
                   second_led_position = {SCREEN_WIDTH / 2 + 2 * LED_SIZE, SCREEN_HEIGHT / 2},
                   L_counter_position = {DEFAULT_FONT_SIZE, SCREEN_HEIGHT - 4 * DEFAULT_FONT_SIZE},
                   K_counter_position = {DEFAULT_FONT_SIZE, SCREEN_HEIGHT - 2 * DEFAULT_FONT_SIZE},
                   simple_counter_position = {DEFAULT_FONT_SIZE, DEFAULT_FONT_SIZE};
    
    static unsigned short current_L_counter = 0,
                          current_K_counter = 0,
                          current_simple_counter = 0;

    // Task resume/suspend control
    static char is_suspended = 0;

    while(1) {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE) {
                xGetButtonInput();
                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                tumDrawClear(White);
                drawHelpText(DEFAULT_FONT_SIZE);
                vDrawFPS();
                vDrawLed(first_led, first_led_position, Green);
                vDrawLed(second_led, second_led_position, Red);
                vButtonPressedSemaphore(PressedSignalL, KEYCODE(L));

                // Notifying "Button K pressed" action by using xTaskNotifyGive
                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                    if (buttons.buttons[KEYCODE(K)]) {
                        buttons.buttons[KEYCODE(K)] = 0;
                        xTaskNotifyGive(ButtonPressedK);
                        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
                    }
                    xSemaphoreGive(buttons.lock);
                }
                
                // Start/Stop counter by pressing "S" button
                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                    if (buttons.buttons[KEYCODE(Z)]) {
                        buttons.buttons[KEYCODE(Z)] = 0;
                        if (is_suspended) {
                            vTaskResume(SimpleCounter);
                            is_suspended = 0;
                        }
                        else {
                            vTaskSuspend(SimpleCounter);
                            is_suspended = 1;
                        }    
                    }
                    xSemaphoreGive(buttons.lock);
                }

                vDrawSimpleCounter(simple_counter, simple_counter_position, 
                                  !is_suspended, &current_simple_counter); 
                vDrawButtonCounter(button_L, L_counter_position, "L", &current_L_counter);
                vDrawButtonCounter(button_K, K_counter_position, "K", &current_K_counter);

                xSemaphoreGive(ScreenLock);
                vCheckStateInput();
            }
    }
}

/* Fourth Exercise (Task 3.3.2) */
// First task. Outputs "1" every tick
void vAlpha (void *pvParameters)
{
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();

    static unsigned short index = 0;
    static unsigned char res = 0;
    static unsigned char is_finished = 0;

    static unsigned char task_return = 1;
    if (xSemaphoreTake(output.lock, 0) == pdTRUE) {
        output.start_tick = xTaskGetTickCount();
        xSemaphoreGive(output.lock);    
    }

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
                output.stop_tick = xTaskGetTickCount();
            }
            xSemaphoreGive(output.lock);
        }
        if (is_finished) {
            vTaskResume(FourthExercise);
            vTaskResume(SwapBuffers);
            vTaskResume(StateMachine);
            
            vTaskResume(SimpleCounter);
            vTaskResume(ButtonPressedK);
            vTaskResume(ButtonPressedL);
            vTaskSuspend( NULL );
        }
    }
}

// Second task. Outputs "2" every second tick and
// calls Third task by binary semaphore
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

// Third task. Outputs "3" when called by Second task.
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

// Fourth task. Outputs "4" every 4 ticks.
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

void vFourthExercise (void *pvParameters)
{
    ResultQueue = xQueueCreate(TASKS_TO_SHOW, sizeof( unsigned char) );

    xTaskCreate(vAlpha, "Task1", mainGENERIC_STACK_SIZE, NULL, 1, &Alpha);

    xTaskCreate(vBeta, "Task2", mainGENERIC_STACK_SIZE, NULL, 2, &Beta);

    xTaskCreate(vGamma, "Task3", mainGENERIC_STACK_SIZE, NULL, 3, &Gamma);
    GammaSignal = xSemaphoreCreateBinary();

    xTaskCreate(vDelta, "Task4", mainGENERIC_STACK_SIZE, NULL, 4, &Delta);

    vTaskSuspend(Alpha);
    vTaskSuspend(Beta);
    vTaskSuspend(Gamma);
    vTaskSuspend(Delta);

    vTaskSuspend(SwapBuffers);
    vTaskSuspend(StateMachine);
    vTaskSuspend(SimpleCounter);
    vTaskSuspend(ButtonPressedK);
    vTaskSuspend(ButtonPressedL);
    vTaskSuspend(FirstLedControlStatic);
    vTaskSuspend(SecondLedControl);

    vTaskResume(Alpha);
    vTaskResume(Beta);
    vTaskResume(Gamma);
    vTaskResume(Delta);
    vTaskSuspend( NULL );
    
    static unsigned char is_fetched = 0;

    char values_to_show[TICKS_TO_SHOW][20] = { 0 };
    char ticks_counter_string[50] = { 0 };

    while(1) {     
        
        if (!is_fetched)
                {
                    if (xSemaphoreTake(output.lock, 0) == pdTRUE) {
                        for (int i = 0; i < TICKS_TO_SHOW; i++) {
                            sprintf(values_to_show[i], "TICK #%d: %d%d%d%d", 
                                    i,
                                    output.values[i][0],
                                    output.values[i][1],
                                    output.values[i][2],
                                    output.values[i][3]);
                        }
                        sprintf(ticks_counter_string, "Executed in: %d - %d = %d real ticks",
                                output.stop_tick, output.start_tick, 
                                output.stop_tick - output.start_tick);
                        xSemaphoreGive(output.lock);    
                    }
                    is_fetched = 1;
                }

        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE) {
                xGetButtonInput();
                xSemaphoreTake(ScreenLock, portMAX_DELAY); 
                tumDrawClear(White);
                drawHelpText(DEFAULT_FONT_SIZE);
                vDrawFPS();
            
                for (int i = 0; i < TICKS_TO_SHOW - 1; i++)
                {
                    tumDrawText(values_to_show[i], DEFAULT_FONT_SIZE, 
                                1.5 * (i+1) *DEFAULT_FONT_SIZE, Black);
                }
                
                tumDrawText(ticks_counter_string, DEFAULT_FONT_SIZE,
                            25 * DEFAULT_FONT_SIZE, Black);
                xSemaphoreGive(ScreenLock);
                vCheckStateInput();
            }
    }
}

#define PRINT_TASK_ERROR(task) PRINT_ERROR("Failed to print task ##task");

int main (int argc, char *argv[])
{
    char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);
    printf("Initializing: ");
    
    /* Initializing all necessary libraries w/o error check */
    tumDrawInit(bin_folder_path);
    tumEventInit();

    tumFontSetSize((ssize_t)20);
    
    /* StateMachine Task */
    // State machine queue
    StateQueue = xQueueCreate(STATE_QUEUE_LENGTH, sizeof(unsigned char));
    if (!StateQueue) {
        PRINT_ERROR("Could not open state queue");
    }
    
    if (xTaskCreate(basicSequentialStateMachine, "StateMachine",
                    mainGENERIC_STACK_SIZE * 2, NULL, 
                    configMAX_PRIORITIES - 1, &StateMachine) != pdPASS) {
        PRINT_ERROR("FSM Task");
                    }

    /* Screen updating mechanism initialization */
    xTaskCreate(vSwapBuffers, "BufferSwap", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES, &SwapBuffers);
    DrawSignal = xSemaphoreCreateBinary();
    ScreenLock = xSemaphoreCreateMutex();
    // Keyboard MUTEX
    buttons.lock = xSemaphoreCreateMutex();

    /* Exercise 2 */
    xTaskCreate(vSecondExercise, "Exercise2", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES - 1, &SecondExercise);
    

    /* Exercise 3 */
    xTaskCreate(vThirdExercise, "Exercise3", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES - 1, &ThirdExercise);
    
    // Led control tasks and mutexes
    FirstLedControlStatic = xTaskCreateStatic(vFirstLedControl,
                                            "1HzLed",      
                                            mainGENERIC_STACK_SIZE,  
                                            ( void * ) 1,
                                            configMAX_PRIORITIES,
                                            xFirstLedControlStack,  
                                            &xFirstLedControlBuffer );

    xTaskCreate(vSecondLedControl, "2HzLed", mainGENERIC_STACK_SIZE,
                NULL, configMAX_PRIORITIES - 1, &SecondLedControl);
    
    first_led.lock = xSemaphoreCreateMutex();
    second_led.lock = xSemaphoreCreateMutex();
    
    // Button-triggerable tasks (enabling via synchronization mechanisms)
    xTaskCreate(vButtonPressedL, "L_pressed_control", mainGENERIC_STACK_SIZE,
                NULL, configMAX_PRIORITIES, &ButtonPressedL);
    PressedSignalL = xSemaphoreCreateBinary();
    button_L.lock = xSemaphoreCreateMutex();

    xTaskCreate(vButtonPressedK, "K_pressed_control", mainGENERIC_STACK_SIZE,
                NULL, configMAX_PRIORITIES, &ButtonPressedK);
    button_K.lock = xSemaphoreCreateMutex();

    // Timer initialization
    ResetTimer = xTimerCreate("ResetButtonsKandL", RESET_TIMER_PERIOD, 
                            pdTRUE, ( void *) 0, vResetTimerCallback);
    if (ResetTimer == NULL) {
        PRINT_ERROR("ResetTimer");
    }
    else {
        if (xTimerStart(ResetTimer,0) != pdPASS) {
            PRINT_ERROR("TimerActivation");
        }
    }

    // 1 second counter task initialization
    xTaskCreate(vSimpleCounter, "1SecondCounter", mainGENERIC_STACK_SIZE,
                NULL, configMAX_PRIORITIES, &SimpleCounter);
    simple_counter.lock = xSemaphoreCreateMutex();


    /* Fourth Exercise */
    xTaskCreate(vFourthExercise, "FourthExercise", mainGENERIC_STACK_SIZE * 2,
                NULL, configMAX_PRIORITIES - 1, &FourthExercise);

    output.lock = xSemaphoreCreateMutex();

    vTaskSuspend(SecondExercise);
    
    vTaskSuspend(ThirdExercise);
    vTaskSuspend(FirstLedControlStatic);
    vTaskSuspend(SecondLedControl);

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
