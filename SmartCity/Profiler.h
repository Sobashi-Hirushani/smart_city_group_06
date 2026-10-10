// ============================================================
//  Profiler.h  -  STEP 6: Efficiency profiling (Part IV)
//  Travel time, waiting time, transfers, peak vs off-peak,
//  route load factor (crowding) and busiest stations.
//  Also writes CSV files that can be opened in Excel for charts.
//  IS2202 Group Assignment - Problem C
// ============================================================
#ifndef PROFILER_H
#define PROFILER_H

#include "Passenger.h"
#include <map>

// Each simulated passenger represents this many real passengers.
// (We simulate 1180 people; the real city has ~20x more.)
const int SCALE = 20;

class Profiler {
private:
    const CityGraph& city;
    const TransportNetwork& net;
    const std::vector<Passenger>& passengers;

    // Simple running averages
    struct Stats {
        int count = 0;
        double time = 0, wait = 0, ride = 0, transfers = 0;
        void add(const Journey& j) {
            count++; time += j.totalTime; wait += j.waitTime;
            ride += j.rideTime; transfers += j.transfers;
        }
        double avg(double v) const { return count ? v / count : 0; }
    };

public:
    Profiler(const CityGraph& c, const TransportNetwork& n, const std::vector<Passenger>& p)
        : city(c), net(n), passengers(p) {}

    // ---------- 1. Overall + peak vs off-peak ----------
    void printPeriodSummary() const {
        std::map<std::string, Stats> byPeriod;
        Stats all;
        int busOnly = 0, trainOnly = 0, mixed = 0;

        for (const auto& p : passengers) {
            if (!p.journey.found) continue;
            all.add(p.journey);
            byPeriod[DemandGenerator::periodName(p.departTime / 60)].add(p.journey);

            bool usedBus = false, usedTrain = false;
            for (const auto& leg : p.journey.legs)
                (net.getRoutes()[leg.routeIndex].mode == BUS ? usedBus : usedTrain) = true;
            if (usedBus && usedTrain) mixed++;
            else if (usedBus) busOnly++;
            else trainOnly++;
        }

        std::cout << "\n===== EFFICIENCY SUMMARY: PEAK vs OFF-PEAK =====\n";
        std::cout << std::left << std::setw(16) << "Period" << std::setw(12) << "Passengers"
                  << std::setw(12) << "Avg Time" << std::setw(12) << "Avg Wait"
                  << std::setw(12) << "Avg Ride" << "Avg Transfers\n";
        std::cout << std::fixed << std::setprecision(1);
        auto row = [&](const std::string& name, const Stats& s) {
            std::cout << std::setw(16) << name << std::setw(12) << s.count
                      << std::setw(12) << s.avg(s.time) << std::setw(12) << s.avg(s.wait)
                      << std::setw(12) << s.avg(s.ride) << std::setprecision(2)
                      << s.avg(s.transfers) << std::setprecision(1) << "\n";
        };
        for (std::string name : {"Morning Peak", "Evening Peak", "Off-Peak"}) row(name, byPeriod[name]);
        row("ALL DAY", all);
        std::cout << "(times in minutes)\n";

        int total = busOnly + trainOnly + mixed;
        std::cout << "\nMode share: Bus only " << 100.0 * busOnly / total << "%, "
                  << "Train only " << 100.0 * trainOnly / total << "%, "
                  << "Bus + Train " << 100.0 * mixed / total << "%\n";
    }

    // ---------- 2. Hourly table (+ CSV for a line chart) ----------
    void printHourly() const {
        std::map<int, Stats> byHour;
        for (const auto& p : passengers)
            if (p.journey.found) byHour[p.departTime / 60].add(p.journey);

        std::ofstream csv("hourly_stats.csv");
        csv << "Hour,Passengers,RealPassengers,AvgTime,AvgWait\n";

        std::cout << "\n===== HOURLY PROFILE =====\n";
        std::cout << std::left << std::setw(8) << "Hour" << std::setw(12) << "Passengers"
                  << std::setw(10) << "Avg Time" << std::setw(10) << "Avg Wait" << "Demand\n";
        for (auto& [h, s] : byHour) {
            std::cout << std::setw(8) << toClock(h * 60) << std::setw(12) << s.count * SCALE
                      << std::setw(10) << s.avg(s.time) << std::setw(10) << s.avg(s.wait)
                      << std::string(s.count / 10, '#') << "\n";          // text bar chart
            csv << toClock(h * 60) << "," << s.count << "," << s.count * SCALE << ","
                << s.avg(s.time) << "," << s.avg(s.wait) << "\n";
        }
        std::cout << "(Passengers = real passengers, each simulated passenger x" << SCALE << ")\n";
    }

    // ---------- 3. Route load factor (how full the vehicles are) ----------
    // For every route, segment and hour we count riders. The busiest segment
    // decides how full the vehicle is. Load factor = riders / seats offered.
    void printRouteLoad(const std::string& csvFile = "route_load.csv") const {
        const auto& routes = net.getRoutes();
        // load[route][hour][segment key] = riders
        std::vector<std::map<int, std::map<std::pair<int,int>, int>>> load(routes.size());

        for (const auto& p : passengers) {
            if (!p.journey.found) continue;
            double t = p.departTime;
            for (const auto& leg : p.journey.legs) {
                t += leg.waitTime;
                int hour = (int)t / 60;
                for (size_t i = 0; i + 1 < leg.stops.size(); i++)
                    load[leg.routeIndex][hour][{leg.stops[i], leg.stops[i + 1]}]++;
                t += leg.rideTime + TRANSFER_PENALTY;
            }
        }

        std::ofstream csv(csvFile);
        csv << "Route,Mode,PeakHourLoadFactor,OffPeakLoadFactor\n";

        std::cout << "\n===== ROUTE LOAD FACTOR (how full vehicles are) =====\n";
        std::cout << std::left << std::setw(8) << "Route" << std::setw(7) << "Mode"
                  << std::setw(13) << "Seats/hour" << std::setw(15) << "Peak hour"
                  << std::setw(16) << "Off-Peak avg" << "Status\n";

        std::string busiestRoute; double busiestLF = -1;
        for (size_t r = 0; r < routes.size(); r++) {
            const Route& rt = routes[r];
            double seatsPerHour = (60.0 / rt.frequency) * rt.capacity;   // one direction

            auto lfAt = [&](int hour) {
                int maxRiders = 0;
                auto it = load[r].find(hour);
                if (it != load[r].end())
                    for (auto& [seg, n] : it->second) maxRiders = std::max(maxRiders, n);
                return 100.0 * maxRiders * SCALE / seatsPerHour;
            };

            double peak = std::max(lfAt(8), lfAt(17));
            double off = 0; int offHours = 0;
            for (int h : {10, 11, 12, 13, 14, 15, 20, 21}) { off += lfAt(h); offHours++; }
            off /= offHours;

            std::string status = peak > 100 ? "OVERCROWDED" : peak > 80 ? "Busy" : peak < 30 ? "Under-used" : "OK";
            std::cout << std::setw(8) << rt.id << std::setw(7) << (rt.mode == BUS ? "Bus" : "Train")
                      << std::setw(13) << (int)seatsPerHour
                      << std::setw(15) << (std::to_string((int)(peak + 0.5)) + "%")
                      << std::setw(16) << (std::to_string((int)(off + 0.5)) + "%") << status << "\n";
            csv << rt.id << "," << (rt.mode == BUS ? "Bus" : "Train") << "," << peak << "," << off << "\n";
            if (peak > busiestLF) { busiestLF = peak; busiestRoute = rt.id; }
        }
        std::cout << "Most crowded route at peak: " << busiestRoute
                  << " (" << (int)(busiestLF + 0.5) << "% full)\n";
    }

    // ---------- 4. Busiest stations ----------
    void printBusiestStations(int top = 5) const {
        std::vector<int> useCount(city.size(), 0);   // boardings + alightings
        for (const auto& p : passengers) {
            if (!p.journey.found) continue;
            for (const auto& leg : p.journey.legs) {
                useCount[leg.stops.front()]++;
                useCount[leg.stops.back()]++;
            }
        }
        std::vector<int> order(city.size());
        for (int i = 0; i < city.size(); i++) order[i] = i;
        std::sort(order.begin(), order.end(), [&](int a, int b) { return useCount[a] > useCount[b]; });

        std::cout << "\n===== BUSIEST STATIONS (boardings + alightings per day) =====\n";
        for (int k = 0; k < top && k < city.size(); k++)
            std::cout << k + 1 << ". " << std::left << std::setw(20) << city.getLocation(order[k]).name
                      << useCount[order[k]] * SCALE << "\n";
    }

    // Average total time (used for the "what if" comparison)
    double averageTime() const {
        Stats s;
        for (const auto& p : passengers) if (p.journey.found) s.add(p.journey);
        return s.avg(s.time);
    }
    double averageWait() const {
        Stats s;
        for (const auto& p : passengers) if (p.journey.found) s.add(p.journey);
        return s.avg(s.wait);
    }
};

#endif