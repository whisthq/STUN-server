#ifndef CLOCK_H
#define CLOCK_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file clock.h
 * @brief Helper functions for timing
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

/*
============================
Defines
============================
*/

#if defined(_WIN32)
#define fractal_clock_t LARGE_INTEGER
#else
#define fractal_clock_t struct timeval
#endif

/*
============================
Public Functions
============================
*/

/**
 * @brief                          Start the given timer at the current time, as
 *                                 a stopwatch
 *
 * @param timer		               Pointer to the the timer in question
 */
void FractalStartTimer(fractal_clock_t* timer);

/**
 * @brief                          Get the amount of elapsed time since the last
 *                                 StartTimer
 *
 * @param timer		               The timer in question
 */
double FractalGetTimer(fractal_clock_t timer);

/**
 * @brief                          Create a clock that represents the given
 *                                 timeout in milliseconds
 *
 * @param timeout_ms	           The number of milliseconds for the clock
 *
 * @returns						   The desired clock
 */
fractal_clock_t FractalCreateClock(int timeout_ms);

/**
 * @brief                          Returns the current time as a string
 *
 * @returns						   The current time as a string
 */
char* FractalCurrentTimeStr();

#endif
