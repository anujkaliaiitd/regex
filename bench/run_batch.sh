sudo perf stat numactl --physcpubind=0 --membind=0 ./bench_batch .*Strasse.* --set gdk_nr_threads=10

#sudo numactl --physcpubind=0 --membind=0 ./bench_batch .*Strasse.* --set gdk_nr_threads=10
