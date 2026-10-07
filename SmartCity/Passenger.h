// ============================================================
//  Passenger.h  -  STEP 5: Passenger demand simulation (Part III)
//  Generates passengers for a full day (06:00 - 22:00) with
//  morning / evening peaks and finds each one's journey.
//  IS2202 Group Assignment - Problem C
// ============================================================
#ifndef PASSENGER_H
#define PASSENGER_H

#include "PathFinder.h"
#include <random>
#include <fstream>
#include <sstream>

// ---------- One passenger ----------
struct Passenger {
    int id;
    int origin;
    int destination;
    int departTime;      // minutes after midnight (e.g. 7:30 = 450)
    Journey journey;     // filled by Dijkstra
};

// Convert minutes-after-midnight to "HH:MM"
std::string toClock(double minutes) {
    int m = (int)(minutes + 0.5);
    std::ostringstream out;
    out << std::setfill('0') << std::setw(2) << (m / 60) % 24 << ":"
        << std::setw(2) << m % 60;
    return out.str();
}

// ---------- Demand generator ----------
class DemandGenerator {
private:
    std::vector<int> residential;   // where people live
    std::vector<int> workplaces;    // where people work / study
    std::vector<int> allPlaces;     // every location
    std::mt19937 rng;               // random number generator

    int pick(const std::vector<int>& list) {
        std::uniform_int_distribution<int> d(0, list.size() - 1);
        return list[d(rng)];
    }
    bool chance(double p) {         // true with probability p
        std::uniform_real_distribution<double> d(0, 1);
        return d(rng) < p;
    }

public:
    DemandGenerator(std::vector<int> res, std::vector<int> work, int numLocations, int seed)
        : residential(res), workplaces(work), rng(seed) {
        for (int i = 0; i < numLocations; i++) allPlaces.push_back(i);
    }

    // How many passengers start travelling in each hour (the demand pattern)
    static int passengersInHour(int hour) {
        if (hour == 7 || hour == 8)   return 160;   // morning peak
        if (hour == 17 || hour == 18) return 150;   // evening peak
        if (hour == 6 || hour == 9 || hour == 16 || hour == 19) return 70;  // shoulder
        return 35;                                  // off-peak (day & night)
    }

    static std::string periodName(int hour) {
        if (hour >= 7 && hour < 9)   return "Morning Peak";
        if (hour >= 17 && hour < 19) return "Evening Peak";
        return "Off-Peak";
    }

    // Generate all passengers from 06:00 to 22:00
    std::vector<Passenger> generateDay() {
        std::vector<Passenger> list;
        std::uniform_int_distribution<int> minute(0, 59);

        for (int hour = 6; hour < 22; hour++) {
            int count = passengersInHour(hour);
            for (int k = 0; k < count; k++) {
                Passenger p;
                p.id = 0;
                p.departTime = hour * 60 + minute(rng);

                if (hour >= 6 && hour < 10 && chance(0.8)) {
                    // Morning: 80% go from home to work
                    p.origin = pick(residential);
                    p.destination = pick(workplaces);
                } else if (hour >= 16 && hour < 20 && chance(0.8)) {
                    // Evening: 80% go from work back home
                    p.origin = pick(workplaces);
                    p.destination = pick(residential);
                } else {
                    // Other trips: shopping, hospital, airport... anywhere
                    p.origin = pick(allPlaces);
                    do { p.destination = pick(allPlaces); } while (p.destination == p.origin);
                }
                list.push_back(p);
            }
        }
        // Sort by departure time so the log reads in time order
        std::sort(list.begin(), list.end(),
                  [](const Passenger& a, const Passenger& b) { return a.departTime < b.departTime; });
        for (size_t i = 0; i < list.size(); i++) list[i].id = i + 1;   // number them in time order
        return list;
    }
};

// One line of the log, e.g.
// [07:03] P0012: North Residential -> (TRAIN RED) -> Central Hub | arrive 07:25, 22 min, 0 transfer(s)
std::string journeyLine(const Passenger& p, const CityGraph& city, const TransportNetwork& net) {
    std::ostringstream out;
    out << "[" << toClock(p.departTime) << "] P" << std::setfill('0') << std::setw(4) << p.id
        << std::setfill(' ') << ": " << city.getLocation(p.origin).name;
    if (!p.journey.found) { out << " -> " << city.getLocation(p.destination).name << " | NO ROUTE"; return out.str(); }
    for (const auto& leg : p.journey.legs) {
        const Route& r = net.getRoutes()[leg.routeIndex];
        out << " -> (" << (r.mode == BUS ? "BUS " : "TRAIN ") << r.id << ") -> "
            << city.getLocation(leg.stops.back()).name;
    }
    out << std::fixed << std::setprecision(0)
        << " | arrive " << toClock(p.departTime + p.journey.totalTime)
        << ", " << p.journey.totalTime << " min, " << p.journey.transfers << " transfer(s)";
    return out.str();
}

#endif