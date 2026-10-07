// ============================================================
//  Route.h  -  STEP 2 + 3: Bus routes (Part I) and Train routes (Part II)
//  IS2202 Group Assignment - Problem C
// ============================================================
#ifndef ROUTE_H
#define ROUTE_H

#include "CityGraph.h"

// ---------- One bus (or train) route ----------
struct Route {
    std::string id;          // e.g. "101"
    Mode mode;               // BUS or TRAIN
    std::vector<int> stops;  // node ids in travel order
    int frequency;           // a vehicle every X minutes
    int capacity;            // passengers per vehicle
    double speed;            // average speed (km/h)
};

// ---------- Collection of routes (the transport network) ----------
class TransportNetwork {
private:
    const CityGraph& city;
    std::vector<Route> routes;

    // Distance between two neighbouring stops using an edge of the given mode.
    // Returns -1 if there is no such road/track.
    double edgeDistance(int a, int b, Mode mode) const {
        for (const auto& e : city.getEdges(a))
            if (e.to == b && e.mode == mode) return e.distance;
        return -1;
    }

public:
    TransportNetwork(const CityGraph& c) : city(c) {}

    // Add a route after checking every pair of stops is really connected
    bool addRoute(const Route& r) {
        for (size_t i = 0; i + 1 < r.stops.size(); i++) {
            if (edgeDistance(r.stops[i], r.stops[i + 1], r.mode) < 0) {
                std::cout << "ERROR: Route " << r.id << " - no "
                          << (r.mode == BUS ? "road" : "track") << " between "
                          << city.getLocation(r.stops[i]).name << " and "
                          << city.getLocation(r.stops[i + 1]).name << "\n";
                return false;
            }
        }
        routes.push_back(r);
        return true;
    }

    // Total length of a route (km)
    double routeLength(const Route& r) const {
        double total = 0;
        for (size_t i = 0; i + 1 < r.stops.size(); i++)
            total += edgeDistance(r.stops[i], r.stops[i + 1], r.mode);
        return total;
    }

    // One-way travel time (minutes) = distance / speed * 60
    double routeTime(const Route& r) const {
        return routeLength(r) / r.speed * 60.0;
    }

    const std::vector<Route>& getRoutes() const { return routes; }

    // Change how often a route runs (used for "what if" experiments)
    void setFrequency(const std::string& id, int minutes) {
        for (auto& r : routes) if (r.id == id) r.frequency = minutes;
    }

    // Print each route with its stops
    void printRoutes(Mode mode) const {
        std::cout << "\n===== " << (mode == BUS ? "BUS" : "TRAIN") << " ROUTES =====\n";
        for (const auto& r : routes) {
            if (r.mode != mode) continue;
            std::cout << "Route " << r.id << ": ";
            for (size_t i = 0; i < r.stops.size(); i++) {
                std::cout << city.getLocation(r.stops[i]).name;
                if (i + 1 < r.stops.size()) std::cout << " -> ";
            }
            std::cout << "\n";
        }
    }

    // Transfer points = locations served by at least one BUS route AND one TRAIN route.
    // Passengers can change mode (bus <-> train) only at these places.
    void printTransferPoints() const {
        std::cout << "\n===== TRANSFER POINTS (BUS <-> TRAIN) =====\n";
        for (int node = 0; node < city.size(); node++) {
            std::vector<std::string> busRoutes, trainRoutes;
            for (const auto& r : routes) {
                for (int s : r.stops) {
                    if (s == node) {
                        if (r.mode == BUS) busRoutes.push_back(r.id);
                        else trainRoutes.push_back(r.id);
                        break;
                    }
                }
            }
            if (!busRoutes.empty() && !trainRoutes.empty()) {
                std::cout << std::left << std::setw(20) << city.getLocation(node).name << " Train: ";
                for (auto& t : trainRoutes) std::cout << t << " ";
                std::cout << "| Bus: ";
                for (auto& b : busRoutes) std::cout << b << " ";
                std::cout << "\n";
            }
        }
    }

    // Print summary table: length, time, frequency, capacity per hour
    void printSummary(Mode mode) const {
        std::cout << "\n===== " << (mode == BUS ? "BUS" : "TRAIN") << " ROUTE SUMMARY =====\n";
        std::cout << std::left << std::setw(8) << "Route" << std::setw(7) << "Stops"
                  << std::setw(10) << "Length" << std::setw(11) << "Trip Time"
                  << std::setw(11) << "Every" << std::setw(10) << "Avg Wait"
                  << "Capacity/hour\n";
        std::cout << std::fixed << std::setprecision(1);
        for (const auto& r : routes) {
            if (r.mode != mode) continue;
            double avgWait = r.frequency / 2.0;             // on average wait half the gap
            int perHour = (60 / r.frequency) * r.capacity;  // seats offered per hour, one direction
            std::cout << std::setw(8) << r.id
                      << std::setw(7) << r.stops.size()
                      << std::setw(10) << (std::to_string((int)routeLength(r)) + " km")
                      << std::setw(11) << (std::to_string((int)(routeTime(r) + 0.5)) + " min")
                      << std::setw(11) << (std::to_string(r.frequency) + " min")
                      << std::setw(10) << (std::to_string((int)(avgWait + 0.5)) + " min")
                      << perHour << "\n";
        }
    }
};

#endif