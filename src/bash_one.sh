#test_wikipedia_link_en:$(exe)
make clean
make

#graph=livejournal
#graph=livejournal_undirected
graph=livejournal_undirected_reorder
graph_folder=/home/yuedeji/data/data/$graph

# ./ispan $graph_folder/fw_begin.bin $graph_folder/fw_adjacent.bin $graph_folder/bw_begin.bin $graph_folder/bw_adjacent.bin
perf stat -ddd ./ispan $graph_folder/fw_begin.bin $graph_folder/fw_adjacent.bin $graph_folder/bw_begin.bin $graph_folder/bw_adjacent.bin 
# > perf_lj_un_reorder_serial.csv

