#include "OrderBook.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>

int main() {
    OrderBook ob;
    const int NUM_ORDERS = 100000;
    std::vector<long long> latencies;
    latencies.reserve(NUM_ORDERS);

    std::cout << "Generating and processing " << NUM_ORDERS << " passive orders...\n";

    // 1. Order insertion benchmark
    auto start_total = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_ORDERS; ++i) {
        Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
        uint64_t price = 1000 + (i % 50) * (side == Side::BUY ? -1 : 1); 
        
        Order order = { static_cast<uint64_t>(i), price, 100, side };

        auto start_op = std::chrono::high_resolution_clock::now();
        ob.addOrder(order);
        auto end_op = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_op - start_op).count();
        latencies.push_back(duration);
    }

    auto end_total = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_total - start_total).count();

    long long sum_latency = std::accumulate(latencies.begin(), latencies.end(), 0LL);
    long long avg_latency = sum_latency / NUM_ORDERS;

    std::cout << "Total insertion time: " << total_duration << " ms\n";
    std::cout << "Average latency per order: " << avg_latency << " ns\n";

    // 2. Aggressive execution (Sweep) benchmark
    std::cout << "\nSimulating an aggressive market sweep...\n";
    Order massiveSell = { NUM_ORDERS + 1, 950, 50000, Side::SELL };
    
    auto start_match = std::chrono::high_resolution_clock::now();
    ob.addOrder(massiveSell);
    auto end_match = std::chrono::high_resolution_clock::now();
    
    auto match_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_match - start_match).count();
    std::cout << "Massive order execution time: " << match_duration << " us\n";

    // 3. O(1) cancellation benchmark
    std::cout << "\nCancelling 10,000 active orders...\n";
    auto start_cancel = std::chrono::high_resolution_clock::now();
    
    for (int i = 1; i < 20000; i += 2) {
        ob.cancelOrder(i);
    }
    
    auto end_cancel = std::chrono::high_resolution_clock::now();
    auto cancel_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_cancel - start_cancel).count();
    
    std::cout << "Total cancellation time for 10,000 orders: " << cancel_duration << " us\n";

    return 0;
}