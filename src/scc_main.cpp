#include "wtime.h"
#include "graph.h"
//#include "frontier_queue.h"
#include "scc_common.h"
#include <unistd.h>
#include <map>
#include <set>
#include <algorithm>
#include <functional>
#include <bits/stdc++.h>
#include <fstream>
using namespace std;

// template <typename K, typename V>

// void print_map(map<K, V> const &m)
// {
//     for (auto it = m.cbegin(); it != m.cend(); ++it)
//     {
//         std::cout << "{" << (*it).first << ": " << (*it).second << "}\n";
//     }
// }
int main(int args, char **argv)
{

    printf("args = %d\n", args);
    // if(args != 11)
    // {
    //     std::cout<<"Usage: ./scc_cpu <fw_beg_file> <fw_csr_file> <bw_beg_file> <bw_csr_file> <thread_count> <alpha> <beta> <gamma> <theta> <run_times>\n";
    //     exit(-1);
    // }

    int steps = 100;

    const char *fw_beg_file = argv[1];
    const char *fw_csr_file = argv[2];
    const char *bw_beg_file = argv[3];
    const char *bw_csr_file = argv[4];
    // const index_t thread_count=atoi(argv[5]);
    // const int alpha = atof(argv[6]);
    // const int beta = atof(argv[7]);
    // const int gamma = atof(argv[8]);
    // const double theta= atof(argv[9]);
    // const index_t run_times = atoi(argv[10]);
    // printf("Thread = %d, alpha = %d, beta = %d, gamma = %d, theta = %g, run_times = %d\n", thread_count, alpha, beta, gamma, theta, run_times);

    double *avg_time = new double[15];
    ///step 1: load the graph
    graph *g = graph_load(fw_beg_file,
                          fw_csr_file,
                          bw_beg_file,
                          bw_csr_file,
                          avg_time);
    //index_t i=0;
    int v_count = g->vert_count;
    int e_count = g->edge_count;
    //int result[steps];
    //result[0] = cur_node;

    unsigned seed = (unsigned)time(NULL);
    //map<int, int> umap;
    double end_time;
    double write_time = 0.0;
    double start_time = wtime();


    for (int i = 1; i < 100000; i++)
    {
        if (i % 10000 == 0)
        {
            int a = i / 10000;
            //cout << " 0.01m check:" << a ;
        }

        for (int k = 0; k < 10; k++)
        {

            if (g->fw_beg_pos[i + 1] - g->fw_beg_pos[i] == 0)
            {
                continue;
            }

            int cur_node = i; //start node
            //result[0] = i;
            // if (umap.find(i) != umap.end())
            // {
            //     umap[i] += 1;
            // }
            // else
            // {
            //     umap[i] = 1;
            // }

            for (int j = 1; j < steps; j++)
            {
                int outdegree = g->fw_beg_pos[cur_node + 1] - g->fw_beg_pos[cur_node];
                if (outdegree == 0)
                {
                    cout << cur_node << "end";
                    return 0;
                }
                int random_n = rand_r(&seed);
                int neighbor = g->fw_csr[g->fw_beg_pos[cur_node] + random_n % outdegree];
                //result[j] = neighbor;
                // if (umap.find(neighbor) != umap.end())
                // {
                //     umap[neighbor] += 1;
                // }
                // else
                // {
                //     umap[neighbor] = 1;
                // }
                cur_node = neighbor;
            }
            // double tmp = wtime();
            // for (int i = 0; i < steps; i++)
            // {
            //     myFile << result[i] << ",";
            // }
            // myFile << endl;
            // write_time += wtime() - tmp;
        }
    }
    //cout << endl;
    end_time = wtime();
    cout << "total time spend:" << end_time - start_time << endl;
    //cout << "total wrte time spend:" << write_time << endl;
    //cout << "write time percentage: " << write_time / (end_time - start_time) * 100 << "%" << endl;

    

    //sort map
    //comparator lambda function
    // auto comp = [](pair<int, int> a, pair<int, int> b)
    // {
    //     //comparison logic
    //     //if value is greater for the first element
    //     //no need to swap
    //     if (a.second > b.second)
    //         return false;
    //     //if value is less for the first element
    //     //need to swap
    //     else if (a.second < b.second)
    //         return true;
    //     else
    //     { // when values are same
    //         if (a.first < b.first)
    //         {
    //             return false;
    //         }
    //         else
    //             return true;
    //     }
    // };
    
    // priority_queue<pair<int, int>, vector<pair<int, int>>, decltype(comp)> pq(comp);

    // for (auto &ij : umap)
    // {
    //     pq.push(ij);
    // }
    // //printing the sorted map
    // myFile << "vertex,frequent,outdegree\n";
    // while (!pq.empty())
    // {
    //     int node = pq.top().first;
    //     myFile << node << "," << pq.top().second << "," << g->fw_beg_pos[node + 1] - g->fw_beg_pos[node] << endl;
    //     pq.pop();
    // }

    return 0;
}
