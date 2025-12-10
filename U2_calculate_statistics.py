import networkx as nx

def calculate_statistics(Gi,root_node,xml_file):
   
    calculate_distances(Gi,root_node)

    for n in Gi.nodes():
        if n == root_node: continue
        xml_file.write("\t\t<Paths>\n")
        xml_file.write("\t\t\t<SourceNode>" + str(n) + "</SourceNode>\n")
        xml_file.write("\t\t\t<RootNode>" + str(root_node) + "</RootNode>\n")

        shortest_path_length = len(Gi.edges())
        minimum_edge_disjoint_path_length = len(Gi.edges()) 
        minimum_arbroescecne_path_length = len(Gi.edges())

        average_edge_disjoint_path_length = 0
        average_arbroescecne_path_length = 0

        maximum_edge_disjoint_path_length = 0
        maximum_arbroescecne_path_length = 0

        edge_disjoint_path_number = 0
        arbroescecne_path_number = 0

        shortest_path_list = Gi.nodes()[n]['distances'][0]
        edge_disjoint_path_list = Gi.nodes()[n]['distances'][1]
        arbroescecne_path_list = Gi.nodes()[n]['distances'][2]

        for l in shortest_path_list:
            shortest_path_length = l
    
        for l in edge_disjoint_path_list:
            if l > 0:
                edge_disjoint_path_number += 1
                average_edge_disjoint_path_length += l
            if(l < minimum_edge_disjoint_path_length):
                minimum_edge_disjoint_path_length = l
            if(l > maximum_edge_disjoint_path_length):
                maximum_edge_disjoint_path_length = l
        average_edge_disjoint_path_length /= max(edge_disjoint_path_number,1)

        for l in arbroescecne_path_list:
            if l > 0:
                arbroescecne_path_number += 1
                average_arbroescecne_path_length += l
                if(l < minimum_arbroescecne_path_length):
                    minimum_arbroescecne_path_length = l
                if(l > maximum_arbroescecne_path_length):
                    maximum_arbroescecne_path_length = l
        average_arbroescecne_path_length /= max(arbroescecne_path_number,1)

        xml_file.write("\t\t\t<LocalConnectivity>" + str(arbroescecne_path_number) + "</LocalConnectivity>\n")
        xml_file.write("\t\t\t<ShortestPathLength>" + str(shortest_path_length) + "</ShortestPathLength>\n")
        xml_file.write("\t\t\t<MinimumEdgeDisjointPathLength>" + str(minimum_edge_disjoint_path_length) + "</MinimumEdgeDisjointPathLength>\n")
        xml_file.write("\t\t\t<MinimumArborescencePathLength>" + str(minimum_arbroescecne_path_length) + "</MinimumArborescencePathLength>\n")
        xml_file.write("\t\t\t<AverageEdgeDisjointPathLength>" +  "{:.3f}".format(average_edge_disjoint_path_length)  + "</AverageEdgeDisjointPathLength>\n")
        xml_file.write("\t\t\t<AverageArborescencePathLength>" +  "{:.3f}".format(average_arbroescecne_path_length)+ "</AverageArborescencePathLength>\n")
        xml_file.write("\t\t\t<MaximumEdgeDisjointPathLength>" + str(maximum_edge_disjoint_path_length) + "</MaximumEdgeDisjointPathLength>\n")
        xml_file.write("\t\t\t<MaximumArborescencePathLength>" + str(maximum_arbroescecne_path_length) + "</MaximumArborescencePathLength>\n")
        xml_file.write("\t\t</Paths>\n")

def calculate_distances(Gi,root_node):
    # Calculate shortest path length with Dijkstra
    hop_distance,shortest_path = nx.single_source_dijkstra(Gi,root_node,weight=1)
    for k,v in hop_distance.items():
        Gi.nodes()[k]['distances'] = [[v]]

    # Calculate edge-disjoint path lengths with Suurballe
    for n in Gi.nodes():
        disjoint_distances = []
        paths = nx.edge_disjoint_paths(Gi,n,root_node)
        if n != root_node:
            for path in list(paths):
                disjoint_distances.append(len(path)-1)
        else: disjoint_distances.append(0)

        Gi.nodes()[n]['distances'].append(disjoint_distances)

    # Calculate path lenght in the arborescences
    for n in Gi.nodes():
        arborescence_distances = []
        for arb in range(Gi.out_degree(root_node)):
            Hi = Gi.copy()

            for e in Gi.edges():
                arborescence_edge = False
                for c in Gi.edges()[e]['color']:
                    if c == arb + 1:
                        arborescence_edge = True
                        break

                if(not arborescence_edge):
                    # We change the direction from root to other nodes (to be able to use Dijkstra below)
                    Hi.remove_edge(e[1],e[0])

            tree_distance,tree_path = nx.single_source_dijkstra(Hi,root_node,weight=1)

            arb_dist = 0
            for k,v in tree_distance.items():
                if k == n:
                    arb_dist = v
                    
            arborescence_distances.append(arb_dist)

        Gi.nodes()[n]['distances'].append(arborescence_distances) 


if __name__ == "__main__":
    print("File calculate_statistics.py contains funcitons to save the results of the algoirhtms into .xml.")