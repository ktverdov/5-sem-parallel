#include <mpi.h>
#include <stdio.h>
#include <time.h>

#include "list.h"

typedef struct {
	int x;
	int y;
	int n;
} point_t;

void PointInit(point_t *point, int l, int n) {
	point->x = rand() % l;
	point->y = rand() % l;
	point->n = n;
}

void PointCopy(point_t *src, point_t *dist) {
	dist->x = src->x;
	dist->y = src->y;
	dist->n = src->n;
}

void PointPrint(int rank, point_t point) {
	printf("%d : %d %d %d \n", rank, point.x, point.y, point.n);
}

void PointMovement(int time_without_exchange, point_t *point, int l, double p_l, double p_r, 
					double p_u, int *dead_point, int *change_area) {
	//change area -1 (left), 1 (right), -2 (down), 2 (up)

	int time = 0;
	while (time < time_without_exchange && point->n != 0 && !*change_area) {
		double chance = (double)rand() / RAND_MAX;
		
		if (chance < p_l) {
			point->x--;
			if (point->x < 0) {
				point->x = l - 1;
				*change_area = -1;
			}
		} else if (chance < p_l + p_r) {
			point->x++;
			if (point->x >= l) {
				point->x = 0;
				*change_area = 1;
			}
		} else if (chance < p_l + p_r + p_u) {
			point->y++;
			if (point->y >= l) {
				point->y = 0;
				*change_area = 2;
			}
		} else {
			point->y--;
			if (point->y < 0) {
				point->y = l - 1;
				*change_area = -2;
			}
		}

		time++;
		point->n--;
	}

	if (point->n == 0) {
		*dead_point = 1;
	}
}

void MPIPointInit(MPI_Datatype *mpi_point) {
	int count = 3;
	int blocks[3] = {1, 1, 1};
	MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
	MPI_Aint displacements[3] = {offsetof(point_t, x), offsetof(point_t, y), offsetof(point_t, n)};
	MPI_Type_create_struct(count, blocks, displacements, types, mpi_point);
	MPI_Type_commit(mpi_point);
}


typedef struct {
	int left;
	int right;
	int up;
	int down;
} rank_t;

/*
processes correspond to squares according to the following picture:

10	11	12	13	14

5	6	7	8	9 

0	1 	2 	3 	4
*/

void GetAdjacentRanks(int rank, rank_t *adjacent_ranks, int a, int b) {
	if ((rank + 1) % a == 0) {
		adjacent_ranks->right = rank / a * a;
	} else {
		adjacent_ranks->right = rank + 1;
	}

	if (rank % a == 0) {
		adjacent_ranks->left = rank + a - 1;
	} else {
		adjacent_ranks->left = rank - 1;
	}

	if (rank / a == b - 1) {
		adjacent_ranks->up = rank % a;
	} else {
		adjacent_ranks->up = rank + a;
	}

	if (rank / a == 0) {
		adjacent_ranks->down = a * (b - 1) + rank % a;
	} else {
		adjacent_ranks->down = rank - a;
	}
}


void GetAndTestParameters(int size, int argc, char **argv, int *l, int *a, int *b, int *n, int *N,
							double *p_l, double *p_r, double *p_u, double *p_d) {
	if (argc != 10) {
		fprintf(stderr, "Wrong amount of arguments\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	*l = atoi(argv[1]);
	if (*l < 1) {
		fprintf(stderr, "Wrong size of square area\n");
		MPI_Abort(MPI_COMM_WORLD, 1);	
	}

	*a = atoi(argv[2]);
	*b = atoi(argv[3]);
	if (*a < 2 || *b < 2) {
		fprintf(stderr, "Wrong amount of squares\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	*n = atoi(argv[4]);
	if (*n < 1) {
		fprintf(stderr, "Wrong amount of points\n");
		MPI_Abort(MPI_COMM_WORLD, 1);	
	}

	*N = atoi(argv[5]);
	if (*N < 1) {
		fprintf(stderr, "Wrong amount of jumps\n");
		MPI_Abort(MPI_COMM_WORLD, 1);	
	}

	*p_l = atof(argv[6]);
	*p_r = atof(argv[7]);
	*p_u = atof(argv[8]);
	*p_d = atof(argv[9]);
	if (*p_l < 0 || *p_r < 0 || *p_u < 0 || *p_d < 0 || *p_l + *p_r + *p_u + *p_d != 1) {
		fprintf(stderr, "Something wrong with probability of jumps\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	if (*a * *b != size) {
		fprintf(stderr, "Amount of allotted processes is not equal to number of squares\n");
		MPI_Abort(MPI_COMM_WORLD, 1);	
	}
}


int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int l, a, b, n, N;
	double p_l, p_r, p_u, p_d;
	GetAndTestParameters(size, argc, argv, &l, &a, &b, &n, &N, &p_l, &p_r, &p_u, &p_d);
	
	rank_t adjacent_ranks;
	GetAdjacentRanks(rank, &adjacent_ranks, a, b);

	int time_without_exchange = n / 10;
	if (time_without_exchange < 100) {
		time_without_exchange = 100;
	}


	srand(time(NULL) + rank * N * 10);
	
	list_t points;
	ListInit(&points);

	for (int i = 0; i < N; i++) {
		point_t *point = (point_t *)malloc(sizeof(point_t));
		PointInit(point, l, n);
		Push(&points, point);
	}

	MPI_Datatype mpi_point;
	MPIPointInit(&mpi_point);

	double start_time = MPI_Wtime();

	int dead_points = 0;
	
	while(1) {
		size_t capacity = GetSize(&points);
		
		int num_to_left = 0, num_to_right = 0, num_to_up = 0, num_to_down = 0;
		point_t *to_left = (point_t *)malloc(sizeof(point_t) * capacity);
		point_t *to_right = (point_t *)malloc(sizeof(point_t) * capacity);
		point_t *to_up = (point_t *)malloc(sizeof(point_t) * capacity);
		point_t *to_down = (point_t *)malloc(sizeof(point_t) * capacity);

		list_node_t *curr_node = points.head;
		list_node_t *prev_node = NULL;

		while(curr_node != NULL) {
			int dead_point = 0;
			int change_area = 0;
			point_t *point = (point_t *)curr_node->data;

			PointMovement(time_without_exchange, point, l, p_l, p_r, p_u, &dead_point, 
							&change_area);

			if (change_area) {
				if (change_area == -1) {
					to_left[num_to_left] = *point;
					num_to_left++; 
				}
				if (change_area == 1) {
					to_right[num_to_right] = *point;
					num_to_right++; 
				}
				if (change_area == -2) {
					to_down[num_to_down] = *point;
					num_to_down++; 
				}
				if (change_area == 2) {
					to_up[num_to_up] = *point;
					num_to_up++; 
				}

				Remove(&points, prev_node);
			} else if (dead_point) {
				dead_points++;
				Remove(&points, prev_node);
			}

			if (dead_point || change_area) {
				if (prev_node == NULL) {
					curr_node = points.head;
				} else {
					curr_node = prev_node->next;
				}
			} else {
				prev_node = curr_node;
				curr_node = curr_node->next;
			}
		}

		int total_points;
		MPI_Allreduce(&dead_points, &total_points, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
		
		if (total_points == N * size) {
			free(to_left);
			free(to_right);
			free(to_up);
			free(to_down);
			
			break;
		}

		int num_from_left, num_from_right, num_from_up, num_from_down;

		MPI_Request* request = (MPI_Request*) malloc(sizeof(MPI_Request) * 8);
		MPI_Isend(&num_to_left, 1, MPI_INT, adjacent_ranks.left, 0, MPI_COMM_WORLD, request + 0);
		MPI_Isend(&num_to_right, 1, MPI_INT, adjacent_ranks.right, 0, MPI_COMM_WORLD, request + 1);
		MPI_Isend(&num_to_up, 1, MPI_INT, adjacent_ranks.up, 0, MPI_COMM_WORLD, request + 2);
		MPI_Isend(&num_to_down, 1, MPI_INT, adjacent_ranks.down, 0, MPI_COMM_WORLD, request + 3);

		MPI_Irecv(&num_from_left, 1, MPI_INT, adjacent_ranks.left, MPI_ANY_TAG, MPI_COMM_WORLD, 
					request + 4);
		MPI_Irecv(&num_from_right, 1, MPI_INT, adjacent_ranks.right, MPI_ANY_TAG, MPI_COMM_WORLD, 
					request + 5);
		MPI_Irecv(&num_from_up, 1, MPI_INT, adjacent_ranks.up, MPI_ANY_TAG, MPI_COMM_WORLD, 
					request + 6);
		MPI_Irecv(&num_from_down, 1, MPI_INT, adjacent_ranks.down, MPI_ANY_TAG, MPI_COMM_WORLD, 
					request + 7);
		MPI_Waitall(8, request, MPI_STATUS_IGNORE);
		free(request);

		point_t *from_left = (point_t *)malloc(sizeof(point_t) * num_from_left);
		point_t *from_right = (point_t *)malloc(sizeof(point_t) * num_from_right);
		point_t *from_up = (point_t *)malloc(sizeof(point_t) * num_from_up);
		point_t *from_down = (point_t *)malloc(sizeof(point_t) * num_from_down);
		
		request = (MPI_Request*) malloc(sizeof(MPI_Request) * 8);

		MPI_Isend(to_left, num_to_left, mpi_point, adjacent_ranks.left, 0, MPI_COMM_WORLD, 
					request + 0);
		MPI_Isend(to_right, num_to_right, mpi_point, adjacent_ranks.right, 0, MPI_COMM_WORLD, 
					request + 1);
		MPI_Isend(to_up, num_to_up, mpi_point, adjacent_ranks.up, 0, MPI_COMM_WORLD, 
					request + 2);
		MPI_Isend(to_down, num_to_down, mpi_point, adjacent_ranks.down, 0, MPI_COMM_WORLD, 
					request + 3);

		MPI_Irecv(from_left, num_from_left, mpi_point, adjacent_ranks.left, MPI_ANY_TAG, 
					MPI_COMM_WORLD, request + 4);
		MPI_Irecv(from_right, num_from_right, mpi_point, adjacent_ranks.right, MPI_ANY_TAG, 
					MPI_COMM_WORLD, request + 5);
		MPI_Irecv(from_up, num_from_up, mpi_point, adjacent_ranks.up, MPI_ANY_TAG, 
					MPI_COMM_WORLD, request + 6);
		MPI_Irecv(from_down, num_from_down, mpi_point, adjacent_ranks.down, MPI_ANY_TAG, 
					MPI_COMM_WORLD, request + 7);
		MPI_Waitall(8, request, MPI_STATUS_IGNORE);
		free(request);

		for (int i = 0; i < num_from_left; i++) {
			point_t *point = (point_t *)malloc(sizeof(point_t));
			PointCopy(&from_left[i], point);
			Push(&points, point);
		}

		for (int i = 0; i < num_from_right; i++) {
			point_t *point = (point_t *)malloc(sizeof(point_t));
			PointCopy(&from_right[i], point);
			Push(&points, point);
		}

		for (int i = 0; i < num_from_up; i++) {
			point_t *point = (point_t *)malloc(sizeof(point_t));
			PointCopy(&from_up[i], point);
			Push(&points, point);
		}

		for (int i = 0; i < num_from_down; i++) {
			point_t *point = (point_t *)malloc(sizeof(point_t));
			PointCopy(&from_down[i], point);
			Push(&points, point);
		}

		free(to_left);
		free(to_right);
		free(to_up);
		free(to_down);
		
		free(from_left);
		free(from_right);
		free(from_up);
		free(from_down);
	}

	ListFree(&points);

	int *amounts_of_points = NULL;

	if (rank == 0) {
		amounts_of_points = (int *)malloc(sizeof(int) * size);

		MPI_Gather(&dead_points, 1, MPI_INT, amounts_of_points, 1, MPI_INT, 0, MPI_COMM_WORLD);
			
		double end_time = MPI_Wtime();
			
		FILE *file = fopen("stats.txt", "w");
		fprintf(file, "%d %d %d %d %d %f %f %f %f %f \n", l, a, b, n, N, p_l,
				p_r, p_u, p_d, end_time - start_time);
				
		for (int i = 0; i < size; i++)
			fprintf(file, "%d: %d\n", i, amounts_of_points[i]);
				
		fclose(file);

		free(amounts_of_points);
	} else {
		MPI_Gather(&dead_points, 1, MPI_INT, amounts_of_points, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}

	MPI_Finalize();
	
	return 0;
}