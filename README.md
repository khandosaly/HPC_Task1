# HPC Projects

Report 1:
```
g++ -O3 -o report_1/sequential.exe report_1/raytracing.cpp -lminirt
```

Example 1 ( Hello threads):
```
g++ -O3 -o thread_hello/compiled thread_hello/thread_hellp.cpp -lpthread
```

Example 2 ( Array threads):
```
g++ -O3 -o threads_array/compiled threads_array/array.cpp -lpthread
```

Report 2:
```
g++ -O3 -o report_2/compiled.out report_2/raytracing_threads.cpp -lminirt -lpthread
```

Report 3:
```
g++ -O3 -o report_3/compiled.out report_3/raytracing_openmp.cpp -lminirt -fopenmp
```

