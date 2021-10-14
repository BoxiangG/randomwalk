#test_wikipedia_link_en:$(exe)
make clean
make

graph=livejournal
graph_folder=/home/yuedeji/data/data/$graph

# ./ispan $graph_folder/fw_begin.bin $graph_folder/fw_adjacent.bin $graph_folder/bw_begin.bin $graph_folder/bw_adjacent.bin
perf stat -ddd ./ispan $graph_folder/fw_begin.bin $graph_folder/fw_adjacent.bin $graph_folder/bw_begin.bin $graph_folder/bw_adjacent.bin
#./ispan /mnt/raid0_huge/yuede/data/dbpedia/fw_begin.bin /mnt/raid0_huge/yuede/data/dbpedia/fw_adjacent.bin /mnt/raid0_huge/yuede/data/dbpedia/bw_begin.bin /mnt/raid0_huge/yuede/data/dbpedia/bw_adjacent.bin #56 30 200 10 0.1 10

