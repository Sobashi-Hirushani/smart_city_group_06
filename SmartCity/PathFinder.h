// ============================================================
//  PathFinder.h  -  STEP 4: Dijkstra shortest (fastest) path
//  Finds the quickest journey using bus + train routes,
//  including waiting time and transfers.
//  IS2202 Group Assignment - Problem C
// ============================================================
#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "Route.h"
#include <queue>
#include <limits>
#include <algorithm>

const double TRANSFER_PENALTY = 3.0;   // minutes to walk between vehicles/platforms
const double PEAK_TRAFFIC     = 1.5;   // buses are 50% slower in peak-hour traffic (trains are not affected)

// Peak hours: 07:00-09:00 and 17:00-19:00
inline bool isPeakHour(int hour) { return (hour >= 7 && hour < 9) || (hour >= 17 && hour < 19); }

// One part of a journey on a single vehicle (e.g. "BUS 101 from A to B")
struct Leg {
    int routeIndex;          // which route (index in network.getRoutes())
    std::vector<int> stops;  // nodes visited on this leg, in order
    double waitTime;         // minutes waited before boarding
    double rideTime;         // minutes inside the vehicle
};

// Full result for one passenger
struct Journey {
    bool found = false;
    std::vector<Leg> legs;
    double totalTime = 0;    // wait + ride + transfer penalties
    double waitTime = 0;
    double rideTime = 0;
    int transfers = 0;       // number of vehicle changes = legs - 1
};

class PathFinder {
private:
    const CityGraph& city;
    const TransportNetwork& network;
    int R;                   // number of routes

    // A "state" = (location, route I am currently on).
    // route = R means "not on any vehicle yet" (at the start).
    // State id = node * (R + 1) + route
    int stateId(int node, int route) const { return node * (R + 1) + route; }

    // Minutes to ride one segment of a route (buses slower in peak traffic)
    double segmentTime(const Route& r, int a, int b, bool peak) const {
        for (const auto& e : city.getEdges(a))
            if (e.to == b && e.mode == r.mode) {
                double t = e.distance / r.speed * 60.0;
                if (peak && r.mode == BUS) t *= PEAK_TRAFFIC;
                return t;
            }
        return std::numeric_limits<double>::infinity();
    }

public:
    PathFinder(const CityGraph& c, const TransportNetwork& n)
        : city(c), network(n), R(n.getRoutes().size()) {}

    // ---------------- DIJKSTRA ----------------
    // departTime = minutes after midnight (-1 = ignore traffic)
    Journey findPath(int origin, int destination, int departTime = -1) const {
        bool peak = departTime >= 0 && isPeakHour(departTime / 60);
        const auto& routes = network.getRoutes();
        const double INF = std::numeric_limits<double>::infinity();
        int S = city.size() * (R + 1);

        std::vector<double> dist(S, INF);   // best time found to each state
        std::vector<int> parent(S, -1);     // previous state (to rebuild path)

        // Min-heap: (time, state). Smallest time comes out first.
        std::priority_queue<std::pair<double, int>,
                            std::vector<std::pair<double, int>>,
                            std::greater<>> pq;

        int start = stateId(origin, R);
        dist[start] = 0;
        pq.push({0, start});
        int endState = -1;

        while (!pq.empty()) {
            auto [d, s] = pq.top();
            pq.pop();
            if (d > dist[s]) continue;               // old entry, skip

            int node = s / (R + 1);
            int cur  = s % (R + 1);                  // current route (R = none)

            if (node == destination && cur != R) {   // reached with a vehicle
                endState = s;
                break;
            }

            // (a) Board / change to another route that stops here
            for (int r = 0; r < R; r++) {
                if (r == cur) continue;
                const auto& st = routes[r].stops;
                if (std::find(st.begin(), st.end(), node) == st.end()) continue;
                double cost = routes[r].frequency / 2.0;       // average wait
                if (cur != R) cost += TRANSFER_PENALTY;        // it is a transfer
                int ns = stateId(node, r);
                if (d + cost < dist[ns]) {
                    dist[ns] = d + cost;
                    parent[ns] = s;
                    pq.push({dist[ns], ns});
                }
            }

            // (b) Stay on the current route and ride to the next/previous stop
            if (cur != R) {
                const auto& st = routes[cur].stops;
                int pos = std::find(st.begin(), st.end(), node) - st.begin();
                for (int np : {pos - 1, pos + 1}) {          // routes run both ways
                    if (np < 0 || np >= (int)st.size()) continue;
                    double cost = segmentTime(routes[cur], node, st[np], peak);
                    int ns = stateId(st[np], cur);
                    if (d + cost < dist[ns]) {
                        dist[ns] = d + cost;
                        parent[ns] = s;
                        pq.push({dist[ns], ns});
                    }
                }
            }
        }

        Journey j;
        if (endState == -1) return j;                // no path
        j.found = true;
        j.totalTime = dist[endState];

        // ---- Rebuild the path by walking parents backwards ----
        std::vector<int> states;
        for (int s = endState; s != -1; s = parent[s]) states.push_back(s);
        std::reverse(states.begin(), states.end());

        for (size_t i = 1; i < states.size(); i++) {
            int prevRoute = states[i - 1] % (R + 1);
            int node = states[i] / (R + 1),         route = states[i] % (R + 1);
            double step = dist[states[i]] - dist[states[i - 1]];

            if (route != prevRoute) {                // boarded a new vehicle
                Leg leg{route, {node}, routes[route].frequency / 2.0, 0};
                j.legs.push_back(leg);
                j.waitTime += leg.waitTime;
            } else {                                 // rode one segment
                j.legs.back().stops.push_back(node);
                j.legs.back().rideTime += step;
                j.rideTime += step;
            }
        }
        j.transfers = j.legs.size() - 1;
        return j;
    }

    // Print a journey like: A -> (TRAIN RED) -> B -> (BUS 101) -> C | 34 min
    void printJourney(int origin, int destination, const Journey& j) const {
        const auto& routes = network.getRoutes();
        if (!j.found) {
            std::cout << city.getLocation(origin).name << " -> "
                      << city.getLocation(destination).name << " : NO ROUTE\n";
            return;
        }
        std::cout << city.getLocation(origin).name;
        for (const auto& leg : j.legs) {
            const Route& r = routes[leg.routeIndex];
            std::cout << " -> (" << (r.mode == BUS ? "BUS " : "TRAIN ") << r.id << ") -> "
                      << city.getLocation(leg.stops.back()).name;
        }
        std::cout << std::fixed << std::setprecision(0)
                  << " | " << j.totalTime << " min (wait " << j.waitTime
                  << ", ride " << j.rideTime << "), " << j.transfers << " transfer(s)\n";
    }
};

#endif