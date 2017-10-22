#include <assert.h>
#include <malloc.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

void MergeSort(int *data, int* buffer, int left, int right, int m);
void Merge(int *data, int *buffer, int l1, int r1, int l2, int r2, int start);

int BinarySearch(int key, int *data,  int l, int r);
int Comparator(const void * a, const void * b);

void ReadInitialData(const char *input, int *data, int n);
void WriteSortedData(const char *output, int *data_sorted, int n, 
						const char *mode);
void WriteStatistics(const char *output_stat, double work_time_merge, 
						double work_time_qsort, int n, int m, int P);


int main(int argc, char **argv) {
	int n, m, P, i;

	struct timeval time_start_merge, time_end_merge,
				   time_start_qsort, time_end_qsort;

	double work_time_merge, work_time_qsort;
	
	if (argc != 4) {
		printf("Wrong amount of arguments");
		return 1;
	}
	
	n = atoi(argv[1]);
	m = atoi(argv[2]);
	P = atoi(argv[3]);
	
	int *buffer = (int*)malloc(n * sizeof(int));
	int *array_merge = (int*)malloc(n * sizeof(int));
	int *array_qsort = (int*)malloc(n * sizeof(int));
	
	if ((buffer == NULL) || (array_merge == NULL) || 
		(array_qsort == NULL)) {
		free(buffer);
		free(array_merge);
		free(array_qsort);
		printf("Cannot allocate memory");
		return -1;
	}
	
	ReadInitialData("data.txt", buffer, n);
	
	memcpy(array_merge, buffer, n * sizeof(int));
	memcpy(array_qsort, buffer, n * sizeof(int));
	
	
	assert(gettimeofday(&time_start_merge, NULL) == 0);
	
	omp_set_num_threads(P);
	omp_set_nested(1);
	
	#pragma omp parallel shared(array_merge, buffer)
	#pragma omp single nowait 
	{
		MergeSort(array_merge, buffer, 0, n - 1, m);
	}
	
	assert(gettimeofday(&time_end_merge, NULL) == 0);

	work_time_merge = 
		((time_end_merge.tv_sec - time_start_merge.tv_sec) * 1000000u + 
			time_end_merge.tv_usec - time_start_merge.tv_usec) / 1.e6;
	
	
	assert(gettimeofday(&time_start_qsort, NULL) == 0);
	
	qsort(array_qsort, n, sizeof(int), Comparator);
	
	assert(gettimeofday(&time_end_qsort, NULL) == 0);

	work_time_qsort = 
		((time_end_qsort.tv_sec - time_start_qsort.tv_sec) * 1000000u + 
			time_end_qsort.tv_usec - time_start_qsort.tv_usec) / 1.e6;
	
		  
	free(buffer);
	
	WriteSortedData("data.txt", array_merge, n, "a");
	free(array_merge);
	
	WriteSortedData("data_qsort.txt", array_qsort, n, "w");
	free(array_qsort);

	WriteStatistics("stats.txt", work_time_merge, work_time_qsort, n, m, P);
	
	return 0;
}

void MergeSort(int *data, int *buffer, int left, int right, int m) {
	if (right - left < m) {
		qsort(data + left, right - left  + 1, sizeof(int), Comparator);
		return;
	} 
	
	int mid = (left + right) / 2;
	
	#pragma omp task
	MergeSort(data, buffer, left, mid, m);

	#pragma omp task
	MergeSort(data, buffer, mid + 1, right, m);
	
	#pragma omp taskwait
	
	int first_merge_mid = (left + mid) / 2;
	int second_merge_mid = BinarySearch(data[first_merge_mid], 
										data, mid + 1, right);
    
    
	#pragma omp task
	{
		int start = left;
		Merge(data, buffer, left, first_merge_mid, 
									mid + 1, second_merge_mid - 1, start);
	}
							
	#pragma omp task
	{
		int start = second_merge_mid + first_merge_mid - mid;
		Merge(data, buffer, first_merge_mid + 1, mid, 
							second_merge_mid, right, start);
	}
	#pragma omp taskwait
	
	
	memcpy(data + left, buffer + left, (right - left + 1) * sizeof(int)); 
}

void Merge(int *data, int *buffer, int l1, int r1, int l2, int r2, int start) {
	int n = r1 + r2 - l1 - l2 + 2;
    
	for (int i = 0; i < n; i++)
		if ((l1 <= r1) && (l2 > r2 || data[l1] <= data[l2])) {
			buffer[i + start] = data[l1];
			l1++;
		} else {
			buffer[i + start] = data[l2];
			l2++;
		}
}

int BinarySearch(int key, int *data,  int l, int r) {	
	while (l < r) {
		int mid = (l + r) / 2;
		
		if (key > data[mid]) {
			l = mid + 1;
		} else {
			r = mid;
		}
	}
	
	if (key > data[l])
		r++;
		
	return r;
}

int Comparator(const void * a, const void * b) {
   return ( *(int *)a - *(int *)b );
}

void ReadInitialData(const char *input, int *data, int n) {
	FILE *data_file = fopen(input, "r");
	if (data_file == NULL) {
		printf("Cannot open data file\n");
		exit(1);
	}
	
	for (int i = 0; i < n; i++) {
		fscanf(data_file, "%d", &data[i]);
	}
	fclose(data_file);
}

void WriteSortedData(const char *output, int *data_sorted, int n, 
						const char *mode) {
	FILE *data_file = fopen(output, mode);
	if (data_file == NULL) {
		printf("Cannot open file to write sorted data\n");
		exit(1);
	}
	
	for (int i = 0; i < n; i++)
		fprintf(data_file, "%d ", data_sorted[i]);
	fprintf(data_file, "\n\n");
	
	fclose(data_file);	
}

void WriteStatistics(const char *output_stat, double work_time_merge, 
						double work_time_qsort, int n, int m, int P) {
	FILE *stats_file = fopen(output_stat, "w");
	if (stats_file == NULL) {
		printf("Cannot open file to write statistics\n");
		exit(1);
	}
	fprintf(stats_file, "%.3f %.3f %d %d %d\n",
			work_time_merge, work_time_qsort, n, m, P);
	fclose(stats_file); 
}
