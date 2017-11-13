Запуск на кластере:

```
$scp <file> <login>@calc.dc.phystech.edu:/home/atp_parallel/<login>

... 
$ ssh calc.dc.phystech.edu -l <login>
$ module add openmpi
$ make
$ sbatch -n 4 ./wrapper ./run 10 2 2 100 10 0.25 0.25 0.25 0.25
$ cat stats.txt
$ cat slurm-*
```

RunScript.sh - для запуска на локальной машине
