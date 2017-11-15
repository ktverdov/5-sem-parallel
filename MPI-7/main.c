#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int main(int argc, char **argv) {
	MPI_Init(&argc, &argv);

	double time_start = MPI_Wtime();

	int rank, size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int l, a, b, N;
	l = atoi(argv[1]);
	a = atoi(argv[2]);
	b = atoi(argv[3]);
	N = atoi(argv[4]);

	if (a * b != size) {
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	int *data;
	data = (int *)calloc(l * l * a * b, sizeof(int));
	MPI_Barrier(MPI_COMM_WORLD);

	srand(time(NULL) + rank * 100);
	for (int i = 0; i < N; i++) {
		int x, y, r;
		x = rand() % l;
		y = rand() % l;
		r = rand() % (a* b);
		data[(x + y * l) * (a * b) + r] += 1; 
	}

#ifdef DEBUG
	for (int i = 0; i < a * b * l * l; i++) {
		printf("%d %d \n", rank, data[i]);
	}
#endif

	MPI_File fh;
	int err = MPI_File_open(MPI_COMM_WORLD, "data.bin",
							MPI_MODE_CREATE | MPI_MODE_EXCL | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
	if (err != MPI_SUCCESS)  {
		if (rank == 0) {
			MPI_File_delete("data.bin",MPI_INFO_NULL);
		}
		MPI_File_open(MPI_COMM_WORLD, "data.bin", 
						MPI_MODE_CREATE | MPI_MODE_EXCL | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
	}

	MPI_Datatype view, contiguous;

	MPI_Type_vector(l, l * a * b, a * l * a * b, MPI_INT, &view);
	MPI_Type_commit(&view);

	MPI_Offset disp = ((rank / a) * a * l * l + (rank % a) * l) * a * b * sizeof(int);
	MPI_File_set_view(fh, disp, MPI_INT, view, "native", MPI_INFO_NULL);

	MPI_Type_contiguous(l * a * b, MPI_INT, &contiguous);
	MPI_Type_commit(&contiguous);
	MPI_File_write_all(fh, data, l, contiguous, MPI_STATUS_IGNORE);
	
	MPI_Type_free(&contiguous);
	MPI_Type_free(&view);

	MPI_File_close(&fh);

	free(data);

	if (rank = 0) {
		double time_end = MPI_Wtime();
			
		FILE *file = fopen("stats.txt", "w");
		fprintf(file, "%d %d %d %d %f\n", l, a, b, N, time_end - time_start);
		fclose(file);
	}

	MPI_Finalize();

	return 0;
}