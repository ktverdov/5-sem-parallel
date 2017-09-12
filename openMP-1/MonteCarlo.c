#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

int main(int argc, char **argv) {
	int a, b, x, N, P;
	double p;
	
	int *positions = NULL;
	int *lifetimes = NULL;
	unsigned int i;
	double random_num, work_time, probab_b, avg_lifetime;
	struct timeval time_start, time_end;
	FILE *file;
	
	if (argc != 7) {
		printf("Wrong amount of arguments");
	} 
	else {
		a = atoi(argv[1]);
		b = atoi(argv[2]);
		x = atoi(argv[3]);
		N = atoi(argv[4]);
		p = atof(argv[5]);
		P = atoi(argv[6]);
		
		positions = (int*)malloc(N * sizeof(int));
		for (i = 0; i < N; i++)
			positions[i] = x;
			
		lifetimes = (int*)malloc(N * sizeof(int));
		for (i = 0; i < N; i++)
			lifetimes[i] = 0;
		
		assert(gettimeofday(&time_start, NULL) == 0);
		
		srand(time(NULL));
		
		for (i = 0; i < N; i++) {
			while (positions[i] != a && positions[i] != b) {
				random_num = (double)rand() / 
							 ((double)RAND_MAX / (double)100);
				
				if (random_num < 100 * p) {
					positions[i]++;
				} 
				else {
					positions[i]--;
				}
				
				lifetimes[i]++;
			}
		}
		
		probab_b = 0;
		for (i = 0; i < N; i++)
			if (positions[i] == b)
				probab_b += 1;
		probab_b = probab_b / N;
		
		
		avg_lifetime = 0;
		for (i = 0; i < N; i++)
			avg_lifetime += lifetimes[i];
		avg_lifetime = avg_lifetime / N;
		
		
		assert(gettimeofday(&time_end, NULL) == 0);
		
		work_time = ((time_end.tv_sec - time_start.tv_sec) * 1000000u + 
					  time_end.tv_usec - time_start.tv_usec) / 1.e6;
		
					  
		file = fopen("stats.txt", "w");
		fprintf(file, "%f %.1f %f %d %d %d %d %f %d\n",
				probab_b, avg_lifetime, work_time, a, b, x, N, p, P);
		
		free(lifetimes);
		free(positions);
		fclose(file);
	}
}
