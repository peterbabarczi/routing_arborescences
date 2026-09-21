import networkx as nx

def post_process(Gi,root_node):
    for arb in range(Gi.out_degree(root_node)):
        if arb > 0:
            Hi = Gi.copy()

            for e in Gi.edges():
                remove_edge = False
                for c in Gi.edges()[e]['color']:
                    if c != arb and c > 0:
                        remove_edge = True
                        break

                if remove_edge:
                    # We change the direction from root to other nodes (to be able to use Dijstra below)
                    Hi.remove_edge(e[1],e[0])

            # We remove arborescence arb from the solution
            for e in Gi.edges():
                for c in Gi.edges()[e]['color']:
                    if c == arb:
                        Gi.edges()[e]['color'].append(0)
                        Gi.edges()[e]['color'].remove(arb)
                        break
            
            # Calculate the new tree and save the solution
            tree_distance,tree_path = nx.single_source_dijkstra(Hi,root_node,weight=1)
            for k in tree_path:
                prev_node = root_node
                for v in tree_path[k]:
                    if v != root_node: 
                        # We change the direction back from the other nodes towards the root
                        for c_zero in Gi.edges()[v,prev_node]['color']:
                            if c_zero == 0: 
                                Gi.edges()[v,prev_node]['color'].append(arb)
                                Gi.edges()[v,prev_node]['color'].remove(0)
                        prev_node = v
    return Gi

if __name__ == "__main__":
    print("File post_process.py contains funcitons to find the shortest path tree in post-process.")