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

    unsigned seed = (unsigned)time(NULL);
    double end_time;
    double write_time = 0.0;
    double start_time = wtime();
    int *result;
    result = (int *)malloc(steps * sizeof(int));
    vector<std::pair<int, int>> rank(v_count);
    for (auto it = rank.begin(); it != rank.end(); it++)
    {
        it->first = 0;
        it->second = it - rank.begin();
    }
    for (int i = 0; i < e_count; i++)
    {
        rank[g->fw_csr[i]].first += 1;
    }
    sort(rank.rbegin(), rank.rend());
    int *old2new;
    int *new2old;
    old2new = (int *)malloc(v_count * sizeof(int));
    new2old = (int *)malloc(v_count * sizeof(int));
    int *new_fw_pos;
    int *new_csr;
    new_fw_pos = (int *)malloc(v_count * sizeof(int));
    new_csr = (int *)malloc(e_count * sizeof(int));

    for (int i = 0; i < v_count; i++)
    {
        new2old[i] = rank[i].second;
        old2new[rank[i].second] = i;
    }

    new_fw_pos[0] = 0;
    for (int i = 1; i <= v_count; i++)
    {
        int old_index = new2old[i-1];
        int outdegree = g->fw_beg_pos[old_index+1] - g->fw_beg_pos[old_index];
        new_fw_pos[i] = new_fw_pos[i - 1] + outdegree; 
    }
    int index = 0;
    for (int i = 0; i < v_count; i++)
    {
        int old_index = new2old[i];
        int outdegree = g->fw_beg_pos[old_index+1] - g->fw_beg_pos[old_index];
        for (int j = 0; j < outdegree; j++) 
        {
            new_csr[index] = old2new[g->fw_csr[g->fw_beg_pos[old_index]+j]];
            index ++;
        }
    }    
    for (int i = 0; i < v_count; i++)
    {
        cout << new_fw_pos[i] << " ";
    }
    cout << endl;
    for (int i = 0; i < e_count; i++)
    {
        cout << new_csr[i] << " ";
    }
    cout << endl;


    int csr_index = 0;
    ofstream myFile("reordered_graph.txt");
    for (int i = 0; i < v_count; i++)
    {
        int outdegree = new_fw_pos[i+1]-new_fw_pos[i];
        for (int j = 0; j < outdegree; j++)
        {
            myFile << i << "    " << new_csr[csr_index] << endl;
            csr_index += 1;
        }
        
    }
    

    return 0;
}
