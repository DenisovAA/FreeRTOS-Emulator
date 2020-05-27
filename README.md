# FreeRTOS Emulator

## short README

- Press [Q] to exit
- Press [E] to change between screens (exercise solutions)

### Exercise 3
- Press [K] to count buttons-pressed actions by using xTaskNotify (Task 3.2.3)
- Press [L] to count buttons-pressed actions by using Binary Semaphore (Task 3.2.3)
- Press [Z] to start/stop "1 second counter" (Task 3.2.4)

### Exercise 3.3.2
The result of 15 measures is displayed on the screen once.
The line “Executed in...” shows the difference between the real tick of the first call of "Task 1" and the real tick of the last, 15th call of "Task 1".

## Answers to the questions

### What is the kernel tick?
-The kernel tick is a periodical interruption. After passing the specified "tick time" the tick count is incremented by 1
and kernel have to check which tasks should be now blocked, unblocked or interrupted.
In real-time systems, the time between ticks is controlled by a hardware timer and period is specified by engineer.

### What is the tickless kernel?
-The tickless kernel has no periodical interruptions. Interrupts are generated as needed. This approach, for example, allows to saves energy.

### Stack Size
Allocating a stack of the wrong size leads to a "Sigmentation Fault" error when trying to run an emulator. In this case, the build and compilation are successful.


### Scheduling Insights
Changing the priorities of tasks leads to a change in the order of digits in the output lines, since it changes the order in which tasks are called during the kernel tick. 
