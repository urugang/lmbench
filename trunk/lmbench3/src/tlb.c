/*
 * tlb.c - guess the cache line size
 *
 * usage: tlb
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
	char*	addr;
	char*	p;
	int	pages;
	int	pagesize;
	int	warmup;
	int	repetitions;
};

void compute_times(struct _state* state, double* tlb_time, double* cache_time);
void initialize_tlb(void* cookie);
void initialize_cache(void* cookie);
void benchmark(uint64 iterations, void* cookie);
void cleanup(void* cookie);

#define	ONE	p = (char **)*p;
#define	FIVE	ONE ONE ONE ONE ONE
#define	TEN	FIVE FIVE
#define	FIFTY	TEN TEN TEN TEN TEN
#define	HUNDRED	FIFTY FIFTY

/*
 * Assumptions:
 *
 * 1) Cache lines are a multiple of pointer-size words
 * 2) Cache lines are smaller than 1/2 a page size
 * 3) Pages are an even multiple of cache lines
 */
int
main(int ac, char **av)
{
	int	i, l, len, tlb, maxpages;
	int	c;
	int	print_cost = 0;
	int	maxline = getpagesize() / sizeof(char*);
	double tlb_time, cache_time, diff;
	struct _state state;
	char   *usage = "[-c] [-M len[K|M]]\n";

	maxpages = 16 * 1024;
	state.pagesize = getpagesize();
	state.warmup = 0;
	state.repetitions = TRIES;

	tlb = 2;

	while (( c = getopt(ac, av, "cM:")) != EOF) {
		switch(c) {
		case 'c':
			print_cost = 1;
			break;
		case 'M':
			l = strlen(optarg);
			if (optarg[l-1] == 'm' || optarg[l-1] == 'M') {
				maxpages = 1024 * 1024;
				optarg[l-1] = 0;
			} else if (optarg[l-1] == 'k' || optarg[l-1] == 'K') {
				maxpages = 1024;
				optarg[l-1] = 0;
			}
			maxpages *= atoi(optarg);
			break;
		case 'W':
			state.warmup = atoi(optarg);
			break;
		case 'N':
			state.repetitions = atoi(optarg);
			break;
		default:
			lmbench_usage(ac, av, usage);
			break;
		}
	}

/*
	state.pages = 4;
	initialize_cache(&state);
	benchmark(1000, &state);
	cleanup(&state);
	exit(0);
/**/

	for (i = 2; i <= maxpages; i<<=1) {
		state.pages = i;
		compute_times(&state, &tlb_time, &cache_time);

		if (tlb_time / cache_time > 1.25) {
			i>>=1;
			break;
		}
		tlb = state.pages;
	}

	/* we can't find any tlb effect */
	if (i == maxpages)
		exit(0);

	for (++i; i <= maxpages; ++i) {
		state.pages = i;
		compute_times(&state, &tlb_time, &cache_time);

		if (tlb_time / cache_time > 1.25) {
			if (print_cost) {
				state.pages *= 2;
				compute_times(&state, &tlb_time, &cache_time);
			}
			break;
		}
		tlb = state.pages;
	}

	for (i = 2; i <= maxpages; i<<=1) {
		state.pages = i;
		compute_times(&state, &tlb_time, &cache_time);
	}

	if (print_cost) {
		fprintf(stderr, "tlb: %d pages %.5f nanoseconds\n", tlb, tlb_time - cache_time);
	} else {
		fprintf(stderr, "tlb: %d pages\n", tlb);
	}

	return(0);
}

void
compute_times(struct _state* state, double* tlb_time, double* cache_time)
{
	benchmp(initialize_tlb, benchmark, cleanup, 0, 1, 
		state->warmup, state->repetitions, state);
	
	/* We want nanoseconds / load. */
	*tlb_time = (1000. * (double)gettime()) / (100. * (double)get_n());

	benchmp(initialize_cache, benchmark, cleanup, 0, 1,
		state->warmup, state->repetitions, state);
	
	/* We want nanoseconds / load. */
	*cache_time = (1000. * (double)gettime()) / (100. * (double)get_n());

	fprintf(stderr, "%d %.5f %.5f\n", state->pages, *tlb_time, *cache_time);
}

/*
 * This will access one word per page, for a total of len bytes of accessed
 * memory.
 */
void
initialize_tlb(void* cookie)
{
	int i, npointers;
	unsigned int r;
	char ***pages;
	int    *lines;
	struct _state* state = (struct _state*)cookie;
	register char *p = 0 /* lint */;

	npointers = state->pagesize / sizeof(char*);

	state->p = state->addr = (char*)malloc((state->pages + 1) * state->pagesize);
	pages = (char***)malloc(state->pages * sizeof(char**));
	lines = (int*)malloc(npointers * sizeof(int));

	if (state->addr == NULL || pages == NULL || lines == NULL) {
		exit(0);
	}

	if ((unsigned long)state->p % state->pagesize) {
		state->p += state->pagesize - (unsigned long)state->p % state->pagesize;
	}

	srand(getpid());

	/* first, layout the sequence of page accesses */
	p = state->p;
	for (i = 0; i < state->pages; ++i) {
		pages[i] = (char**)p;
		p += state->pagesize;
	}

	/* randomize the page sequences (except for zeroth page) */
	r = (rand() << 15) ^ rand();
	for (i = state->pages - 2; i > 0; --i) {
		char** l;
		r = (r << 1) ^ (rand() >> 4);
		l = pages[(r % i) + 1];
		pages[(r % i) + 1] = pages[i + 1];
		pages[i + 1] = l;
	}

	/* layout the sequence of line accesses */
	for (i = 0; i < npointers; ++i) {
		lines[i] = i * state->pagesize / (npointers * sizeof(char*));
	}

	/* randomize the line sequences */
	for (i = npointers - 2; i > 0; --i) {
		int l;
		r = (r << 1) ^ (rand() >> 4);
		l = lines[(r % i) + 1];
		lines[(r % i) + 1] = lines[i];
		lines[i] = l;
	}

	/* new setup run through the pages */
	for (i = 0; i < state->pages - 1; ++i) {
		pages[i][lines[i % npointers]] = (char*)(pages[i+1] + lines[(i+1) % npointers]);
	}
	pages[state->pages - 1][lines[(state->pages - 1) % npointers]] = (char*)(pages[0] + lines[0]);

	free(pages);
	free(lines);

	/* run through the chain once to clear the cache */
	benchmark((state->pages + 100) / 100, state);
}

/*
 * This will access len bytes
 */
void
initialize_cache(void* cookie)
{
	int i, j, nlines, nbytes, npages, npointers;
	unsigned int r;
	char ***pages;
	int    *lines;
	struct _state* state = (struct _state*)cookie;
	register char *p = 0 /* lint */;

	nbytes = state->pages * sizeof(char*);
	nlines = state->pagesize / sizeof(char*);
	npages = (nbytes + state->pagesize) / state->pagesize;

	lines = (int*)malloc(nlines * sizeof(int));
	pages = (char***)malloc(npages * sizeof(char**));
	state->p = state->addr = (char*)malloc(nbytes + 2 * state->pagesize);

	if ((unsigned long)state->p % state->pagesize) {
		state->p += state->pagesize - (unsigned long)state->p % state->pagesize;
	}

	if (state->addr == NULL || pages == NULL) {
		exit(0);
	}

	srand(getpid());

	/* first, layout the sequence of page accesses */
	p = state->p;
	for (i = 0; i < npages; ++i) {
		pages[i] = (char**)p;
		p += state->pagesize;
	}

	/* randomize the page sequences (except for zeroth page) */
	r = (rand() << 15) ^ rand();
	for (i = npages - 2; i > 0; --i) {
		char** l;
		r = (r << 1) ^ (rand() >> 4);
		l = pages[(r % i) + 1];
		pages[(r % i) + 1] = pages[i + 1];
		pages[i + 1] = l;
	}

	/* layout the sequence of line accesses */
	for (i = 0; i < nlines; ++i) {
		lines[i] = i * state->pagesize / (nlines * sizeof(char*));
	}

	/* randomize the line sequences */
	for (i = nlines - 2; i > 0; --i) {
		int l;
		r = (r << 1) ^ (rand() >> 4);
		l = lines[(r % i) + 1];
		lines[(r % i) + 1] = lines[i];
		lines[i] = l;
	}

	/* setup the run through the pages */
	for (i = 0, npointers = 0; i < npages; ++i) {
		for (j = 0; j < nlines - 1 && npointers < state->pages - 1; ++j) {
			pages[i][lines[j]] = (char*)(pages[i] + lines[j+1]);
			npointers++;
		}
		if (i == npages - 1 || npointers == state->pages - 1) {
			pages[i][lines[j]] = (char*)(pages[0] + lines[0]);
		} else {
			pages[i][lines[j]] = (char*)(pages[i+1] + lines[0]);
		}
		npointers++;
	}

	free(pages);
	free(lines);

	/* run through the chain once to clear the cache */
	benchmark((npages * nlines + 100) / 100, state);
}


void benchmark(uint64 iterations, void *cookie)
{
	struct _state* state = (struct _state*)cookie;
	static char **p_save = NULL;
	register char **p = p_save ? p_save : (char**)state->p;

	while (iterations-- > 0) {
		HUNDRED;
	}

	use_pointer((void *)p);
	p_save = p;
}

void cleanup(void* cookie)
{
	struct _state* state = (struct _state*)cookie;
	free(state->addr);
	state->addr = NULL;
	state->p = NULL;
}


