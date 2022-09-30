#!/bin/bash

result_folder=$2
result_dir=$1

if [[ !$result_folder ]]
then
 result_folder=$1
fi

echo $result_folder

# result_dir="/tmp/minindn"

solicited=$(for i in `ls $result_dir/*/log/nfd.log`; do cat $i|grep "soli"|grep /file |wc -l;done;)
data=$(for i in `ls $result_dir/*/log/nfd.log`; do cat $i|grep "Multicast data" |wc -l; done;)
interest=$(for i in `ls $result_dir/*/log/nfd.log`; do cat $i|grep "Multicast int" |wc -l; done;)
interest_sent=$(for i in `ls $result_dir/*/log/nfd.log`; do cat $i|grep "finally" | grep "Interest"|wc -l;done;)
data_sent=$(for i in `ls $result_dir/*/log/nfd.log`; do cat $i|grep "data finally"|grep /file |wc -l;done;)

# this can be better
interest_from_app=$(for i in `ls $result_dir/*/log/nfd.log`; do cat $i|grep "in=(261\|in=(260" | grep "onIncomingInterest" | grep /file |wc -l;done;)
# echo $interest_from_app
# if [ "$interest_from_app" -eq "0" ]; then
#    interest_from_app=$(for i in `ls $result_dir/*/log/nfd.log`; do cat $i|grep "onIncomingInterest in=(261"|wc -l;done;)
# fi

drop_dup_data=$(for i in `ls $result_dir/*/log/nfd.log`; do cat $i|grep "Data drop" |wc -l;done;)
drop_dup_interest=$(for i in `ls $result_dir/*/log/nfd.log`; do cat $i|grep "Interest drop" |wc -l;done;)

mkdir -p $result_folder && cd $result_folder

echo "p" "c1" "c2" "c3"> table.dat
echo $interest_from_app >>table.dat
echo $interest >> table.dat
echo $data >> table.dat
echo $solicited >> table.dat
echo $data_sent >> table.dat
echo $interest_sent >> table.dat
echo $drop_dup_data >> table.dat
echo $drop_dup_interest >> table.dat

datamash -W transpose < table.dat > t-table

rm table.dat

counter=1

for i in `ls $result_dir/*/log/nfd.log`
 do
  echo "dup ema st" > $counter.interest
  cat $i | grep "Updating EMA" -A 1 | grep "type: i" -A 1 | grep before | awk '{print $12","$9","$15}' >> $counter.interest
  # cat $i | grep "Updating EMA" -A 2 | grep "type: i" -A 2 | grep before | awk '{print $9 "," $12}' > $counter.st
  #echo "$(tail -n +2 $counter.dup)" > $counter.dup
  #awk -F "," 'p{print 1000*($0-p);}{p=$0}' $counter.dup | awk '{total += $0; $0=total}1' | sed "1s/.*/0/g" > $counter.ts
  #awk -F "," 'FNR==NR{a[NR]=$1;next}{$1=a[FNR]}1' $counter.ts $counter.dup | awk '{print $1 "," $2}' > $counter.dup-ts
  #paste -d "," $counter.dup-ts $counter.st >> $counter.interest

  echo "dup ema st" > $counter.data
  cat $i | grep "Updating EMA" -A 1 | grep "type: d" -A 1 | grep before | awk '{print $12","$9","$15}' >> $counter.data
  #cat $i | grep "Updating EMA" -A 2 | grep "type: d" -A 2 | grep before | awk '{print $9 "," $12}' > $counter.st
  #awk -F "," 'p{print 1000*($0-p);}{p=$0}' $counter.dup | awk '{total += $0; $0=total}1' | sed "1s/.*/0/g" > $counter.ts
  #awk -F "," 'FNR==NR{a[NR]=$1;next}{$1=a[FNR]}1' $counter.ts $counter.dup | awk '{print $1 "," $2}' > $counter.dup-ts
  #paste -d "," $counter.dup-ts $counter.st >> $counter.data

  cat $i | grep "and type: i" | awk '{print $13}' > $counter.rst_i
  cat $i | grep "and type: d" | awk '{print $13}' > $counter.rst_d

  # int_ts_seg_st = interest timestamp segment scheduled
  cat $i | grep "Suppression timer for name: " | grep "type: i" | awk '{split ($8, a, "="); print $1 "," a[3]}' > int_sch_ts_seg
  cat $i | grep "Interest drop" | awk '{split($7,a,"="); print a[3]}' > int_seg_drop
  cat $i | grep finally | grep Interest | awk '{split($5,a,"=");print a[3]}' > int_seg_sent
  cat $i | grep "Suppression timer for name: " | grep "type: i" | awk '{print $13}' > int_sch_st
  paste -d "," int_sch_ts_seg int_seg_sent int_seg_drop int_sch_st > $counter.int_seg_sent_drop_st

  #rm $counter.dup-ts $counter.dup $counter.st $counter.ts  int_sch_ts_seg int_seg_drop int_seg_sent int_sch_st
  rm int_sch_ts_seg int_seg_drop int_seg_sent int_sch_st

  counter=$((counter+1))
done

 paste -d "," *.rst_i > rst.interest
 paste -d "," *.rst_d > rst.data

 rm *.rst_i *.rst_d
