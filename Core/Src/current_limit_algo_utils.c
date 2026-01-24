#include "current_limit_algo_utils.h"
#include <assert.h>
#include <math.h>

bool float_is_equal(float a, float b, float eps)
{
	return (fabsf(a - b) <= eps);
}

float linear_interpolate(float x, float x1, float x2, float y1, float y2)
{
	assert(!float_is_equal(x2, x1, FLOAT_EPSILON));

	return y1 + ((x - x1) * (y2 - y1) / (x2 - x1));
}