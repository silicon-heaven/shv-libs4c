#include <math.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int wire_to_double(double *pval, uint_least64_t onwire)
{
	double val;
	uint_least64_t m;
	int e;
	int s = 0;

	m = onwire & (((uint_least64_t)1 << 52) - 1);
	s = (int)((onwire >> 63) & 1);
	e = (onwire >> 52) & ((1 << 11) - 1);

	if (e == ((1 << 11) - 1)) {
		if (m)
			val = (double)NAN;
		else
			val = (double)INFINITY;
	} else if(!e && !m) {
		val = 0.0;
	} else {
		if (e) {
			m |= ((uint_least64_t)1 << 52);
		} else {
			e = 1;
		}
		e -= ((1 << 10) - 1 + 52);
		val = ldexp((double)m, e);
	}

	if(s)
		val = -val;

	*pval = val;
	return 0;
}

#if __MINGW32__
// MinGW is acting silly and is not able to select the correct function in the signbit/isnan/fpclassify macro. Since
// we're only dealing with doubles, we'll just redefine everything to the __ versions (which are the double versions).
// https://sourceforge.net/p/mingw-w64/bugs/481/
#undef signbit
#undef isnan
#undef fpclassify
#define signbit __signbit
#define isnan __isnan
#define fpclassify __fpclassify
#endif

int wire_from_double(uint_least64_t *ponwire, double val)
{
	uint_least64_t onwire;
	int e = 0;
	int s = 0;

	if(signbit(val)) {
		s = 1;
		val = -val;
	}

	if (isnan(val)) {
		onwire = ((uint_least64_t)1 << 63) - 1;
	} else if (isinf(val)) {
		onwire = (((uint_least64_t)1 << 11) - 1) << 52;
	} else if (val != 0.0) {
		val = frexp(val, &e);
		e += (1 << 10) - 2;
		if (e > 0) {
			onwire = (uint_least64_t)(ldexp (val, 53));
			onwire &= ~((uint_least64_t)1 << 52);
		} else {
			if(e >= -52 + 1)
				onwire = (uint_least64_t)(ldexp (val, 53 - 1 + e));
			else
				onwire = 0;
			e = 0;
		}
		onwire |= (uint_least64_t)e << 52;
	} else {
		onwire = 0;
	}
	onwire |= (uint_least64_t)s << 63;
	*ponwire = onwire;
	return 0;
}

double test_array[] = {0.0, -0.0, 1.0, -1.0, 1234.56789, -1234.56789, 1.23e-308, -1.23e-308, +INFINITY, -INFINITY, NAN, -NAN};
int test_array_size = sizeof(test_array) / sizeof(*test_array);

typedef struct {
	double base;
	double factor;
} test_base_factor_t;

test_base_factor_t test_base_factor[] = {
	{1.56789, 0.5},
	{1.56789, 2.0},
	{-1.56789, 0.5},
	{-1.56789, 2.0},
	{1.26789, 0.5},
	{1.26789, 2.0},
	{-1.26789, 0.5},
	{-1.26789, 2.0},
};

int test_base_factor_size = sizeof(test_base_factor) / sizeof(*test_base_factor);

int main(void)
{
	uint_least64_t onwire;
	double ref;
	double val;
	int i;
	for (i = 0; i < test_array_size; i++) {
		ref = test_array[i];
		printf("In:  %g\t\t%a\t\t%016llx\t\t%016llx\n", ref, ref, (unsigned long long)*(uint64_t*)&ref, (unsigned long long)*(uint64_t*)&ref);
		wire_from_double(&onwire, ref);
		wire_to_double(&val, onwire);
		printf("Out: %g\t\t%a\t\t%016llx\t\t%016llx\n", val, val, (unsigned long long)onwire, (unsigned long long)*(uint64_t*)&val);
	}

	printf("\nAny print below this point is an error\n");

	for (i = 0 ; i < test_base_factor_size; i++) {
		for (ref = test_base_factor[i].base; (ref != 0.0) && (!isinf(ref));
			 ref *= test_base_factor[i].factor) {
			wire_from_double(&onwire, ref);
			wire_to_double(&val, onwire);
			if ((double)val != (double)ref) {
				printf("In:  %g\t\t%a\t\t%016llx\t\t%016llx\n", ref, ref, (unsigned long long)*(uint64_t*)&ref, (unsigned long long)*(uint64_t*)&ref);
				printf("Out: %g\t\t%a\t\t%016llx\t\t%016llx\n", val, val, (unsigned long long)onwire, (unsigned long long)*(uint64_t*)&val);
			}
		}
	}
	return 0;
}
