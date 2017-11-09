#include <assert.h>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

void TryUseParallel(const char *function, const int action, pthread_t *thrs, 
					int *thr_created, int *data, int * buffer, 
					int left, int right, int mid, int first_merge_mid,
					int second_merge_mid, int start, int m);

void * MergeSortRun(void *args);
void MergeSort(int *data, int* buffer, int left, int right, int m);

void * MergeRun(void *args);
void Merge(int *data, int *buffer, int l1, int r1, int l2, int r2, int start);

int BinarySearch(int key, int *data,  int l, int r);
int Comparator(const void * a, const void * b);

void ReadInitialData(const char *input, int *data, int n);
void WriteSortedData(const char *output, int *data_sorted, int n, 
						const char *mode);
void WriteStatistics(const char *output_stat, double work_time_merge, 
						double work_time_qsort, int n, int m, int P);

typedef struct merge_sort_context {	
	int *data;
	int *buffer;
	int left;
	int right;
	int m;

} merge_sort_context;

void MergeSortContextInit(merge_sort_context *m_s_args, int *data, int *buffer, 
							int left, int right, int m);

typedef struct merge_context {
	int *data;
	int *buffer;
	int l1;
	int r1;
	int l2;
	int r2;
	int start;
} merge_context;

void MergeContextInit(merge_context *m_args, int *data, int *buffer, 
						int l1, int r1, int l2, int r2, int start);


pthread_mutex_t mutex;
int num_available_threads;


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
	
	pthread_mutex_init(&mutex, NULL);
	num_available_threads = P - 1;

	assert(gettimeofday(&time_start_merge, NULL) == 0);
	
	MergeSort(array_merge, buffer, 0, n - 1, m);
	
	assert(gettimeofday(&time_end_merge, NULL) == 0);

	work_time_merge = 
		((time_end_merge.tv_sec - time_start_merge.tv_sec) * 1000000u + 
			time_end_merge.tv_usec - time_start_merge.tv_usec) / 1.e6;
	
	pthread_mutex_destroy(&mutex);
	
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

void *MergeSortRun(void *args) {
	merge_sort_context *m_s_args = (merge_sort_context *)args;
	MergeSort(m_s_args->data, m_s_args->buffer, 
				m_s_args->left, m_s_args->right,m_s_args->m);
	free(m_s_args);

	pthread_mutex_lock(&mutex);
	num_available_threads++;
	pthread_mutex_unlock(&mutex);
}

void TryUseParallel(const char *function, const int action, pthread_t *thrs, 
					int *thr_created, int *data, int * buffer, 
					int left, int right, int mid, int first_merge_mid,
					int second_merge_mid, int start, int m) {
	
	int success = 1;
	success = pthread_mutex_trylock(&mutex);
	
	if (success == 0 && num_available_threads > 0) {
		num_available_threads--;
		pthread_mutex_unlock(&mutex);

		thr_created[action] = 1;

		if (function == "MergeSort") {
			merge_sort_context *m_s_args = 
					(merge_sort_context *)malloc(sizeof(merge_sort_context));

			if (action == 0)
				MergeSortContextInit(m_s_args, data, buffer, left, mid, m); 

			if (action == 1)
				MergeSortContextInit(m_s_args, data, buffer, mid + 1, 
										right, m);

			pthread_create(thrs + action, NULL, MergeSortRun, 
							(void *)m_s_args);
		}

		if (function == "Merge") {
			merge_context *m_args = 
				(merge_context *)malloc(sizeof(merge_context));
			
			if (action == 2)
				MergeContextInit(m_args, data, buffer, left, first_merge_mid, 
					   				mid + 1, second_merge_mid - 1, start);
			
			if (action == 3)
				MergeContextInit(m_args, data, buffer, first_merge_mid + 1, 
									mid, second_merge_mid, right, start);

			pthread_create(thrs + action, NULL, MergeRun, (void *)m_args);
		}

	} else {
		if (success == 0)
			pthread_mutex_unlock(&mutex);

		if (action == 0)
			MergeSort(data, buffer, left, mid, m);

		if (action == 1)
			MergeSort(data, buffer, mid + 1, right, m);
			
		if (action == 2)
			Merge(data, buffer, left, first_merge_mid, 
					mid + 1, second_merge_mid - 1, start);

		if (action == 3)
			Merge(data, buffer, first_merge_mid + 1, mid, 
					second_merge_mid, right, start);
	}
}


void MergeSort(int *data, int *buffer, int left, int right, int m) {
	if (right - left < m) {
		qsort(data + left, right - left  + 1, sizeof(int), Comparator);
		return;
	} 

	int mid = (left + right) / 2;
	
	pthread_t *thrs = (pthread_t *)malloc(4 * sizeof(pthread_t));
	int *thr_created = (int *)malloc(4 * sizeof(int));
	for (int i = 0; i < 4; i++)
		thr_created[i] = 0;

	TryUseParallel("MergeSort", 0, thrs, thr_created, data, buffer, 
					left, right, mid, 0, 0, 0, m);
	TryUseParallel("MergeSort", 1, thrs, thr_created, data, buffer, 
					left, right, mid, 0, 0, 0, m);

	for (int i = 0; i < 2; i++)
		if (thr_created[i]) {
			pthread_join(thrs[i], NULL);
		}


	int first_merge_mid = (left + mid) / 2;
	int second_merge_mid = BinarySearch(data[first_merge_mid], 
										data, mid + 1, right);

	int start = left;
	TryUseParallel("Merge", 2, thrs, thr_created, data, buffer, 
					left, right, mid, first_merge_mid, second_merge_mid, 
					start, m);
	start = second_merge_mid + first_merge_mid - mid;
	TryUseParallel("Merge", 3, thrs, thr_created, data, buffer, 
					left, right, mid, first_merge_mid, second_merge_mid, 
					start, m);

	for (int i = 2; i < 4; i++)
		if (thr_created[i]) {
			pthread_join(thrs[i], NULL);
		}

	memcpy(data + left, buffer + left, (right - left + 1) * sizeof(int));

	free(thrs);
	free(thr_created);
}

void *MergeRun(void *args) {
	merge_context *m_args = (merge_context *)args;
	Merge(m_args->data, m_args->buffer, m_args->l1, m_args->r1,
			m_args->l2, m_args->r2, m_args->start);
	free(m_args);

	pthread_mutex_lock(&mutex);
	num_available_threads++;
	pthread_mutex_unlock(&mutex);
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

void MergeSortContextInit(merge_sort_context *m_s_args, int *data, int *buffer, 
							int left, int right, int m) {
	m_s_args->data = data;
	m_s_args->buffer = buffer;
	m_s_args->left = left;
	m_s_args->right = right;
	m_s_args->m = m;
}

void MergeContextInit(merge_context *m_args, int *data, int *buffer, 
						int l1, int r1, int l2, int r2, int start) {
	m_args->data = data;
	m_args->buffer = buffer;
	m_args->l1 = l1;
	m_args->r1 = r1;
	m_args->l2 = l2;
	m_args->r2 = r2;
	m_args->start = start;
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