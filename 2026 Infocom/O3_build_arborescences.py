from networkx.algorithms.connectivity import local_edge_connectivity


def select_best_color_for_uncolored_edge(Gi,so,root_node,c2):
    min_distance_on_c2 = 100000
    best_node_on_c2 = -1
    # Calculate path lenght in the arborescences
    for out_edge in Gi.out_edges(so):
        for c_null in Gi.edges()[out_edge[0],out_edge[1]]['color']:
            if c_null == 0:  
                for next_edge in Gi.out_edges(out_edge[1]):
                        for c_next in Gi.edges()[next_edge[0],next_edge[1]]['color']:
                            if c_next == c2: 
                                current_node = out_edge[1]
                                target_node = out_edge[1]
                                hop_length = 1

                                while target_node != root_node:
                                    for e in Gi.out_edges(target_node):
                                        for c in Gi.edges()[e[0],e[1]]['color']:
                                            if c == c2:
                                                target_node = e[1]
                                                hop_length += 1

                                if hop_length < min_distance_on_c2:
                                    min_distance_on_c2 = hop_length
                                    best_node_on_c2 = current_node

    return best_node_on_c2,min_distance_on_c2

def color_outedges(Gi,so,root_node,optimize):
  # Check if the new node so has out-edges with the same color:
    out_edge_colors = []
    for e in Gi.out_edges(so): 
        for c in Gi.edges()[e[0],e[1]]['color']:
            if c != 0:
                out_edge_colors.append(c)

    # Resolve conflict between outgoing edges
    if len(set(out_edge_colors)) < len(out_edge_colors):
        #print("\tWARNING: Resolve conflict for colors: " + str(out_edge_colors))
        for c1 in set(out_edge_colors):
            conflicting_end_nodes = []
            for e in Gi.out_edges(so): 
                for c2 in Gi.edges()[e[0],e[1]]['color']:
                    if c1 == c2:
                        conflicting_end_nodes.append(e[1])
            
            if len(conflicting_end_nodes) > 1:                
                hop_length_to_root = {}
                for n1 in conflicting_end_nodes:
                    target_node = n1
                    hop_length = 1
                    upstream = False
                    while target_node != root_node and target_node != so:
                        for e in Gi.out_edges(target_node):
                            for c2 in Gi.edges()[e[0],e[1]]['color']:
                                if c1 == c2:
                                    target_node = e[1]
                                    hop_length += 1

                        for n2 in conflicting_end_nodes:
                            if n1 != n2:
                                if target_node == n2 or target_node == so:
                                    upstream = True
                    
                    if upstream:
                        hop_length_to_root[n1] = 100000
                    else: hop_length_to_root[n1] = hop_length

                # Remove color from conflicting edges, keep the one with minimum hop distance
                keep_color_min_hop_distance = min(hop_length_to_root, key=hop_length_to_root.get)
                for n1 in conflicting_end_nodes:
                    if n1 != keep_color_min_hop_distance:
                        for c2 in Gi.edges()[so,n1]['color']:
                            if c1 == c2:
                                Gi.edges()[so,n1]['color'].append(0)
                                Gi.edges()[so,n1]['color'].remove(c1)
                                break
                
        # Remove color from conflicting edges if multiple edges were directed towards the same node
        same_edge_colors = []
        for e in Gi.out_edges(so): 
            used_colors = Gi.edges()[e[0],e[1]]['color'].copy()
            for c2 in used_colors:
                if c2 != 0:
                    for c3 in same_edge_colors:
                        if c2 == c3: 
                            Gi.edges()[e[0],e[1]]['color'].append(0)
                            Gi.edges()[e[0],e[1]]['color'].remove(c2)
                    same_edge_colors.append(c2)

    # Color the non-colored outging edges (either originally or becoming non-colored after removing conflict)
    arborescences_at_v = []
    for e1 in Gi.out_edges(so):
        for e2 in Gi.out_edges(e1[1]):  
            for c in Gi.edges()[e2[0],e2[1]]['color']:
                if c != 0:
                    arborescences_at_v.append(c)

    unused_colors_at_so = list(set(arborescences_at_v) - set(out_edge_colors))

    if optimize:
        trees_to_extend = len(unused_colors_at_so)
        for i in range(trees_to_extend):
            best_color = -1
            best_node = -1
            min_distance = 10000
            for c2 in unused_colors_at_so:
                best_node_on_c2, min_distance_on_c2 = select_best_color_for_uncolored_edge(Gi,so,root_node,c2)   
                if(min_distance_on_c2 < min_distance):
                    min_distance = min_distance_on_c2
                    best_node = best_node_on_c2
                    best_color = c2
            
            if(best_color > 0):
                Gi.edges()[so,best_node]['color'].append(best_color)
                Gi.edges()[so,best_node]['color'].remove(0)
                unused_colors_at_so.remove(best_color)

    else:
        for e1 in Gi.out_edges(so): 
            used_colors_at_so = Gi.edges()[e1[0],e1[1]]['color'].copy()
            for c1 in used_colors_at_so:
                if c1 == 0:   
                    already_colored = False
                    if e1[1] != root_node:
                        for e2 in Gi.out_edges(e1[1]): 
                            for c2 in Gi.edges()[e2[0],e2[1]]['color']:
                                if c2 in unused_colors_at_so and not already_colored:
                                    Gi.edges()[e1[0],e1[1]]['color'].append(c2)
                                    Gi.edges()[e1[0],e1[1]]['color'].remove(c1)
                                    unused_colors_at_so.remove(c2)
                                    already_colored = True

    return Gi

def check_loss_in_coverage(G,Gi,root_node,so):
    # Check if r(so,root) arborescences are available or not
    arborescences_at_new_node = []
    for e in Gi.out_edges(so):
        for c in Gi.edges()[e]['color']:
            if c > 0: arborescences_at_new_node.append(c)
    
    if local_edge_connectivity(G,so,root_node) != len(set(arborescences_at_new_node)):
        print("Lost coverage for " + str(local_edge_connectivity(G,so,root_node)-len(set(arborescences_at_new_node))) + " arborescences at " +str(so) + " FAILURE! FAILURE! FAILURE! FAILURE!")

def build_arborescences(G,Gi,root_node,splitoff_order,optimize):
    # Assign different colors to the loop edges of the root.
    new_color = 1
    for e in Gi.in_edges(root_node):
        Gi.edges()[e]['color'] = []
        for i in range(Gi.edges()[e]['capacity']):
            Gi.edges()[e]['color'].append(new_color)
            new_color += 1
     
    # Extend the arborescences in the reverse order of split-off nodes.
    for so in reversed(splitoff_order):
        # Assign an existing arborescence for each edge.
        for u,v in Gi.nodes()[so]['split_pairs']:
            if u == so and v == so: 
                if Gi.has_edge(so,so):
                    Gi.edges()[so,so]['capacity'] += 1
                    Gi.edges()[so,so]['color'].append(0)
                else: 
                    Gi.add_edge(so, so, weight=1, color=[0], capacity=1)
            else:
                inherited_color = 0
                for c in Gi.edges()[u,v]['color']:
                    if c > 0:
                        inherited_color = c
                    
                if Gi.has_edge(so,v):
                    Gi.edges()[so,v]['capacity'] += 1
                    Gi.edges()[so,v]['color'].append(inherited_color)
                else:
                    Gi.add_edge(so, v, weight=1, color=[inherited_color], capacity=1)

                if Gi.has_edge(u,so):
                    Gi.edges()[u,so]['capacity'] += 1
                    if u == root_node and v == root_node: Gi.edges()[u,so]['color'].append(0)
                    else: Gi.edges()[u,so]['color'].append(inherited_color)
                else:
                    if u == root_node and v == root_node: Gi.add_edge(u, so, weight=1, color=[0], capacity=1)
                    else: Gi.add_edge(u, so, weight=1, color=[inherited_color], capacity=1)
                
                Gi.edges()[u,v]['capacity'] -= 1
                if Gi.edges()[u,v]['capacity'] == 0: 
                    Gi.remove_edge(u,v)
                else:
                    for i in Gi.edges()[u,v]['color']:
                        if i == inherited_color:
                            Gi.edges()[u,v]['color'].pop(Gi.edges()[u,v]['color'].index(inherited_color))
                            break
        
        # Resolve conflicts and color uncolored arborescneces
        Gi = color_outedges(Gi,so,root_node,optimize)

        # Only for debug to check if the solution is correct or not
        # check_loss_in_coverage(G,Gi,root_node,so)

    return Gi

if __name__ == "__main__":
    print("File build_arborescences.py contains funcitons to construct routing arborescences based on the reverse split-off.")