/*
 * par_ops.c - benchmark of simple operation parallelism
 *
 * Copyright (c) 2000 Carl Staelin.
 * Copyright (c) 1994 Larry McVoy.  Distributed under the FSF GPL with
 * additional restriction that results may published only if
 * (1) the benchmark is unmodified, and
 * (2) the version in the sccsid below is included in the report.
 * Support for this development by Sun Microsystems is gratefully acknowledged.
 */
char	*id = "$Id$\n";

#include "bench.h"

struct _state {
	int	N;
	int	M;
	int*	int_data;
	double*	double_data;
};

void	initialize(void* cookie);
void	cleanup(void* cookie);

#define	FIVE(m)		m m m m m
#define	TEN(m)		FIVE(m) FIVE(m)
#define	FIFTY(m)	TEN(m) TEN(m) TEN(m) TEN(m) TEN(m)
#define	HUNDRED(m)	FIFTY(m) FIFTY(m)

#define MAX_LOAD_PARALLELISM 16

double
max_parallelism(bench_f* benchmarks, 
		int warmup, int repetitions, void* cookie)
{
	int		i, j, k;
	double		baseline, max_load_parallelism, load_parallelism;
	result_t	*results, *r_save;

	max_load_parallelism = 1.;

	for (i = 0; i < MAX_LOAD_PARALLELISM; ++i) {
		benchmp(initialize, benchmarks[i], cleanup, 
			0, 1, warmup, repetitions, cookie);

		if (gettime() == 0)
			return -1.;

		if (i == 0) {
			baseline = (double)gettime() / (double)get_n();
		} else {
			load_parallelism = baseline;
			load_parallelism /= (double)gettime();
			load_parallelism *= (double)((i + 1) * get_n());
			if (load_parallelism > max_load_parallelism) {
				max_load_parallelism = load_parallelism;
			}
		}
	}
	return max_load_parallelism;
}

#define REPEAT_0(m)	m(0)
#define REPEAT_1(m)	REPEAT_0(m) m(1)
#define REPEAT_2(m)	REPEAT_1(m) m(2)
#define REPEAT_3(m)	REPEAT_2(m) m(3)
#define REPEAT_4(m)	REPEAT_3(m) m(4)
#define REPEAT_5(m)	REPEAT_4(m) m(5)
#define REPEAT_6(m)	REPEAT_5(m) m(6)
#define REPEAT_7(m)	REPEAT_6(m) m(7)
#define REPEAT_8(m)	REPEAT_7(m) m(8)
#define REPEAT_9(m)	REPEAT_8(m) m(9)
#define REPEAT_10(m)	REPEAT_9(m) m(10)
#define REPEAT_11(m)	REPEAT_10(m) m(11)
#define REPEAT_12(m)	REPEAT_11(m) m(12)
#define REPEAT_13(m)	REPEAT_12(m) m(13)
#define REPEAT_14(m)	REPEAT_13(m) m(14)
#define REPEAT_15(m)	REPEAT_14(m) m(15)

#define BODY(N)		p##N = (char**)*p##N;
#define DECLARE(N)	static char **sp##N; register char **p##N;
#define INIT(N)		p##N = (addr_save==state->addr) ? sp##N : (char**)state->p[N];
#define SAVE(N)		sp##N = p##N;

#define BENCHMARK(benchmark,N,repeat)					\
void benchmark##_##N(iter_t iterations, void *cookie) 			\
{									\
	register iter_t i = iterations;					\
	struct _state* state = (struct _state*)cookie;			\
	repeat(DECLARE);						\
									\
	repeat(INIT);							\
	while (i-- > 0) {						\
		TEN(repeat(BODY));					\
	}								\
									\
	repeat(SAVE);							\
}

#define PARALLEL_BENCHMARKS(benchmark)					\
	BENCHMARK(benchmark, 0, REPEAT_0)				\
	BENCHMARK(benchmark, 1, REPEAT_1)				\
	BENCHMARK(benchmark, 2, REPEAT_2)				\
	BENCHMARK(benchmark, 3, REPEAT_3)				\
	BENCHMARK(benchmark, 4, REPEAT_4)				\
	BENCHMARK(benchmark, 5, REPEAT_5)				\
	BENCHMARK(benchmark, 6, REPEAT_6)				\
	BENCHMARK(benchmark, 7, REPEAT_7)				\
	BENCHMARK(benchmark, 8, REPEAT_8)				\
	BENCHMARK(benchmark, 9, REPEAT_9)				\
	BENCHMARK(benchmark, 10, REPEAT_10)				\
	BENCHMARK(benchmark, 11, REPEAT_11)				\
	BENCHMARK(benchmark, 12, REPEAT_12)				\
	BENCHMARK(benchmark, 13, REPEAT_13)				\
	BENCHMARK(benchmark, 14, REPEAT_14)				\
	BENCHMARK(benchmark, 15, REPEAT_15)				\
									\
	bench_f benchmark##_benchmarks[] = {				\
		benchmark##_0,						\
		benchmark##_1,						\
		benchmark##_2,						\
		benchmark##_3,						\
		benchmark##_4,						\
		benchmark##_5,						\
		benchmark##_6,						\
		benchmark##_7,						\
		benchmark##_8,						\
		benchmark##_9,						\
		benchmark##_10,						\
		benchmark##_11,						\
		benchmark##_12,						\
		benchmark##_13,						\
		benchmark##_14,						\
		benchmark##_15						\
	};

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N ^= i; r##N<<=1;
#define DECLARE(N)	register int r##N;
#define INIT(N)		r##N = state->int_data[N];
#define SAVE(N)		state->int_data[N] = r##N;
PARALLEL_BENCHMARKS(integer_bit)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#ifndef __GNUC__
			/* required because of an HP ANSI/C compiler bug */
			r##N = ( r##N + i ) ^ r##N;
#else
#define BODY(N)		r##N = r##N + r##N + i;
#endif
#define DECLARE(N)	register int r##N;
#define INIT(N)		r##N = state->int_data[N];
#define SAVE(N)		state->int_data[N] = r##N;
PARALLEL_BENCHMARKS(integer_add)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N = ( r##N * i) ^ r##N;
#define DECLARE(N)	register int r##N;
#define INIT(N)		r##N = state->int_data[N];
#define SAVE(N)		state->int_data[N] = r##N;
PARALLEL_BENCHMARKS(integer_mul)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N = ( r##N / i) ^ r##N;
#define DECLARE(N)	register int r##N;
#define INIT(N)		r##N = state->int_data[N];
#define SAVE(N)		state->int_data[N] = r##N;
PARALLEL_BENCHMARKS(integer_div)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N = ( r##N % i) ^ r##N;
#define DECLARE(N)	register int r##N;
#define INIT(N)		r##N = state->int_data[N];
#define SAVE(N)		state->int_data[N] = r##N;
PARALLEL_BENCHMARKS(integer_mod)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N ^= i; r##N<<=1;
#define DECLARE(N)	register uint64 r##N;
#define INIT(N)		r##N = (uint64)state->int_data[N];
#define SAVE(N)		state->int_data[N] = (int)r##N;
PARALLEL_BENCHMARKS(uint64_bit)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#ifndef __GNUC__
			/* required because of an HP ANSI/C compiler bug */
			r##N = ( r##N + i ) ^ r##N;
#else
#define BODY(N)		r##N = r##N + r##N + i;
#endif
#define DECLARE(N)	register uint64 r##N;
#define INIT(N)		r##N = (uint64)state->int_data[N];
#define SAVE(N)		state->int_data[N] = (int)r##N;
PARALLEL_BENCHMARKS(uint64_add)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N = ( r##N * i) ^ r##N;
#define DECLARE(N)	register uint64 r##N;
#define INIT(N)		r##N = (uint64)state->int_data[N];
#define SAVE(N)		state->int_data[N] = (int)r##N;
PARALLEL_BENCHMARKS(uint64_mul)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N = ( r##N / i) ^ r##N;
#define DECLARE(N)	register uint64 r##N;
#define INIT(N)		r##N = (uint64)state->int_data[N];
#define SAVE(N)		state->int_data[N] = (int)r##N;
PARALLEL_BENCHMARKS(uint64_div)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N = ( r##N % i) ^ r##N;
#define DECLARE(N)	register uint64 r##N;
#define INIT(N)		r##N = (uint64)state->int_data[N];
#define SAVE(N)		state->int_data[N] = (int)r##N;
PARALLEL_BENCHMARKS(uint64_mod)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N += r##N;
#define DECLARE(N)	register float r##N;
#define INIT(N)		r##N = (float)state->double_data[N];
#define SAVE(N)		state->double_data[N] = (double)r##N;
PARALLEL_BENCHMARKS(float_add)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N *= r##N;
#define DECLARE(N)	register float r##N;
#define INIT(N)		r##N = (float)state->double_data[N];
#define SAVE(N)		state->double_data[N] = (double)r##N;
PARALLEL_BENCHMARKS(float_mul)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N = s##N / r##N;
#define DECLARE(N)	register float r##N, s##N;
#define INIT(N)		r##N = (float)state->double_data[N]; s##N = (float)state->int_data[N];
#define SAVE(N)		state->double_data[N] = (double)r##N;
PARALLEL_BENCHMARKS(float_div)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N += r##N;
#define DECLARE(N)	register double r##N;
#define INIT(N)		r##N = state->double_data[N];
#define SAVE(N)		state->double_data[N] = r##N;
PARALLEL_BENCHMARKS(double_add)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N *= r##N;
#define DECLARE(N)	register double r##N;
#define INIT(N)		r##N = state->double_data[N];
#define SAVE(N)		state->double_data[N] = r##N;
PARALLEL_BENCHMARKS(double_mul)

#undef	BODY
#undef	DECLARE
#undef	INIT
#undef	SAVE
#define BODY(N)		r##N = s##N / r##N;
#define DECLARE(N)	register double r##N, s##N;
#define INIT(N)		r##N = state->double_data[N]; s##N = (double)state->int_data[N];
#define SAVE(N)		state->double_data[N] = r##N;
PARALLEL_BENCHMARKS(double_div)


void
initialize(void* cookie)
{
	struct _state *state = (struct _state*)cookie;
	register int i;

	state->int_data = (int*)malloc(MAX_LOAD_PARALLELISM * sizeof(int));
	state->double_data = (double*)malloc(MAX_LOAD_PARALLELISM * sizeof(double));

	for (i = 0; i < MAX_LOAD_PARALLELISM; ++i) {
		state->int_data[i] = (double)(rand() + 1);
		state->double_data[i] = (double)(rand() + 1);
	}
}

void
cleanup(void* cookie)
{
	struct _state *state = (struct _state*)cookie;

	free(state->int_data);
	free(state->double_data);
}


int
main(int ac, char **av)
{
	int	c;
	int	warmup = 0;
	int	repetitions = TRIES;
	double	par;
	struct _state	state;
	char   *usage = "[-W <warmup>] [-N <repetitions>]\n";

	state.N = 1000;
	state.M = 1;

	while (( c = getopt(ac, av, "W:N:")) != EOF) {
		switch(c) {
		case 'W':
			warmup = atoi(optarg);
			break;
		case 'N':
			repetitions = atoi(optarg);
			break;
		default:
			lmbench_usage(ac, av, usage);
			break;
		}
	}

	par = max_parallelism(integer_bit_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "integer bit parallelism: %.2f\n", par);

	par = max_parallelism(integer_add_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "integer add parallelism: %.2f\n", par);

	par = max_parallelism(integer_mul_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "integer mul parallelism: %.2f\n", par);

	par = max_parallelism(integer_div_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "integer div parallelism: %.2f\n", par);

	par = max_parallelism(integer_mod_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "integer mod parallelism: %.2f\n", par);

	par = max_parallelism(uint64_bit_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "uint64 bit parallelism: %.2f\n", par);

	par = max_parallelism(uint64_add_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "uint64 add parallelism: %.2f\n", par);

	par = max_parallelism(uint64_mul_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "uint64 mul parallelism: %.2f\n", par);

	par = max_parallelism(uint64_div_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "uint64 div parallelism: %.2f\n", par);

	par = max_parallelism(uint64_mod_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "uint64 mod parallelism: %.2f\n", par);

	par = max_parallelism(float_add_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "float add parallelism: %.2f\n", par);

	par = max_parallelism(float_mul_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "float mul parallelism: %.2f\n", par);

	par = max_parallelism(float_div_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "float div parallelism: %.2f\n", par);

	par = max_parallelism(double_add_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "double add parallelism: %.2f\n", par);

	par = max_parallelism(double_mul_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "double mul parallelism: %.2f\n", par);

	par = max_parallelism(double_div_benchmarks, 
			      warmup, repetitions, &state);
	if (par > 0.)
		fprintf(stderr, "double div parallelism: %.2f\n", par);


	return(0);
}
