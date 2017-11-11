sudo perf stat numactl --physcpubind=0 --membind=0 ./bench .*Strasse.* $str
