#ifndef CURRENT_LIMIT_ALGO_UTILS_H
#define CURRENT_LIMIT_ALGO_UTILS_H

#include <stdbool.h>

/* Default epsilon for generic float comparisons */
#define FLOAT_EPSILON (0.0001f)

/**
 * @brief Check if two floating-point values are equal within a tolerance.
 *
 * Two floating-point values are considered equal if the absolute difference
 * between them is less than or equal to the specified tolerance.
 *
 * @param a   First value
 * @param b   Second value
 * @param eps Allowed absolute difference
 *
 * @return true if values are equal within tolerance, false otherwise
 */
bool float_is_equal(float a, float b, float eps);

/**
 * @brief Performs linear interpolation between two points.
 *
 * Computes the interpolated y-value corresponding to x, given two
 * reference points (x1, y1) and (x2, y2).
 *
 * @param x   Input value at which to interpolate.
 * @param x1  First reference x-coordinate.
 * @param x2  Second reference x-coordinate.
 * @param y1  y-value at x1.
 * @param y2  y-value at x2.
 *
 * @return Interpolated y-value.
 */
float linear_interpolate(float x, float x1, float x2, float y1, float y2);

#endif // CURRENT_LIMIT_ALGO_UTILS_H