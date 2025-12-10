import networkx as nx
import matplotlib.pyplot as plt
import json

colorcodes = {0: "black", 1: "red", 2: "blue", 3: "green", 4: "yellow", 5: "brown", 6: "orange"}

def construct_digraph_json(topology_name):
    json_graph = json.load(open(topology_name, 'rb'))
    Hi = nx.node_link_graph(json_graph, edges="links")
    G = nx.DiGraph()

    # Set node attributes
    coordinates = {}
    split_pairs = {}
    distances = {}
    for node in Hi.nodes(data=True):
        G.add_node(int(node[0]))
        split_pairs = []
        distances = []
        node_data = node[1]
        if 'x' in node_data and 'y' in node_data:
            latitude = float(node_data['x'])
            longitude = float(node_data['y'])
            coordinates[int(node[0])] = (longitude, latitude)

    nx.set_node_attributes(G,coordinates,"coordinates")
    nx.set_node_attributes(G,split_pairs,"split_pairs")
    nx.set_node_attributes(G,distances,"distances")

    # Set edge attributes
    edge_weight = {}
    edge_color = {}
    edge_capacity = {}
    for edge in Hi.edges():
        G.add_edge(int(edge[0]),int(edge[1]))
        G.add_edge(int(edge[1]),int(edge[0]))
        edge_weight[int(edge[0]),int(edge[1])] = 1 #round((float(edge_data['length'])),3)
        edge_weight[int(edge[1]),int(edge[0])] = 1 #round((float(edge_data['length'])),3)
        # We store color as list
        edge_color[int(edge[0]),int(edge[1])] = [0] 
        edge_color[int(edge[1]),int(edge[0])] = [0] 
        # Capacity is used to store multiple instances of multi-edges
        edge_capacity[int(edge[0]),int(edge[1])] = 1 
        edge_capacity[int(edge[1]),int(edge[0])] = 1

    nx.set_edge_attributes(G,edge_weight,"weight")
    nx.set_edge_attributes(G,edge_color,"color")
    nx.set_edge_attributes(G,edge_capacity,"capacity")

    return G


def construct_digraph_gml(topology_name):
    G = nx.DiGraph()
    GML = nx.read_gml(topology_name)

    # Set node attributes
    coordinates = {}
    split_pairs = {}
    distances = {}
    for node in GML.nodes(data=True):
        G.add_node(int(node[0]))
        split_pairs = []
        distances = []
        node_data = node[1]
        if 'Latitude' in node_data and 'Longitude' in node_data:
            latitude = float(node_data['Latitude'])
            longitude = float(node_data['Longitude'])
            coordinates[int(node[0])] = (longitude, latitude)

    nx.set_node_attributes(G,coordinates,"coordinates")
    nx.set_node_attributes(G,split_pairs,"split_pairs")
    nx.set_node_attributes(G,distances,"distances")

    # Set edge attributes
    edge_weight = {}
    edge_color = {}
    edge_capacity = {}
    for edge in GML.edges(data=True):
        G.add_edge(int(edge[0]),int(edge[1]))
        G.add_edge(int(edge[1]),int(edge[0]))
        edge_data = edge[2]
        edge_weight[int(edge[0]),int(edge[1])] = 1 #round((float(edge_data['length'])),3)
        edge_weight[int(edge[1]),int(edge[0])] = 1 #round((float(edge_data['length'])),3)
        # We store color as list
        edge_color[int(edge[0]),int(edge[1])] = [0] 
        edge_color[int(edge[1]),int(edge[0])] = [0] 
        # Capacity is used to store multiple instances of multi-edges
        edge_capacity[int(edge[0]),int(edge[1])] = 1 
        edge_capacity[int(edge[1]),int(edge[0])] = 1

    nx.set_edge_attributes(G,edge_weight,"weight")
    nx.set_edge_attributes(G,edge_color,"color")
    nx.set_edge_attributes(G,edge_capacity,"capacity")

    return G

def view_digraph(Gi,tree_number=0):
    Hi = Gi.copy()

    if(tree_number > 0):
        for e in Gi.edges():
            arborescence_edge = False
            for c in Gi.edges()[e]['color']:
                if c == tree_number:
                    Hi.edges()[e]['color'] = [c]
                    arborescence_edge = True
                    break

            if(not arborescence_edge):
                Hi.remove_edge(e[0],e[1])

    options = {"font_size": 8,"node_size": 100,"node_color": "white", "edgecolors": "black", "linewidths": 1,"connectionstyle": "arc3,rad=0.1"}

    nx.draw_networkx(Hi, pos=nx.get_node_attributes(Hi, "coordinates"), edge_color=[colorcodes[c[0]] for c in [Hi[u][w]['color'] for u,w in Hi.edges()]], width=[Hi[u][v]['capacity'] for u,v in Hi.edges()], **options)
    plt.axis("off")
    plt.show()

def save_trees_to_json(Gi,root_node,input_graph,Opt1,Opt2,Opt3,Opt4):
    json_file = open('res/json/' + str(input_graph[0:-4])  + "_O" + ("","1")[Opt1] + ("","2")[Opt2] + ("","3")[Opt3] + ("","4")[Opt4] + "_r" + str(root_node) + '.json', 'w')
    json_file.write(json.dumps(nx.node_link_data(Gi, edges="links")))
    json_file.close()

    json_graph = json.load(open('res/json/' + str(input_graph[0:-4])  + "_O" + ("","1")[Opt1] + ("","2")[Opt2] + ("","3")[Opt3] + ("","4")[Opt4] + "_r" + str(root_node) + '.json', 'r'))
    Hi = nx.node_link_graph(json_graph, edges="links")

    return Hi

if __name__ == "__main__":
    print("File graph_utils.py contains utility funcitons for reading input graphs and viewing them.")