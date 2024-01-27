result_dir=$1

if [ -z "$1" ]; then
  echo "result directory not provided"
fi

# testing
# declare -a consumers_count=("1")
# declare -a takes=("take1")

# declare -a consumers_count=("1" "2" "4" "6" "8" "10" "12")
declare -a consumers_count=("6")
# declare -a takes=("take3" "take4" "take5" "take6" "take7" "take8" "take9" "take10") # "take11" "take12" "take13" "take14" "take15")
declare -a takes=("take1" "take2" "take3" "take4" "take5") # "take6" "take7" "take8" "take9" "take10") # "take11" "take12" "take13" "take14" "take15")

# get length of an array
number_of_takes=${#takes[@]}
number_of_consumers=${#consumers_count[@]}

# use for loop to read all values and indexes
for (( j=0; j<${number_of_consumers}; j++ ));
do
  c=${consumers_count[$j]}

  mkdir -p $result_dir/"1p"$c"c"

  for (( i=0; i<${number_of_takes}; i++ ));
  do
    echo "1p"$c"c", "consumers_count: $c, value: ${takes[$i]}"

    # create directory to store results for each take
    take=${takes[$i]}
    mkdir -p $result_dir/"1p"$c"c"/$take

    # run the experiment with desired number of consumer $c
    sudo mn -c
    sudo python file-transfer.py $c

    # move the results to the respective folder
    sleep 2
    sudo mv /tmp/minindn/* $result_dir/"1p"$c"c"/$take

    # sleep for 1 second, just to make sure the trasnfer is complete
    # sleep 1

    # generate packet count results
    # /home/map901/sdulal/multicast-suppression-ndn/results_icn23/process_results.sh $result_dir/"1p"$c"c"/$take $c 1

  done
done
