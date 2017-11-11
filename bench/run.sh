for i in `seq 0 10`; do
  str=`cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 64 | head -n 1`
  echo "String = $str"
  sudo numactl --physcpubind=0 --membind=0 ./bench .*Strasse.* $str
  echo ""
done
