// ============================================================
//  CityGraph.h  -  STEP 1: Smart city represented as a graph
//  IS2202 Group Assignment - Problem C
// ============================================================
#ifndef CITYGRAPH_H
#define CITYGRAPH_H

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

// Transport mode of an edge
enum Mode { BUS, TRAIN };

// ---------- Node (vertex) ----------
struct Location {
    int id;
    std::string name;
    bool hasBusStop;
    bool hasTrainStation;
};

// ---------- Edge ----------
struct Edge {
    int to;            // destination node id
    double distance;   // km
    Mode mode;         // BUS road or TRAIN track
};

// ---------- Graph ----------
class CityGraph {
private:
    std::vector<Location> locations;          // all nodes
    std::vector<std::vector<Edge>> adj;       // adjacency list

public:
    // Add a node and return its id
    int addLocation(const std::string& name, bool bus, bool train) {
        int id = locations.size();
        locations.push_back({id, name, bus, train});
        adj.push_back({});                    // empty edge list for new node
        return id;
    }

    // Road: two-way edge used by buses
    void addRoad(int a, int b, double km) {
        adj[a].push_back({b, km, BUS});
        adj[b].push_back({a, km, BUS});
    }

    // Railway track: two-way edge used by trains
    void addRail(int a, int b, double km) {
        adj[a].push_back({b, km, TRAIN});
        adj[b].push_back({a, km, TRAIN});
    }

    int size() const { return locations.size(); }
    const Location& getLocation(int id) const { return locations[id]; }
    const std::vector<Edge>& getEdges(int id) const { return adj[id]; }

    // Print all nodes
    void printLocations() const {
        std::cout << "\n===== LOCATIONS (NODES) =====\n";
        std::cout << std::left << std::setw(4) << "ID" << std::setw(22) << "Name"
                  << std::setw(8) << "Bus" << "Train\n";
        for (const auto& l : locations) {
            std::cout << std::setw(4) << l.id << std::setw(22) << l.name
                      << std::setw(8) << (l.hasBusStop ? "Yes" : "-")
                      << (l.hasTrainStation ? "Yes" : "-") << "\n";
        }
    }

    // Print adjacency list (the graph itself)
    void printGraph() const {
        std::cout << "\n===== ADJACENCY LIST (EDGES) =====\n";
        for (int i = 0; i < size(); i++) {
            std::cout << locations[i].name << " -> ";
            for (const auto& e : adj[i]) {
                std::cout << locations[e.to].name << "("
                          << e.distance << "km," << (e.mode == BUS ? "Bus" : "Train") << ")  ";
            }
            std::cout << "\n";
        }
    }
};

#endif