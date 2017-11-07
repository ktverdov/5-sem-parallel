#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
	if (argc != 2 || argv[1] < 0) {
		printf("Wrong arguments\n");
		return -1;
	}
	
	int n = atoi(argv[1]);
	const int MAX_INT = 2 * 100 * 1000;
	
	srand(time(NULL));
	
	FILE *data_file = fopen("data.txt", "w");
	if (data_file == NULL) {
		printf("Cannot open data file\n");
	}
	
	for (int i = 0; i < n; i++) {
		fprintf(data_file, "%d ", -MAX_INT + (int)rand() % (2 * MAX_INT + 1));
	}

	fprintf(data_file, "\n\n");
	
	fclose(data_file);
	
	return 0;
}
