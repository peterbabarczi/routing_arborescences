import networkx as nx
from itertools import groupby
from networkx.algorithms.connectivity import local_edge_connectivity

def split_off_nodes_in_order(G,root_node,optimize):
    # Ordered list of node degrees indexed by node name
    node_degrees = {}
    for n in G.nodes():
        if(G.out_degree(n) != G.in_degree(n)): print("ERROR: Graph is not symmetric!")
        node_degrees[n] = G.out_degree(n)

    node_degrees = {k: v for k,v in sorted(node_degrees.items(), key=lambda x: x[1])}

    #Ordered list of hop distances form root node indexed by node name 
    hop_distance,path =nx.single_source_dijkstra(G,root_node,weight=1)
    hop_distance = {k: v for k,v in sorted(hop_distance.items(), key=lambda x: x[1])}

    # Ordered list of local conccentivity between root node indexed by node name 
    local_connectivity = {}
    for j in G.nodes():
        if(j!=root_node): local_connectivity[j] = local_edge_connectivity(G,root_node,j)
    local_connectivity = {k: v for k,v in sorted(local_connectivity.items(), key=lambda x: x[1])}

    #Create and ordered list in which nodes will be split-off by the heuristic
    ## Divides nodes into sets based on local connectivty
    connectivity_groups = {i: [j[0] for j in j] for i, j in groupby(sorted(local_connectivity.items(), key = lambda x : x[1]), lambda x : x[1])}
    connectivity_groups = {k: v for k,v in sorted(connectivity_groups.items(), key=lambda x: x[0])}

    ## Increasing order of the sets order nodes within sets based on decreasing hop distance
    splitoff_order = []
    for key in connectivity_groups:
        connectivity_set=[]
        for n in connectivity_groups[key]:
            connectivity_set.append(n)
        if(optimize):
            for hop_key in reversed(hop_distance):
                for j in connectivity_set:
                    if(hop_key == j): splitoff_order.append(j)
        else:
            for j in connectivity_set:
                splitoff_order.append(j) 

    return splitoff_order

if __name__ == "__main__":
    print("File node_order.py contains funcitons to calculate the split-off order of nodes of a given graph and root node.")