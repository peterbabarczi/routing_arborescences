import networkx as nx
from itertools import permutations

def get_split_off_score(l):
    # Base score: 1000, Loop-edge: -1, Bidirectional edge: -10 (-5 below because we count them twice)
    score = 1000
    for v1,w1 in l:
        if v1 == w1: score -= 1
        else: 
            for v2,w2 in l:
                if v1 == w2 and w1 == v2:
                    score -= 5
    return score

def split_off_node(Gi,root_node,so,optimize):
    # Creaete all possible split-off node pairings (without self-loops)
    start_nodes = []
    end_nodes = []
    self_loops = 0

    for u,v in Gi.in_edges(so):
        if u != so: 
            for i in range(Gi.edges()[u,v]['capacity']):
                start_nodes.append(u)
        else:
            self_loops += Gi.edges()[so,so]['capacity']

    for u,v in Gi.out_edges(so):
        if v != so:
            for i in range(Gi.edges()[u,v]['capacity']):
                end_nodes.append(v)

    pairings = [list(zip(start_nodes, p)) for p in permutations(end_nodes)]

    # Find the best possible pairing to split-off a given node
    best_score = -1
    Gbest = Gi.copy()
    for l in pairings:
        Gtest = Gi.copy()
        # Remove self-loop edges as they do not participate in the node split-off and split-off the other pairs
        for i in range(self_loops):
            Gtest.edges()[so,so]['capacity'] -= 1
            if(Gtest.edges()[so,so]['capacity'] == 0): Gtest.remove_edge(so,so)

        for v,w in l:
            Gtest.edges()[v,so]['capacity'] -= 1
            Gtest.edges()[so,w]['capacity'] -= 1
            if(Gtest.edges()[v,so]['capacity'] == 0): Gtest.remove_edge(v,so)
            if(Gtest.edges()[so,w]['capacity'] == 0): Gtest.remove_edge(so,w)
            if w in Gtest[v]:
                Gtest.edges()[v,w]['capacity'] += 1
            else:
                Gtest.add_edge(v,w, weight=1, color=[0], capacity=1)

        # Check if a split-off is valid by calculating the maximum flow value before and after for all remaining node-pairs
        valid_split_off = True
        for j in Gtest.nodes():
            if(j!=root_node and Gtest.in_degree(j) > 0): 
                if(nx.maximum_flow_value(Gtest,root_node,j,capacity='capacity') < nx.maximum_flow_value(Gi,root_node,j,capacity='capacity') ):
                    # print("\t\tInvalid split-off: " + str(l))
                    valid_split_off = False
                    break

        # If a split-off is valid calculate the score and save the best split-off           
        if(valid_split_off):
            score = (1,get_split_off_score(l))[optimize]
            Gtest.nodes()[so]['split_pairs'] = l
            for i in range(self_loops):
                Gtest.nodes()[so]['split_pairs'].append((so,so))
            # print("\t\tValid split-off: " + str(Gtest.nodes()[so]['split_pairs']) + " with score " + str(score))
            if(best_score < score):
                best_score = score
                Gbest = Gtest
            
    # print("Best split-off for node " + str(so) + " is " + str(Gbest.nodes()[so]['split_pairs'])+ " with score " + str(best_score))
    
    return Gbest

if __name__ == "__main__":
    print("File split_off.py contains funcitons to find the best possible split-off of a given node.")