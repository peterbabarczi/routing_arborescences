import os
import time
import networkx as nx

import O1_node_order as fno
import O2_split_off as fso
import O3_build_arborescences as fba
import O4_post_process as fpp
import U1_graph_utils as fgu
import U2_calculate_statistics as fcs

for input_graph in os.listdir('net'):
    Opt1 = True
    Opt2 = True
    Opt3 = True
    Opt4 = True

    # Read a simple undirected graph as the input topology from .gml or .json
    G = nx.DiGraph()
    if input_graph.endswith(".gml"): 
        xml_file = open('res/' + str(input_graph[0:-4]) + "_O" + ("","1")[Opt1] + ("","2")[Opt2] + ("","3")[Opt3] + ("","4")[Opt4] + '.xml', 'w')
        G = fgu.construct_digraph_gml('net/' + str(input_graph))
    elif(input_graph.endswith(".json")): 
        xml_file = open('res/' + str(input_graph[0:-5]) + "_O" + ("","1")[Opt1] + ("","2")[Opt2] + ("","3")[Opt3] + ("","4")[Opt4] + '.xml', 'w')
        G = fgu.construct_digraph_json('net/' + str(input_graph))
    else:
        continue

    Gi = G.copy()
    xml_file.write("<Simulator>\n")
    xml_file.write("\t<Algorithm>" + "OPT_" + ("","1")[Opt1] + ("","2")[Opt2] + ("","3")[Opt3] + ("","4")[Opt4] + "</Algorithm>\n")
    xml_file.write("\t<Network>\n")
    if input_graph.endswith(".gml"): 
        xml_file.write("\t\t<TopologyName>" + str(input_graph[0:-4]) + "</TopologyName>\n")
    else:
        xml_file.write("\t\t<TopologyName>" + str(input_graph[0:-5]) + "</TopologyName>\n")
    xml_file.write("\t\t<NodeNum>" + str(len(Gi.nodes())) + "</NodeNum>\n")
    xml_file.write("\t\t<EdgeNum>" + str(len(Gi.edges())) + "</EdgeNum>\n")
    xml_file.write("\t</Network>\n")
    xml_file.write("\t<Statistics>\n")

    start_time = time.process_time()

    investigated_roots = 0
    for root_node in Gi.nodes():
        investigated_roots +=1
        iter_time = time.process_time()

        # Calculate the split-off order as: 1) Groups with increasing local connectivity to root; 2) Within the groups dercreasing hop distance from the root."
        splitoff_order = fno.split_off_nodes_in_order(G,root_node,Opt1)
        #print("Order of nodes to split-off from root " + str(root_node) + " is: " + str(splitoff_order))

        # Split-off nodes in the above order
        for so in splitoff_order:
            Gi = fso.split_off_node(Gi,root_node,so,Opt2)

        # Build arborescences based on the reverse splitting-off operations
        Gi = fba.build_arborescences(G,Gi,root_node,splitoff_order,Opt3)

        # Run post-process to minimize path lenghts
        if(Opt4): fpp.post_process(Gi,root_node)

        fcs.calculate_statistics(Gi,root_node,xml_file)
        # for arb in range(G.out_degree(root_node)):
        #     fgu.view_digraph(Gi,arb+1)
        Hi = fgu.save_trees_to_json(Gi,root_node,input_graph,Opt1,Opt2,Opt3,Opt4)
        fgu.view_digraph(Hi,0)

        print("\t--- %s seconds ---" % (time.process_time() - iter_time))

    print("--- %s seconds ---" % (time.process_time() - start_time))
    xml_file.write("\t</Statistics>\n")
    xml_file.write("\t<TotalRunningTime>" + str(time.process_time() - start_time) + "</TotalRunningTime>\n")
    xml_file.write("\t<RunningTimePerRoot>" + str((time.process_time() - start_time)/investigated_roots) + "</RunningTimePerRoot>\n")
    xml_file.write("</Simulator>\n")
    xml_file.close()    



