// main.cpp - Final assignment code
// compile with g++ -std=c++17 main.cpp -o transport
#include "Profiler.h"   // includes Passenger.h, PathFinder.h, Route.h, CityGraph.h

int main() {
    CityGraph city;

    // adding all the locations in the city (name, bus, train)
    int central    = city.addLocation("Central Hub",       true, true);
    int northRes   = city.addLocation("North Residential", true, true);
    int southRes   = city.addLocation("South Residential", true, true);
    int eastRes    = city.addLocation("East Residential",  true, false);
    int westRes    = city.addLocation("West Residential",  true, false);
    int university = city.addLocation("University",        true, true);
    int hospital   = city.addLocation("Hospital",          true, false);
    int industrial = city.addLocation("Industrial Zone",   true, true);
    int airport    = city.addLocation("Airport",           true, true);
    int mall       = city.addLocation("Shopping Mall",     true, false);
    int stadium    = city.addLocation("Stadium",           true, false);
    int techPark   = city.addLocation("Tech Park",         true, true);

    // bus roads and distances
    // cout << "adding roads..." << endl;
    city.addRoad(central, northRes, 5);
    city.addRoad(central, southRes, 6);
    city.addRoad(central, eastRes, 4);
    city.addRoad(central, westRes, 4);
    city.addRoad(central, mall, 2);
    city.addRoad(central, hospital, 3);
    city.addRoad(northRes, university, 3);
    city.addRoad(northRes, westRes, 5);
    city.addRoad(southRes, industrial, 4);
    city.addRoad(southRes, stadium, 3);
    city.addRoad(southRes, eastRes, 6);
    city.addRoad(eastRes, techPark, 3);
    city.addRoad(eastRes, mall, 3);
    city.addRoad(westRes, hospital, 4);
    city.addRoad(university, techPark, 4);
    city.addRoad(hospital, stadium, 5);
    city.addRoad(industrial, airport, 6);
    city.addRoad(airport, techPark, 7);
    city.addRoad(mall, stadium, 4);

    // train tracks
    // Red Line: North -> Central -> South -> Industrial -> Airport
    city.addRail(northRes, central, 5);
    city.addRail(central, southRes, 6);
    city.addRail(southRes, industrial, 4);
    city.addRail(industrial, airport, 6);
    // Blue Line: University - Central - Tech Park - Airport
    city.addRail(university, central, 6);
    city.addRail(central, techPark, 5);
    city.addRail(techPark, airport, 7);

    // part 1: Bus routes
    // assuming 30 km/h speed and 60 pax capacity
    // Route{ id, mode, {stops...}, frequency(min), capacity, speed(km/h) }
    TransportNetwork network(city);
    network.addRoute({"101", BUS, {northRes, central, mall, eastRes, techPark},       10, 60, 30});
    network.addRoute({"102", BUS, {westRes, hospital, central, southRes, industrial}, 10, 60, 30});
    network.addRoute({"103", BUS, {university, northRes, westRes, hospital, stadium}, 15, 60, 30});
    network.addRoute({"104", BUS, {airport, industrial, southRes, stadium, mall, central}, 20, 60, 30});
    network.addRoute({"105", BUS, {university, techPark, airport},                    15, 60, 30});

    // part 2: Train routes
    // fast trains (60km/h), holds 500 ppl
    network.addRoute({"RED",  TRAIN, {northRes, central, southRes, industrial, airport}, 15, 500, 60});
    network.addRoute({"BLUE", TRAIN, {university, central, techPark, airport},           20, 500, 60});

    // print out the network info
    std::cout << "********** NOVA CITY - SMART PUBLIC TRANSPORT **********\n";
    city.printLocations();
    city.printGraph();
    network.printRoutes(BUS);
    network.printSummary(BUS);
    network.printRoutes(TRAIN);
    network.printSummary(TRAIN);
    network.printTransferPoints();

    // part 3: passenger demand sim
    PathFinder finder(city, network);
    DemandGenerator demGen(
        {northRes, southRes, eastRes, westRes},                // residential areas
        {central, industrial, techPark, university, hospital}, // work / study places
        city.size(),
        42);                                                   // random seed (same result every run)

    std::vector<Passenger> passList = demGen.generateDay();
    for (auto& p : passList)
        p.journey = finder.findPath(p.origin, p.destination, p.departTime);  // Dijkstra for every passenger

    // Full log of every passenger -> text file
    std::ofstream log("passenger_log.txt");
    for (const auto& p : passList) log << journeyLine(p, city, network) << "\n";
    log.close();

    // Console: first 3 passengers of every hour
    std::cout << "\n===== PASSENGER MOVEMENTS (first 3 of each hour) =====\n";
    int lastHour = -1, shown = 0;
    // std::cout << "printing passenger logs..." << "\n";
    for (const auto& p : passList) {
        int hour = p.departTime / 60;
        if (hour != lastHour) {
            lastHour = hour; shown = 0;
            std::cout << "\n--- " << toClock(hour * 60) << " - " << toClock(hour * 60 + 59)
                      << "  (" << DemandGenerator::periodName(hour) << ", "
                      << DemandGenerator::passengersInHour(hour) << " passengers) ---\n";
        }
        if (shown++ < 3) std::cout << journeyLine(p, city, network) << "\n";
    }
    std::cout << "\nTotal passengers simulated: " << passList.size()
              << "  (full list saved to passenger_log.txt)\n";

    // part 4: efficiency profiling
    Profiler profiler(city, network, passList);
    std::cout << "\n\n********** PART IV: EFFICIENCY PROFILING **********\n";
    profiler.printPeriodSummary();
    profiler.printHourly();
    profiler.printRouteLoad();
    profiler.printBusiestStations();

    // what-if experiment for the report
    // Buses 101 and 102 run every 5 min instead of 10, RED train every 10 min instead of 15.
    TransportNetwork improved = network;          // copy of the network
    improved.setFrequency("101", 5);
    improved.setFrequency("102", 5);
    improved.setFrequency("RED", 10);

    PathFinder finder2(city, improved);
    std::vector<Passenger> passList2 = passList;   // copy passengers
    for (auto& p : passList2) p.journey = finder2.findPath(p.origin, p.destination, p.departTime);
    Profiler profiler2(city, improved, passList2);

    std::cout << "\n===== WHAT IF: more frequent 101, 102 (every 5 min) and RED (every 10 min) =====\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << std::left << std::setw(20) << "" << std::setw(12) << "Current" << "Improved\n";
    std::cout << std::setw(20) << "Avg travel time" << std::setw(12) << profiler.averageTime()
              << profiler2.averageTime() << " min\n";
    std::cout << std::setw(20) << "Avg waiting time" << std::setw(12) << profiler.averageWait()
              << profiler2.averageWait() << " min\n";
    profiler2.printRouteLoad("route_load_improved.csv");

    std::cout << "\nCSV files written: hourly_stats.csv, route_load.csv, route_load_improved.csv (open in Excel for charts)\n";
    return 0;
}