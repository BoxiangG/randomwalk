import csv
import sys
import os
import time
import networkx as nx
import networkx.classes.function
from networkx.algorithms.distance_measures import diameter
from networkx.algorithms.components import number_weakly_connected_components
from networkx.algorithms.components import weakly_connected_components
from networkx.algorithms.components import number_strongly_connected_components
from networkx.algorithms.components import strongly_connected_components
from networkx.algorithms.community.modularity_max import greedy_modularity_communities
from networkx.algorithms.cluster import triangles

from collections import Counter

def read_graph(input_file):

    g = nx.DiGraph()
    header = True
    with open(input_file) as fp:
        for line in csv.reader(fp, delimiter = ' '):
            if line[0] == '%' and header == True:
                v_count, e_count = int(line[1]), int(line[2])
                for v in range(v_count):
                    g.add_node(v)
                header = False
                continue

            g.add_edge(int(line[0]), int(line[1]))

    return g


def get_degree_distribution(g):

    degree_counter = Counter()
    # in_degree
    for v in g.nodes():
        v_degree = g.in_degree[v]
        degree_counter[v_degree] += 1

    print("\nDegree(in) distribution (indegree, vertex count)")
    for one in sorted(degree_counter.items(), reverse = True):
        print('{}, {}'.format(one[0], one[1]))


def get_wcc_distribution(g):

    wcc_counter = Counter()

    wccs = weakly_connected_components(g)

    for wcc in wccs:
        wcc_counter[len(wcc)] += 1

    print("WCC distribution (wcc size, wcc count)")
    for one in sorted(wcc_counter.items(), reverse = True):
        print('{}, {}'.format(one[0], one[1]))


def get_scc_distribution(g):

    scc_counter = Counter()

    sccs = strongly_connected_components(g)

    for scc in sccs:
        scc_counter[len(scc)] += 1

    print("SCC distribution (scc size, scc count)")
    for one in sorted(scc_counter.items(), reverse = True):
        print('{}, {}'.format(one[0], one[1]))


def get_community_result(g):

    g_u = g.to_undirected()

    community_list = list(greedy_modularity_communities(g_u))

    print('\n# of communities (greedy_modularity),', len(community_list))

    community_counter = Counter()
    for community in community_list:
        community_counter[len(community)] += 1

    print("Community distribution (community size, community count)")
    for one in sorted(community_counter.items(), reverse = True):
        print('{}, {}'.format(one[0], one[1]))


def get_triangle(g):

    g_u = g.to_undirected()
    ts = triangles(g_u)

    print('\nTriangles')
    num = 0
    for v in ts:
        print('{}, {}'.format(v, ts[v]))
        num += ts[v]
    print('# of triangles', int(num / 3))


def main(input_file):

    g = read_graph(input_file)

    print('vert_count,', g.number_of_nodes(), 'edge_count,', g.number_of_edges())
    get_degree_distribution(g)

    print('\n# of weakly connected components,', number_weakly_connected_components(g))
    get_wcc_distribution(g)

    print('\n# of strongly connected components,', number_strongly_connected_components(g))
    get_scc_distribution(g)

    get_community_result(g)

    get_triangle(g)
#    print('community result')

if __name__ == "__main__":

    if len(sys.argv) != 2:
        print("Usage: python graph_stat_nx.py <input_graph>")
        exit(-1)

    main(sys.argv[1])
