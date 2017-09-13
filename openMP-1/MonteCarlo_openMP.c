#include <assert.h>
#include <malloc.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>


int main(int argc, char **argv) {
	int a, b, x, N, P;
	double p;
	
	int position;
	unsigned int i, seed;
	unsigned int *seeds = NULL;
	double random_num, work_time, probab_b, avg_lifetime;
	struct timeval time_start, time_end, time_for_rand;
	FILE *file;
	
	if (argc != 7) {
		printf("Wrong amount of arguments");
		return 1;
	} 
		
	a = atoi(argv[1]);
	b = atoi(argv[2]);
	x = atoi(argv[3]);
	N = atoi(argv[4]);
	p = atof(argv[5]);
	P = atoi(argv[6]);
	
	
	//since the program is probably launched more 
	//than once per second
	gettimeofday(&time_for_rand, NULL);
	srand((time_for_rand.tv_sec * 1000) + 
		  (time_for_rand.tv_usec / 1000));
	
	seeds = (int*)malloc(P * sizeof(int));
	for (i = 0; i < P; i++)
		seeds[i] = rand();
		
	
	assert(gettimeofday(&time_start, NULL) == 0);
		
	avg_lifetime = 0;
	probab_b = 0;
		
	omp_set_num_threads(P);
	#pragma omp parallel for reduction(+:probab_b, avg_lifetime) \
							private(seed, random_num, position)
	for (i = 0; i < N; i++) {
		position = x;
			
		seed = seeds[omp_get_thread_num()]; 
			
		while (position != a && position != b) {
			random_num = (double)rand_r(&seed) / 
						((double)RAND_MAX / (double)100);
				
			if (random_num < 100 * p) {
				position++;
			} 
			else {
				position--;
			}
				
			avg_lifetime++;
		}
			
		if (position == b) {
			probab_b++;
		}
	}
		
	probab_b = probab_b / N;
	avg_lifetime = avg_lifetime / N;
		
		
	assert(gettimeofday(&time_end, NULL) == 0);
		
	work_time = ((time_end.tv_sec - time_start.tv_sec) * 1000000u + 
				  time_end.tv_usec - time_start.tv_usec) / 1.e6;
		
	
	free(seeds);
					  
	file = fopen("stats.txt", "w");
	fprintf(file, "%f %.1f %f %d %d %d %d %f %d\n",
			probab_b, avg_lifetime, work_time, a, b, x, N, p, P);
	fclose(file);
	
	return 0;
}
