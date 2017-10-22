task.pdf - task description

main.c - implementation of parallel merge sort

main_1.c - sequential implementation of merge sort

analysis.ipynb - python script which generates random data, runs main.c and main_1.c several times for each number of threads and then builds needed plots

gen.c - generates random data for running main.c and main_1.c without analysis.ipynb.  
Use: `$ ./run "number of elements to generate"`
