/**
 * Copyright Fractal Computers, Inc. 2020
 * @file clock.h
 * @brief Helper functions for timing
============================
Usage
============================

Use these functions to time code, via FractalStartTimer and FractalGetTimer.
*/

/*
============================
Includes
============================
*/

#include "clock.h"

/*
============================
Defines
============================
*/

#if defined(_WIN32)
LARGE_INTEGER frequency;
bool set_frequency = false;
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
void FractalStartTimer(fractal_clock_t* timer) {
#if defined(_WIN32)
    if (!set_frequency) {
        QueryPerformanceFrequency(&frequency);
        set_frequency = true;
    }
    QueryPerformanceCounter(timer);
#else
    // start timer
    gettimeofday(timer, NULL);
#endif
    return;
}

/**
 * @brief                          Get the amount of elapsed time since the last
 *                                 StartTimer
 *
 * @param timer		               The timer in question
 */
double FractalGetTimer(fractal_clock_t timer) {
#if defined(_WIN32)
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    double ret = (double)(end.QuadPart - timer.QuadPart) / frequency.QuadPart;
#else
    // stop timer
    struct timeval t2;
    gettimeofday(&t2, NULL);

    // compute and print the elapsed time in millisec
    double elapsedTime = (t2.tv_sec - timer.tv_sec) * 1000.0;  // sec to ms
    elapsedTime += (t2.tv_usec - timer.tv_usec) / 1000.0;      // us to ms

    // printf("elapsed time in ms is: %f\n", elapsedTime);

    // standard var to return and convert to seconds since it gets converted to
    // ms in function call
    double ret = elapsedTime / 1000.0;
#endif
    return ret;
}

/**
 * @brief                          Create a clock that represents the given
 *                                 timeout in milliseconds
 *
 * @param timeout_ms	           The number of milliseconds for the clock
 *
 * @returns						   The desired clock
 */
fractal_clock_t FractalCreateClock(int timeout_ms) {
    fractal_clock_t out;
#if defined(_WIN32)
    out.QuadPart = timeout_ms;
#else
    out.tv_sec = timeout_ms / 1000;
    out.tv_usec = (timeout_ms % 1000) * 1000;
#endif
    return out;
}

/**
 * @brief                          Returns the current time as a string
 *
 * @returns						   The current time as a string
 */
char* FractalCurrentTimeStr() {
    static char buffer[64];

#if defined(_WIN32)
    SYSTEMTIME time_now;
    GetSystemTime(&time_now);
    snprintf(buffer, sizeof(buffer), "%02i:%02i:%02i:%03i", time_now.wHour,
             time_now.wMinute, time_now.wSecond, time_now.wMilliseconds);
#else
    struct tm* time_str_tm;
    struct timeval time_now;
    gettimeofday(&time_now, NULL);

    time_str_tm = gmtime(&time_now.tv_sec);
    snprintf(buffer, sizeof(buffer), "%02i:%02i:%02i:%06li",
             time_str_tm->tm_hour, time_str_tm->tm_min, time_str_tm->tm_sec,
             (long)time_now.tv_usec);
#endif
    return buffer;
}
