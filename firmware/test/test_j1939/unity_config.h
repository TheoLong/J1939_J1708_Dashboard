/**
 * @file unity_config.h
 * @brief Unity Test Framework configuration for native builds
 */

#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Enable double support for floating point tests
#ifndef UNITY_INCLUDE_DOUBLE
#define UNITY_INCLUDE_DOUBLE 1
#endif

// Enable float comparison with delta
#ifndef UNITY_INCLUDE_FLOAT
#define UNITY_INCLUDE_FLOAT 1
#endif

// Use standard output
#include <stdio.h>

#define UNITY_OUTPUT_CHAR(c) putchar(c)
#define UNITY_OUTPUT_START()
#define UNITY_OUTPUT_FLUSH() fflush(stdout)
#define UNITY_OUTPUT_COMPLETE()

#endif // UNITY_CONFIG_H
