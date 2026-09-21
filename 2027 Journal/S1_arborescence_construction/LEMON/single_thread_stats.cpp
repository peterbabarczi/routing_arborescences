/**
 \file  single_thread_stats.cpp
 \brief Calculates perferct routing arborescences.

 \author              Péter Babarczi
 \author              babarczi@tmit.bme.hu
 \date                2026 August
*/

#include <string>
#include <thread>
#include <chrono>

#include <lemon/core.h>
#include <lemon/adaptors.h>
#include <lemon/smart_graph.h>
#include <lemon/list_graph.h>
#include <lemon/dim2.h>
#include <lemon/lgf_reader.h>



#include <lemon/dijkstra.h>
#include <lemon/suurballe.h>
#include <lemon/edmonds_karp.h>

using namespace std;
using namespace lemon;

typedef ListDigraph Digraph; 

typedef Digraph::Arc Arc;
typedef Digraph::ArcIt ArcIt; 

typedef Digraph::Node DNode;  
typedef Digraph::NodeIt DNodeIt; 
typedef Digraph::OutArcIt OutArcIt; 
typedef Digraph::InArcIt InArcIt;  

typedef Digraph::ArcMap<double> ArcDoubleMap;  
typedef Digraph::ArcMap<int> ArcIntMap; 
typedef Digraph::ArcMap<bool> ArcBoolMap; 

typedef Digraph::NodeMap<double> DNodeDoubleMap;
typedef Digraph::NodeMap<int> DNodeIntMap; 
typedef Digraph::NodeMap<bool> DNodeBoolMap; 

typedef double CostValue;
typedef Digraph::ArcMap<CostValue> ArcCostMap; 
typedef double CapValue; 
typedef Digraph::ArcMap<CapValue> ArcCapMap; 
typedef int ColorValue; 
typedef Digraph::ArcMap<ColorValue> ArcColorMap; 
typedef vector<int> VisitedNodesValue; 
typedef Digraph::ArcMap<VisitedNodesValue> ArcVisitedNodesMap; 
typedef map<int,int> ArborescenceDistance;
typedef Digraph::ArcMap<ArborescenceDistance> ArcArborescenceDistance; 

typedef dim2::Point<double> Point; 
typedef Digraph::NodeMap<Point> DNodeCoordMap; 
typedef multimap<int,int> SplitPairs;
typedef Digraph::NodeMap<SplitPairs> DNodeSplitPairsMap; 
typedef double ConnectivityValue; 
typedef Digraph::NodeMap<ConnectivityValue> DNodeConnectivityMap; 
typedef double OriginalName; 
typedef Digraph::NodeMap<OriginalName> DNodeNameMap; 
typedef map<int,int> DistanceOnArborescence;
typedef Digraph::NodeMap<DistanceOnArborescence> DNodeDistanceOnArborescence; 

typedef Dijkstra<Digraph, ArcCostMap> DDijkstra; 
typedef EdmondsKarp<Digraph, ArcCapMap> DEdmonds; 
typedef Suurballe<Digraph, ArcCapMap> DSuurballe; 

typedef FilterArcs<Digraph, ArcBoolMap> DFilteredGraph;
typedef Dijkstra<DFilteredGraph, ArcCostMap> DFilteredDijkstra;  
typedef EdmondsKarp<DFilteredGraph, ArcCostMap> DFilteredEdmonds;  

typedef DFilteredGraph::Node DFilteredNode;  
typedef DFilteredGraph::NodeIt DFilteredNodeIt; 

typedef DFilteredGraph::ArcIt  DFilteredArcIt; 


double overall_shortest_path_length = 0;

double overall_minimum_edge_disjoint_path_stretch = 0;
double overall_minimum_arborescecne_path_stretch = 0;

double overall_average_edge_disjoint_path_stretch = 0;
double overall_average_arborescecne_path_stretch = 0;

double overall_maximum_edge_disjoint_path_stretch = 0;
double overall_maximum_arborescecne_path_stretch= 0;

int edge_number = 0;
int node_number = 0;
int total_tested_permutations = 0;
int total_all_permutations = 0;
int st_pairs = 0;

//! Running time calculation
double timeDiff(const rusage& _start, const rusage& _end)
{
	// Adds both utime and stime
	double time = (double)(_end.ru_utime.tv_usec - _start.ru_utime.tv_usec)/1000000
			+(double)(_end.ru_utime.tv_sec - _start.ru_utime.tv_sec)
			+(double)(_end.ru_stime.tv_usec - _start.ru_stime.tv_usec)/1000000
			+(double)(_end.ru_stime.tv_sec - _start.ru_stime.tv_sec);

	return time;
}

int calculate_arborescences_for_root(int root_id, string lgf_file_name, ofstream& os){
	auto start_time_chrono_iter = chrono::steady_clock::now();
	// We read the directed graph from the .lgf file.
	ifstream lgf_file(lgf_file_name.c_str());
	if (!lgf_file) {
		cout << "Error opening lgf file of "<< lgf_file_name;
		return 0;
	}

	bool view_results_on_stdout = false;
	Digraph g;

    ArcCostMap cost(g);
	ArcCapMap cap(g);  
	ArcColorMap color(g);

    DNodeCoordMap coords(g);
	DNodeSplitPairsMap split_pairs(g); 
	DNodeConnectivityMap local_connectivity_value(g);
	DNodeNameMap original_node_name(g);

	DigraphReader<Digraph> digraphreader(g, lgf_file);
	digraphreader.run();

	DNode nit;
    for (DNodeIt nit1(g); nit1 != INVALID; ++nit1){
		if(g.id(nit1) == root_id){
			nit = nit1;
		}
		split_pairs[nit1].clear();
		local_connectivity_value[nit1] = 0;
		original_node_name[nit1] = g.id(nit1);
    }

    for (ArcIt eit(g); eit != INVALID; ++eit){
		cost[eit] = 1;
		cap[eit] = 1;
		color[eit] = 0;
    }

	rusage iterStartTime;
	getrusage(RUSAGE_SELF, &iterStartTime);	

	// Reset maps
	for (DNodeIt nit1(g); nit1 != INVALID; ++nit1){
		split_pairs[nit1].clear();
		local_connectivity_value[nit1] = 0;
	}

	for (ArcIt eit(g); eit != INVALID; ++eit){
		cost[eit] = 1;
		cap[eit] = 1;
		color[eit] = 0;
	}

	/*****************************************
	O1_node_order.py: Calculating splitting order of nodes in increasing local connectivity, 
	using decreasing hop count as a tie-breaker.
	******************************************/
	DDijkstra ddijkstra(g,cost);
	ddijkstra.run(nit);
	map<int,int> local_connectivity;

	for (DNodeIt nit2(g); nit2 != INVALID; ++nit2){
		//cout << "n" << g.id(nit2) << " hop distance from n" << g.id(nit) << " is " << ddijkstra.dist(nit2) << endl;
		if(nit != nit2){
			DEdmonds dedmonds(g,cap,nit2,nit);
			dedmonds.run();

			local_connectivity.insert(make_pair(g.id(nit2), dedmonds.flowValue()));
			local_connectivity_value[nit2] = dedmonds.flowValue();
			
			//cout << "local connectivity between n" << g.id(nit2) << " and n" << g.id(nit) << " is " << dedmonds.flowValue() << endl;
		}
	}

	vector<int> splitoff_order;

	while(splitoff_order.size() < node_number-1){
		int min_connectivity = edge_number*2;
		int max_distance = 0;
		int next_candidate = -1;

		for(map<int,int>::iterator mapit = local_connectivity.begin(); mapit != local_connectivity.end(); mapit++){
			if(mapit -> second < min_connectivity){
					min_connectivity = mapit -> second;
			}
		}

		for(map<int,int>::iterator mapit = local_connectivity.begin(); mapit != local_connectivity.end(); mapit++){
			if(mapit -> second == min_connectivity){
					if(max_distance <=  ddijkstra.dist(g.nodeFromId(mapit -> first))){
					max_distance = ddijkstra.dist(g.nodeFromId(mapit -> first));
					next_candidate = mapit -> first;
					}
			}
		}

		splitoff_order.push_back(next_candidate);
		local_connectivity.erase(next_candidate);
	}

	if(view_results_on_stdout){
		cout << "Splitoff order of O1: [ ";
		for(vector<int>::iterator vit = splitoff_order.begin(); vit != splitoff_order.end(); vit++){
			cout << *vit << " ";
		}
		cout << "] " << endl;
	}

	rusage timeAfterO1;
	getrusage(RUSAGE_SELF, &timeAfterO1);
	

	/*****************************************
	O2_split-off.py: Split-off nodes in the order defined in splitoff-order
	******************************************/
	// Create a copy of G and splitting-off the edges of the new Gi graph.
	Digraph gi;
	ArcCostMap costi(gi);
	ArcCapMap capi(gi);  
	ArcColorMap colori(gi);
	DNodeConnectivityMap local_connectivity_valuei(gi);
	DNodeNameMap original_node_namei(gi);

	for (DNodeIt nit2(g); nit2 != INVALID; ++nit2){
		DNode n = gi.addNode();
		local_connectivity_valuei[n] = local_connectivity_value[nit2];
		original_node_namei[n] = original_node_name[nit2];
	}

	for (ArcIt eit(g); eit != INVALID; ++eit){
		DNode src,dest;
		for (DNodeIt nit2(gi); nit2 != INVALID; ++nit2){
			if(original_node_name[g.source(eit)] == original_node_namei[nit2]) src = nit2;
			if(original_node_name[g.target(eit)] == original_node_namei[nit2]) dest = nit2;
		}
		Arc e = gi.addArc(src,dest);
		costi[e] = cost[eit];
		capi[e] = cap[eit];
		colori[e] = color[eit];
	}

	// Splitting-off the nodes in the given order from O1.
	for(vector<int>::iterator vit = splitoff_order.begin(); vit != splitoff_order.end(); vit++){
		vector<int> inarcs;
		vector<int> outarcs;

		//cout << "Splitting-off node n" << *vit << endl;
		DNode remove_node;
		for (DNodeIt nit2(gi); nit2 != INVALID; ++nit2){
			if(original_node_namei[nit2] == *vit){
				remove_node = nit2;
			}
		}

		// Remove self-loop edges as they do not participate in the node split-off and split-off the other pairs
		int number_of_self_loops = 0;
		//cout << "In-arcs: " << endl;
		for(InArcIt eit(gi,remove_node); eit != INVALID; ++eit){ 
			if(original_node_namei[gi.source(eit)] == *vit){
				number_of_self_loops++;
			}
			else{
				inarcs.push_back(original_node_namei[gi.source(eit)]);
				//cout << original_node_namei[gi.source(eit)] << " ";
			}
		}
	
		//cout << endl << "Out-arcs: " << endl;
		for(OutArcIt eit(gi,remove_node); eit != INVALID; ++eit) {
			if(original_node_namei[gi.target(eit)] != *vit){
				outarcs.push_back(original_node_namei[gi.target(eit)]);
				//cout << original_node_namei[gi.target(eit)] << " ";
			}
		}
		//cout << endl;

		// In order to get all permutations list must be sorted first.
		sort(inarcs.begin(), inarcs.end());
		sort(outarcs.begin(), outarcs.end());
		multimap<int,multimap<int,int> > all_permutations;
		do{
			multimap<int,int> current_permutation;
			for(vector<int>::iterator init = inarcs.begin(); init != inarcs.end(); ){
				for(vector<int>::iterator outit = outarcs.begin(); outit != outarcs.end(); ){
					//cout << "(" << *init << " " << *outit << ") ";
					current_permutation.insert(make_pair(*init,*outit));
					init++;
					outit++;
				}
			}

			// Calculating score for all permutations, sorting the list in which split-off will be attempted.
			// NOTE: Improved score compared to the Python code: avoid bidirectinal arcs as much as possible, avoid loop arcs as secondary objective!
			int score = 0;
			for(multimap<int,int>::iterator mapit1 = current_permutation.begin(); mapit1 != current_permutation.end(); mapit1++){
				if(mapit1 ->first == mapit1 -> second){
					score +=1;
				}
				else{
					for(multimap<int,int>::iterator mapit2 = current_permutation.begin(); mapit2 != current_permutation.end(); mapit2++){
						if(mapit1 ->first == mapit2 -> second && mapit2 ->first == mapit1 -> second){
							score += 50;
						}
					}
				}
			}

			//cout << " Score: " << score << endl;
			all_permutations.insert(make_pair(score,current_permutation));

		} while(next_permutation(outarcs.begin(), outarcs.end()));  
		// cout << "All permutations: " << all_permutations.size() << endl;
		total_all_permutations += all_permutations.size();

		// Starting from the permutation with best(=lowest) score, trying to split-off the node.
		int tested_permutations = 0;
		bool valid_splitoff_found = false;
		for(multimap<int, multimap<int,int> >::iterator perit = all_permutations.begin(); perit != all_permutations.end(); perit++){
			tested_permutations++;
			//cout << perit -> first << ": ";
			for(multimap<int,int>::iterator mapit = (perit -> second).begin(); mapit != (perit -> second).end(); mapit++){
				//cout << "(" << mapit->first << " " << mapit->second << ") ";
			}
			//cout << endl;

			// Create a copy of Gi called Gtest to check whether it is a valid split-off or not.
			Digraph gtest;
			ArcCostMap costtest(gtest);
			ArcCapMap captest(gtest);  
			ArcColorMap colortest(gtest);
			DNodeConnectivityMap local_connectivity_valuetest(gtest);
			DNodeNameMap original_node_nametest(gtest);

			for (DNodeIt nit2(gi); nit2 != INVALID; ++nit2){
				DNode n = gtest.addNode();
				local_connectivity_valuetest[n] = local_connectivity_valuei[nit2];
				original_node_nametest[n] = original_node_namei[nit2];
			}

			for (ArcIt eit(gi); eit != INVALID; ++eit){
				DNode src,dest;
				for (DNodeIt nit2(gtest); nit2 != INVALID; ++nit2){
					if(original_node_namei[gi.source(eit)] == original_node_nametest[nit2]) src = nit2;
					if(original_node_namei[gi.target(eit)] == original_node_nametest[nit2]) dest = nit2;
				}
				Arc e = gtest.addArc(src,dest);
				costtest[e] = costi[eit];
				captest[e] = capi[eit];
				colortest[e] = colori[eit];
			}

			// Split off all edges of the node, check if the resulting graph is valid, i.e., flor value does not decrese from any node towards to root.
			for(multimap<int,int>::iterator mapit = (perit -> second).begin(); mapit != (perit -> second).end(); mapit++){
				DNode src,dest;
				for (ArcIt eit(gtest); eit != INVALID; ++eit){
					if(original_node_nametest[gtest.source(eit)] == mapit->first && original_node_nametest[gtest.target(eit)] == *vit){
						src = gtest.source(eit);
						//cout << "\t Remove arc: " <<original_node_nametest[gtest.source(eit)] << "-" << *vit << endl;
						gtest.erase(eit);
						break; // We must break to ensure only one copy removed at a time.
					}
				}
				
				for (ArcIt eit(gtest); eit != INVALID; ++eit){
					if(original_node_nametest[gtest.source(eit)] == *vit && original_node_nametest[gtest.target(eit)] == mapit->second){
						dest = gtest.target(eit);
						//cout << "\t Remove arc: " << *vit << "-" << original_node_nametest[gtest.target(eit)] << endl;
						gtest.erase(eit);
						break; // We must break to ensure only one copy removed at a time.
					}
				}
				//cout << "\t Add arc: " << original_node_nametest[src] << "-" << original_node_nametest[dest] << endl;
				Arc addarc = gtest.addArc(src,dest);
				costtest[addarc] = 1;
				captest[addarc] = 1;
				colortest[addarc] = 0;
			}

			// Remove the split-off node.
			for (DNodeIt nit2(gtest); nit2 != INVALID; ++nit2){
				if(original_node_nametest[nit2] == *vit){
					gtest.erase(nit2);
					break;
				}
			}

			// Test whether flow value decreses or not with the split-off.
			bool valid_split_off = true;
			for (DNodeIt nit3(gtest); nit3 != INVALID; ++nit3){
				if(original_node_nametest[nit3] == root_id){
					for (DNodeIt nit2(gtest); nit2 != INVALID; ++nit2){
						if(nit3 != nit2){
							DEdmonds dedmonds(gtest,captest,nit2,nit3);
							dedmonds.run();

							if(dedmonds.flowValue() < local_connectivity_valuetest[nit2]){
								//cout << "\t INVALID: Flow value decreased for node n" << original_node_nametest[nit2] << " from " << local_connectivity_valuetest[nit2] << " to " << dedmonds.flowValue() << endl;
								valid_split_off = false;
								break;
							}
							else{
								local_connectivity_valuetest[nit2] = dedmonds.flowValue();
							}
						}
					}
				}
			}

			// Save the results in Gi and the split-off in G.
			if(valid_split_off){
				valid_splitoff_found = true;
				//cout << "\tValid split-off with highest score is: " << endl;
				for(multimap<int,int>::iterator mapit = (perit -> second).begin(); mapit != (perit -> second).end(); mapit++){							
					DNode src,dest;
					for (ArcIt eit(gi); eit != INVALID; ++eit){
						if(original_node_namei[gi.source(eit)] == mapit->first && original_node_namei[gi.target(eit)] == *vit){
							src = gi.source(eit);
							//cout << "\t Remove arc: " <<original_node_namei[gi.source(eit)] << "-" << *vit << endl;
							gi.erase(eit);
							break; // We must break to ensure only one copy removed at a time.
						}
					}
					
					for (ArcIt eit(gi); eit != INVALID; ++eit){
						if(original_node_namei[gi.source(eit)] == *vit && original_node_namei[gi.target(eit)] == mapit->second){
							dest = gi.target(eit);
							//cout << "\t Remove arc: " << *vit << "-" << original_node_namei[gi.target(eit)] << endl;
							gi.erase(eit);
							break; // We must break to ensure only one copy removed at a time.
						}
					}
					//cout << "\t Add arc: " << original_node_namei[src] << "-" << original_node_namei[dest] << endl;
					Arc addarc = gi.addArc(src,dest);
					costi[addarc] = 1;
					capi[addarc] = 1;
					colori[addarc] = 0;
				}

				for (DNodeIt nit2(gi); nit2 != INVALID; ++nit2){
					if(original_node_namei[nit2] == *vit){
						gi.erase(nit2);
						break;
					}
				}

				// Update local-connectivity values if they increased somewhere
				for (DNodeIt nit3(gtest); nit3 != INVALID; ++nit3){
					for (DNodeIt nit2(gi); nit2 != INVALID; ++nit2){
						if(original_node_namei[nit2] == original_node_nametest[nit3]){
							if(local_connectivity_valuei[nit2] < local_connectivity_valuetest[nit3]){
								cout << "\t Connectivity of node " << original_node_namei[nit2] << " increased from " << local_connectivity_valuei[nit2] << " to " << local_connectivity_valuetest[nit3] << endl;
								local_connectivity_valuei[nit2] = local_connectivity_valuetest[nit3];
							}
							else if(local_connectivity_valuei[nit2] > local_connectivity_valuetest[nit3]){
								cout << "ERROR: Local connectivity decreased for node " << original_node_namei[nit2] << endl;
								return 0;
							}
							else{
								// Test
								// cout << "\t Connectivity of node " << original_node_namei[nit2] << " remained " << local_connectivity_valuei[nit2] << " from " << local_connectivity_valuetest[nit3] << endl;
							}
						}
					}
				}

				// Save the split-off permutation in the original graph G! Add back the removed self-loop arcs.
				for (DNodeIt nit2(g); nit2 != INVALID; ++nit2){
					if(original_node_name[nit2] == *vit){
						split_pairs[nit2] = (perit -> second);
						for(int i = 0; i < number_of_self_loops; i++){
							split_pairs[nit2].insert(make_pair(*vit,*vit));
						}
					}
				}
				break;
			}
		}
		//cout << "Tested_permutations: " << tested_permutations << endl << endl;
		total_tested_permutations += tested_permutations;
		if(!valid_splitoff_found){
			cout << "ERROR: no valid split-off found for node n" << *vit << endl;
			return 0;
		}

		// View the graph Gi (only for debugging).
		/*
		cout << "Graph Gi is: " << endl;
		for (DNodeIt nit2(gi); nit2 != INVALID; ++nit2) cout << "n" << original_node_namei[nit2] << endl;
		for (ArcIt eit(gi); eit != INVALID; ++eit) cout <<  original_node_namei[gi.source(eit)] << "-" << original_node_namei[gi.target(eit)] << endl;		
		int indegreei = 0;
		int outdegreei = 0;
		for(InArcIt eit(gi,nit); eit != INVALID; ++eit) indegreei++;
		for(OutArcIt eit(gi,nit); eit != INVALID; ++eit) outdegreei++;
		if(indegreei != outdegreei){
			cout << "ERROR! Moofified graph is not Eulerian! ERROR! " << endl;
			return 0;
		}
		*/	
	}

	if(view_results_on_stdout){
		// View the results of O2.
		cout << "Split-off permutation of O2: " << endl;
		for (DNodeIt nit2(g); nit2 != INVALID; ++nit2){
			cout << original_node_name[nit2] << ": ";
			for(multimap<int,int>::iterator mapit = split_pairs[nit2].begin(); mapit != split_pairs[nit2].end(); mapit++){
				cout << "(" << mapit->first << " " << mapit->second << ") ";
			}
			cout << endl;
		}
	}

	rusage timeAfterO2;
	getrusage(RUSAGE_SELF, &timeAfterO2);

	/*****************************************
	O3_build_arboresecences
	******************************************/
	// NOTE: Different implementation form Python, here every multi-edge is a separate edge, while there capacity represented the multiplicity.
	ArcVisitedNodesMap visited_path_nodes(gi);
	ArcArborescenceDistance arborescence_distance(gi);
	
	// Color the loop edges of the root.
	int new_color = 0;
	for (ArcIt eit(gi); eit != INVALID; ++eit){
		new_color++;
		colori[eit] = new_color;
		costi[eit] = 1;
		capi[eit] = 1;
		//cout <<  original_node_namei[gi.source(eit)] << "-" << original_node_namei[gi.target(eit)] << " [" << colori[eit] << "]" << endl;	
	}	
	int arborescence_number = new_color;

	for(vector<int>::reverse_iterator vit = splitoff_order.rbegin(); vit != splitoff_order.rend(); vit++){
		//cout << "Add node n" << *vit << " " << endl;

		for (DNodeIt nit2(g); nit2 != INVALID; ++nit2){
			if(original_node_name[nit2]== *vit){
				DNode n = gi.addNode();
				local_connectivity_valuei[n] = local_connectivity_value[nit2];
				original_node_namei[n] = original_node_name[nit2];

				for(multimap<int,int>::iterator mapit = split_pairs[nit2].begin(); mapit != split_pairs[nit2].end(); mapit++){
					//cout << "(" << mapit->first << " " << mapit->second << ") ";
					// If it was a loop on the newly added node, we add it back uncolored.
					if(mapit->first == *vit &&  mapit->second == *vit){
						
						Arc addarc = gi.addArc(n,n);
						costi[addarc] = 1;
						capi[addarc] = 1;
						colori[addarc] = 0;
					}
					// Otherwise we add the new edges and color them accordingly. We temporarily color out-edges as well.
					else{
						for (ArcIt eit(gi); eit != INVALID; ++eit){
							if(original_node_namei[gi.source(eit)] == mapit->first && original_node_namei[gi.target(eit)] == mapit -> second){
								int old_costi = costi[eit];
								int old_capi = capi[eit];
								int old_colori =  colori[eit];

								Arc addarc1 = gi.addArc(gi.source(eit),n);
								costi[addarc1] = old_costi;
								capi[addarc1] = old_capi;
								colori[addarc1] = old_colori;

								Arc addarc2 = gi.addArc(n,gi.target(eit));
								costi[addarc2] = old_costi;
								capi[addarc2] = old_capi;
								colori[addarc2] = old_colori;

								gi.erase(eit);
								break;
							}	
						}	
					}
				}
				//cout << endl;

				// We calculate the unique path along the arborescence and save it into visited_path_nodes
				set<int> used_colors;
				for(OutArcIt oit1(gi,n); oit1 != INVALID; ++oit1){
					if(colori[oit1] > 0){
						used_colors.insert(colori[oit1]);
						visited_path_nodes[oit1].clear();
						int next_node_id = original_node_namei[gi.target(oit1)];
						visited_path_nodes[oit1].push_back(next_node_id);

						while(next_node_id != root_id){
							// If we reach back to the added node, there is an ininite loop!
							if(next_node_id == *vit){
								visited_path_nodes[oit1].clear();
								break;
							}

							bool next_hop_found = false;
							for(DNodeIt pathit(gi); pathit != INVALID; ++pathit){
								if(!next_hop_found && original_node_namei[pathit] == next_node_id){
									for(OutArcIt oit3(gi,pathit); oit3 != INVALID; ++oit3){
										if(colori[oit3] == colori[oit1]){
											next_node_id = original_node_namei[gi.target(oit3)];
											visited_path_nodes[oit1].push_back(next_node_id);
											next_hop_found = true;
										}
									}
								}
							}
						}
					}
				}

				// We are resolving conflict on the outgoing edges immediately as we find two conflicing arcs.
				for(OutArcIt oit1(gi,n); oit1 != INVALID; ++oit1){
					if(colori[oit1] > 0){
						for(OutArcIt oit2(gi,n); oit2 != INVALID; ++oit2){
							if(oit1 != oit2 && colori[oit1] == colori[oit2]){
								if(visited_path_nodes[oit1].size() > 0 && visited_path_nodes[oit2].size() > 0){
									if(visited_path_nodes[oit1].size() <= visited_path_nodes[oit2].size()){
										colori[oit2] = 0;
									}
									else{
										colori[oit1] = 0;
									}
								}
								else if(visited_path_nodes[oit1].size() > 0 && visited_path_nodes[oit2].size() == 0){
									colori[oit2] = 0;
								}									
								else if(visited_path_nodes[oit1].size() == 0 && visited_path_nodes[oit2].size() > 0){
									colori[oit1] = 0;
								}
								else{
									colori[oit1] = 0;
									colori[oit2] = 0;
								}
							}
						}
					}
				}

				// Color uncolored edges (originally or becoming uncolored after removing conflicts) to have local connectivity number different trees.
				if(used_colors.size() < local_connectivity_valuei[n]){
					int trees_to_add = local_connectivity_valuei[n] - used_colors.size();
					// Calculate the colors (i.e., arborescenes) which were not used at the added node.
					set<int> available_colors;		
					for(int i = 1; i <= arborescence_number; i++){
						if(used_colors.count(i) == 0){
							available_colors.insert(i);
						}
					}

					// Calculate the distances for each available color through the uncolored out-edges.
					for(OutArcIt oit1(gi,n); oit1 != INVALID; ++oit1){
						if(colori[oit1] == 0){
							if(original_node_namei[gi.target(oit1)] == root_id){
								cout << "ERROR: In-edge of root become uncolored!" << endl;
								return 0;
							}

							for(OutArcIt oit2(gi,gi.target(oit1)); oit2 != INVALID; ++oit2){
								if(available_colors.count(colori[oit2]) > 0){
									int path_length = 2;
									int next_node_id = original_node_namei[gi.target(oit2)];
									
									while(next_node_id != root_id){
										bool next_hop_found = false;
										for(DNodeIt pathit(gi); pathit != INVALID; ++pathit){
											if(!next_hop_found && original_node_namei[pathit] == next_node_id){
												for(OutArcIt oit3(gi,pathit); oit3 != INVALID; ++oit3){
													if(colori[oit3] == colori[oit2]){
														next_node_id = original_node_namei[gi.target(oit3)];
														path_length++;
														next_hop_found = true;
													}
												}
											}
										}
									}
									
									// Save the path to the edge map
									arborescence_distance[oit1].insert(make_pair(colori[oit2],path_length));
								}
							}
						}
					}

					while(trees_to_add > 0){
						Arc best_arc;
						int best_tree_to_add = -1;
						int best_path_length = INT_MAX;
						for(OutArcIt oit1(gi,n); oit1 != INVALID; ++oit1){
							if(colori[oit1] == 0){
								for(map<int,int>::iterator mapit = arborescence_distance[oit1].begin(); mapit != arborescence_distance[oit1].end(); mapit++){
									if(available_colors.count(mapit->first) > 0){
										if(mapit->second < best_path_length){
											best_arc = oit1;
											best_tree_to_add = mapit->first;
											best_path_length = mapit->second;
										}
									}
								}	
							}
						}
						colori[best_arc] = best_tree_to_add;
						available_colors.erase(best_tree_to_add);
						trees_to_add--;
					}
				}
			}
		}
		
	}

	// Remove color from out-arcs of the root node
	for (DNodeIt nit2(gi); nit2 != INVALID; ++nit2){
		if(original_node_namei[nit2] == root_id){
			for(OutArcIt oit1(gi,nit2); oit1 != INVALID; ++oit1){
				colori[oit1] = 0;
			}
		}
	}
	
	if(view_results_on_stdout){
		// View the results.
		cout << "Routing arborescences of O3: " << endl;
		for (ArcIt eit(gi); eit != INVALID; ++eit){
			cout <<  original_node_namei[gi.source(eit)] << "-" << original_node_namei[gi.target(eit)] << " [" << colori[eit] << "]" << endl;	
		}	
	}



	rusage timeAfterO3;
	getrusage(RUSAGE_SELF, &timeAfterO3);

	/*****************************************
	O4_post_process
	******************************************/
	ArcBoolMap should_be_usei(gi);
	DNodeDistanceOnArborescence distance_on_arborescence(gi);

	// We "revese the arcs": as they are symmetric, we just swap the map values on the edges to make it faster.
	for(ArcIt ait1(gi);ait1 != INVALID;++ait1){
		for(ArcIt ait2(gi);ait2 != INVALID;++ait2){
			if(original_node_namei[gi.source(ait1)] == original_node_namei[gi.target(ait2)] && original_node_namei[gi.source(ait2)] == original_node_namei[gi.target(ait1)]){
				// We want to swap each bidirectional edge only once (assuming no loops)
				if(original_node_namei[gi.source(ait1)] <= original_node_namei[gi.source(ait2)]){
					int temp_color = colori[ait1];
					int temp_cost= costi[ait1];
					int temp_cap = capi[ait1];
					colori[ait1] = colori[ait2];
					costi[ait1] = costi[ait2];
					capi[ait1] = capi[ait2];
					colori[ait2] =temp_color;
					costi[ait2] = temp_cost;
					capi[ait2] = temp_cap;
				}
			}
		}
	}

	// Post-provess: remove the edges of all other arborescences, calculate an SPT in the remaining graph.
	for (DNodeIt nit3(gi); nit3 != INVALID; ++nit3){		
		if(original_node_namei[nit3] == root_id){
			for(int i = 1; i <= arborescence_number; i++){
				for(ArcIt ait1(gi);ait1 != INVALID;++ait1){
					if(colori[ait1] == i || colori[ait1] == 0){
						should_be_usei[ait1] = true;
						colori[ait1] = 0;
					}
					else{
						should_be_usei[ait1] = false;
					}
				}
				DFilteredGraph removed_subgraphi(gi, should_be_usei);
				DFilteredDijkstra dfdijkstra(removed_subgraphi,costi);
				dfdijkstra.run(nit3);

				for (DFilteredNodeIt nit2(removed_subgraphi); nit2 != INVALID; ++nit2){
					DFilteredNode n = dfdijkstra.predNode(nit2);
					//cout << original_node_namei[n] << "->" << original_node_namei[nit2]  << " " << dfdijkstra.dist(nit2) << endl;
					if(dfdijkstra.dist(nit2) > 0){
						for(DFilteredArcIt ait1(removed_subgraphi);ait1 != INVALID;++ait1){
							if(original_node_namei[n] == original_node_namei[removed_subgraphi.source(ait1)] && original_node_namei[nit2] == original_node_namei[removed_subgraphi.target(ait1)]){
								colori[ait1] = i;
							}
						}
					}
				}
			}
		}
	}
	
	// Precalculate the distances for the statistics along the arborescences from all source towards the root_id.
	for (DNodeIt nit3(gi); nit3 != INVALID; ++nit3){		
		if(original_node_namei[nit3] == root_id){
			for(int i = 1; i <= arborescence_number; i++){
				for(ArcIt ait1(gi);ait1 != INVALID;++ait1){
					if(colori[ait1] == i){
						should_be_usei[ait1] = true;
					}
					else{
						should_be_usei[ait1] = false;
					}
				}
				DFilteredGraph color_subgraphi(gi, should_be_usei);
				DFilteredDijkstra dfdijkstra(color_subgraphi,costi);
				dfdijkstra.run(nit3);

				for (DFilteredNodeIt nit2(color_subgraphi); nit2 != INVALID; ++nit2){
					//cout << "n" << original_node_namei[nit2] << " distance from n" << original_node_namei[nit3] << " on arborescence " << i << " is " << dfdijkstra.dist(nit2) << endl;
					distance_on_arborescence[nit2].insert(make_pair(i,dfdijkstra.dist(nit2)));
				}
			}
		}
	}

	// We "revese the arcs" back.
	for(ArcIt ait1(gi);ait1 != INVALID;++ait1){
		for(ArcIt ait2(gi);ait2 != INVALID;++ait2){
			if(original_node_namei[gi.source(ait1)] == original_node_namei[gi.target(ait2)] && original_node_namei[gi.source(ait2)] == original_node_namei[gi.target(ait1)]){
				// We want to swap each bidirectional edge only once (assuming no loops)
				if(original_node_namei[gi.source(ait1)] <= original_node_namei[gi.source(ait2)]){
					int temp_color = colori[ait1];
					int temp_cost= costi[ait1];
					int temp_cap = capi[ait1];
					colori[ait1] = colori[ait2];
					costi[ait1] = costi[ait2];
					capi[ait1] = capi[ait2];
					colori[ait2] =temp_color;
					costi[ait2] = temp_cost;
					capi[ait2] = temp_cap;
				}
			}
		}
	}

	// DEBUG ONLY: Check loss in coverage for validating the trees
	for(int i = 1; i <= arborescence_number; i++){
		for(ArcIt ait(gi);ait != INVALID;++ait){
			if(colori[ait] == 0){
				should_be_usei[ait] = false;
			}
			else{
				should_be_usei[ait] = true;
			}
		}
	}

	DFilteredGraph allowed_subgraphi(gi, should_be_usei);
	for (DNodeIt nfit1(gi); nfit1 != INVALID; ++nfit1){
		if(original_node_namei[nfit1] == root_id){
			for (DNodeIt nfit2(gi); nfit2 != INVALID; ++nfit2){
				if(nfit1 != nfit2){
					DFilteredEdmonds dfedmonds(allowed_subgraphi,capi,nfit2,nfit1);
					dfedmonds.run();

					if(dfedmonds.flowValue() == local_connectivity_valuei[nfit2]){
						//cout << "Valid solution from " <<  original_node_namei[nfit2] << " to " << original_node_namei[nfit1] << " with local connectivity " << dfedmonds.flowValue() << endl;
					}
					else{
						cout << "ERROR: Connectivity lost for " << root_id << " from " << local_connectivity_valuei[nfit2] << " to " << dfedmonds.flowValue() << endl;
						return 0;
					}					
				}
			}
		}
	}

	if(view_results_on_stdout){
		// View the results.
		cout << "Routing arborescences of O4: " << endl;
		for (ArcIt eit(gi); eit != INVALID; ++eit){
			cout <<  original_node_namei[gi.source(eit)] << "-" << original_node_namei[gi.target(eit)] << " [" << colori[eit] << "]" << endl;	
		}	
	}

	rusage timeAfterO4;
	getrusage(RUSAGE_SELF, &timeAfterO4);
	
	/*****************************************
	U2_calculate_statistics
	******************************************/
	// Save statistics
	for (DNodeIt nit2(gi); nit2 != INVALID; ++nit2){
		if(original_node_namei[nit2] != root_id){
			double minimum_edge_disjoint_path_length = edge_number*2;
			double minimum_arborescecne_path_length = edge_number*2;

			double average_edge_disjoint_path_length = 0;
			double average_arborescecne_path_length = 0;

			double maximum_edge_disjoint_path_length = 0;
			double  maximum_arborescecne_path_length = 0;

			double  edge_disjoint_path_number = 0;
			double  shortest_path_length = 0;

			// Save shortest path length	
			for (DNodeIt nit3(g); nit3 != INVALID; ++nit3){		
				if(original_node_name[nit3] == original_node_namei[nit2]){
					shortest_path_length = ddijkstra.dist(nit3);
				}	
			}

			overall_shortest_path_length += shortest_path_length;

			// Save arc-disjiont path length
			for (DNodeIt nit3(gi); nit3 != INVALID; ++nit3){		
				if(original_node_namei[nit3] == root_id){
					DSuurballe edge_disjoint_suurballe(gi,costi);
					edge_disjoint_suurballe.fullInit(nit2);
					int suurballe_path_num = edge_disjoint_suurballe.start(nit3,local_connectivity_valuei[nit2]);

					if(suurballe_path_num != local_connectivity_valuei[nit2]){
						cout << "ERROR: Suurballe fails to find local connectivity paths!" << endl;
						return 0;
					}

					//cout << "Suurbelle paths between " << original_node_namei[nit2] << " and root " << original_node_namei[nit3] << ":" << endl;
					for(int i = 0; i < suurballe_path_num; i++){
						//cout << "\t" <<(edge_disjoint_suurballe.path(i)).length() << endl;
						if((edge_disjoint_suurballe.path(i)).length() < minimum_edge_disjoint_path_length){
							minimum_edge_disjoint_path_length = (edge_disjoint_suurballe.path(i)).length();
						}
						if((edge_disjoint_suurballe.path(i)).length() > maximum_edge_disjoint_path_length){
							maximum_edge_disjoint_path_length = (edge_disjoint_suurballe.path(i)).length();
						}
						average_edge_disjoint_path_length += (edge_disjoint_suurballe.path(i)).length();
					}

					//cout <<  edge_disjoint_suurballe.totalLength() << " " << average_edge_disjoint_path_length << endl;
					average_edge_disjoint_path_length /= suurballe_path_num;
				}
			}
			overall_minimum_edge_disjoint_path_stretch += (minimum_edge_disjoint_path_length/shortest_path_length);
			overall_average_edge_disjoint_path_stretch += (average_edge_disjoint_path_length/shortest_path_length);
			overall_maximum_edge_disjoint_path_stretch += (maximum_edge_disjoint_path_length/shortest_path_length);
					
			// Save the arborescence path length
			int arborescence_path_number = 0;
			for(map<int,int>::iterator mapit = distance_on_arborescence[nit2].begin(); mapit != distance_on_arborescence[nit2].end(); mapit++){
				if(mapit->second > 0){
					arborescence_path_number++;
					if(mapit->second < minimum_arborescecne_path_length){
						minimum_arborescecne_path_length = mapit->second;
					}
					if(mapit->second > maximum_arborescecne_path_length){
						maximum_arborescecne_path_length = mapit->second;
					}
					average_arborescecne_path_length += mapit->second;
				}
			}	
			average_arborescecne_path_length /= arborescence_path_number;

			overall_minimum_arborescecne_path_stretch += (minimum_arborescecne_path_length/shortest_path_length);
			overall_average_arborescecne_path_stretch += (average_arborescecne_path_length/shortest_path_length);
			overall_maximum_arborescecne_path_stretch += (maximum_arborescecne_path_length/shortest_path_length);
			
			st_pairs++;
			/*
			cout << "\t\t\t<LocalConnectivity>"  <<  local_connectivity_valuei[nit2] <<  "</LocalConnectivity>\n";
			cout << "\t\t\t<ShortestPathLength>"  <<  shortest_path_length   <<  "</ShortestPathLength>\n";
			cout << "\t\t\t<MinimumEdgeDisjointPathLength>"  <<  minimum_edge_disjoint_path_length  <<  "</MinimumEdgeDisjointPathLength>\n";
			cout << "\t\t\t<MinimumArborescencePathLength>"  <<  minimum_arborescecne_path_length   <<  "</MinimumArborescencePathLength>\n";
			cout << "\t\t\t<AverageEdgeDisjointPathLength>"  <<  average_edge_disjoint_path_length  << "</AverageEdgeDisjointPathLength>\n";
			cout << "\t\t\t<AverageArborescencePathLength>"  <<  average_arborescecne_path_length   <<  "</AverageArborescencePathLength>\n";
			cout << "\t\t\t<MaximumEdgeDisjointPathLength>"  <<  maximum_edge_disjoint_path_length   <<  "</MaximumEdgeDisjointPathLength>\n";
			cout << "\t\t\t<MaximumArborescencePathLength>"  <<  maximum_arborescecne_path_length  << "</MaximumArborescencePathLength>\n";
			*/

			os << "\t\t<Paths>\n";
			os << "\t\t\t<SourceNode>" << original_node_namei[nit2] << "</SourceNode>\n";
			os << "\t\t\t<RootNode>" <<  root_id <<  "</RootNode>\n";
			os << "\t\t\t<LocalConnectivity>"  <<  local_connectivity_valuei[nit2] <<  "</LocalConnectivity>\n";
			os << "\t\t\t<ShortestPathLength>"  <<  shortest_path_length   <<  "</ShortestPathLength>\n";
			os << "\t\t\t<MinimumEdgeDisjointPathLength>"  <<  minimum_edge_disjoint_path_length  <<  "</MinimumEdgeDisjointPathLength>\n";
			os << "\t\t\t<MinimumArborescencePathLength>"  <<  minimum_arborescecne_path_length   <<  "</MinimumArborescencePathLength>\n";
			os << "\t\t\t<AverageEdgeDisjointPathLength>"  <<  average_edge_disjoint_path_length  << "</AverageEdgeDisjointPathLength>\n";
			os << "\t\t\t<AverageArborescencePathLength>"  <<  average_arborescecne_path_length    <<  "</AverageArborescencePathLength>\n";
			os << "\t\t\t<MaximumEdgeDisjointPathLength>"  <<  maximum_edge_disjoint_path_length   <<  "</MaximumEdgeDisjointPathLength>\n";
			os << "\t\t\t<MaximumArborescencePathLength>"  <<  maximum_arborescecne_path_length  << "</MaximumArborescencePathLength>\n";
			os << "\t\t</Paths>\n";	
		}
	}
	

	rusage iterEndTime;
	getrusage(RUSAGE_SELF, &iterEndTime);

	auto end_time_chrono_iter = chrono::steady_clock::now();
	chrono::duration<double> runTime_Chrono_iter = end_time_chrono_iter - start_time_chrono_iter;

	cout << "-- Root " << root_id << " calculated in " << runTime_Chrono_iter .count() << " seconds ---" << endl; //<< timeDiff(iterStartTime, iterEndTime) << " seconds ---" << endl;

	lgf_file.close();

	return 1;
}


int main(int argc, char** argv){
	auto start_time_chrono = chrono::steady_clock::now();
	if (argc<=1) {
		cout <<"Usage: "<<argv[0]<<" --help \n";
		return -1;
	}

	string lgf_file_name= argv[1];

	// We read the directed graph from the .lgf file.
	ifstream lgf_file(lgf_file_name.c_str());
	if (!lgf_file) {
		cout << "Error opening lgf file of "<< lgf_file_name;
		return 0;
	}

	rusage enviromentBuildStartTime;
	getrusage(RUSAGE_SELF, &enviromentBuildStartTime);
	
    Digraph g;

    ArcCostMap cost(g);
	ArcCapMap cap(g);  
	ArcColorMap color(g);

    DNodeCoordMap coords(g);
	DNodeSplitPairsMap split_pairs(g); 
	DNodeConnectivityMap local_connectivity_value(g);
	DNodeNameMap original_node_name(g);

	DigraphReader<Digraph> digraphreader(g, lgf_file);
    //digraphreader.nodeMap("coords", coords).run();
	digraphreader.run();

    for (DNodeIt nit(g); nit != INVALID; ++nit){
        node_number++;
		split_pairs[nit].clear();
		local_connectivity_value[nit] = 0;
		original_node_name[nit] = g.id(nit);
        //cout << "n" << g.id(nit) << endl;

		int indegree = 0;
		int outdegree = 0;

		for(InArcIt eit(g,nit); eit != INVALID; ++eit) indegree++;
		for(OutArcIt eit(g,nit); eit != INVALID; ++eit) outdegree++;

		if(indegree != outdegree){
			cout << "ERROR! Input graph is not symmetric directed graph! ERROR! " << endl;
			return 0;
		}
		if(indegree == 1){
			cout << "WARNING: Graph is not 2-connected at " << original_node_name[nit] << endl;
		}
		//else cout << "n" << g.id(nit) << " degree is " << indegree << endl;
    }

    for (ArcIt eit(g); eit != INVALID; ++eit){
		edge_number++;
		cost[eit] = 1;
		cap[eit] = 1;
		color[eit] = 0;
        //cout << g.id(g.source(eit)) << "-" << g.id(g.target(eit)) << endl;
    }
    edge_number /= 2; // As the graph is symmetric directed graph.

	ostringstream out_file_name;
	out_file_name << lgf_file_name.substr(0, lgf_file_name.size() - 4);

	out_file_name << ".xml";
	ofstream os(out_file_name.str().c_str());
	if (!os)
	{
		exit(-1); // abnormal exit: error opening file
	}
	os << "<Simulator>\n";
	os << "\t<Algorithm>" << "pra" << "</Algorithm>\n";
	os << "\t<Network>\n";
	os << "\t\t<TopologyName>" << lgf_file_name.substr(0, lgf_file_name.size() - 4) << "</TopologyName>\n";
	os << "\t\t<NodeNum>" << node_number << "</NodeNum>\n";
	os << "\t\t<EdgeNum>" << edge_number << "</EdgeNum>\n";
	os << "\t</Network>\n";
	os << "\t<Statistics>\n";

	rusage enviromentBuildEndTime;
	getrusage(RUSAGE_SELF, &enviromentBuildEndTime);

	vector<thread> threads_per_root;
	int investigated_roots = 0;
	int concurrent_threads = 0;
	int max_thread_number = 1;

    for (DNodeIt nit(g); nit != INVALID; ++nit){
		int root_id = g.id(nit);
		//if(root_id < 20){
			investigated_roots++;
			calculate_arborescences_for_root(root_id,lgf_file_name,os);	
		//}
	}

	rusage finaltime;
	getrusage(RUSAGE_SELF, &finaltime);

	cout << "st_pairs: " << st_pairs << endl;

	// Save the results.
	double runTime = timeDiff(enviromentBuildEndTime, finaltime);

	auto end_time_chrono = chrono::steady_clock::now();
	chrono::duration<double> runTime_Chrono = end_time_chrono - start_time_chrono;

	os << "\t</Statistics>\n";
	os << "\t<TotalTestedPermutations>" << total_tested_permutations << "</TotalTestedPermutations>\n";
	os << "\t<TotalAllPermutations>" << total_all_permutations << "</TotalAllPermutations>\n";
	
	os << "\t<OverallShortestPathLength>"  <<  overall_shortest_path_length/(node_number*(node_number-1))   <<  "</OverallShortestPathLength>\n";
	os << "\t<OverallMinimumEdgeDisjointPathStretch>"  <<  overall_minimum_edge_disjoint_path_stretch/(node_number*(node_number-1)) <<  "</OverallMinimumEdgeDisjointPathStretch>\n";
	os << "\t<OverallMinimumArborescencePathStretch>"  <<  overall_minimum_arborescecne_path_stretch/(node_number*(node_number-1))  <<  "</OverallMinimumArborescencePathStretch>\n";
	os << "\t<OverallAverageEdgeDisjointPathStretch>"  <<  overall_average_edge_disjoint_path_stretch/(node_number*(node_number-1))  << "</OverallAverageEdgeDisjointPathStretch>\n";
	os << "\t<OverallAverageArborescencePathStretch>"  <<  overall_average_arborescecne_path_stretch/(node_number*(node_number-1))  <<  "</OverallAverageArborescencePathStretch>\n";
	os << "\t<OverallMaximumEdgeDisjointPathStretch>"  <<  overall_maximum_edge_disjoint_path_stretch/(node_number*(node_number-1)) <<  "</OverallMaximumEdgeDisjointPathStretch>\n";
	os << "\t<OverallMaximumArborescencePathStretch>"  <<  overall_maximum_arborescecne_path_stretch/(node_number*(node_number-1))   << "</OverallMaximumArborescencePathStretch>\n";
		
	os << "\t<MaxThreadNumber>" << max_thread_number << "</MaxThreadNumber>\n";
	os << "\t<TotalRunningTime>" << runTime << "</TotalRunningTime>\n";
	os << "\t<TotalRunningTimeChrono>" << runTime_Chrono.count() << "</TotalRunningTimeChrono>\n";
	os << "\t<AvgRunningTimePerRoot>" << runTime/investigated_roots << "</AvgRunningTimePerRoot>\n";
	os << "\t<AvgRunningTimePerRootChrono>" << runTime_Chrono.count()/investigated_roots << "</AvgRunningTimePerRootChrono>\n";
	os << "</Simulator>\n";
	os.close();

	lgf_file.close();

	return 0;
};

