#include "current_limit_algo_utils.h"
#include <assert.h>
#include <math.h>

/* Default epsilon for generic float comparisons */
#define FLOAT_EPSILON (0.01f)

/**
 * @brief Check if two floating-point values are not equal within a tolerance.
 *
 * @param a   First value
 * @param b   Second value
 * @param eps Allowed absolute difference
 *
 * @return true if values differ beyond tolerance, false otherwise
 */
static bool float_not_equal(float a, float b, float eps)
{
	return (fabs(a - b) > eps);
}

float linear_interpolate(float x, float x1, float x2, float y1, float y2)
{
	assert(float_not_equal(x2, x1, FLOAT_EPSILON) == true);

	return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}