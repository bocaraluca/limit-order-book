#include "OrderBook.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using Nanoseconds = std::chrono::nanoseconds;

constexpr int Repetitions = 5;
constexpr int PassiveOrders = 100'000;
constexpr int CancellationOrders = 10'000;
constexpr int MatchingSamples = 1'000;

struct LatencyStats {
    long long p50;
    long long p99;
    long long p999;
};

LatencyStats calculatePercentiles(std::vector<long long> samples) {
    std::sort(samples.begin(), samples.end());

    const auto percentile = [&samples](double probability) {
        const auto index = static_cast<std::size_t>(
            probability * static_cast<double>(samples.size() - 1));
        return samples[index];
    };

    return {
        percentile(0.50),
        percentile(0.99),
        percentile(0.999)
    };
}

long long elapsedNanoseconds(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration_cast<Nanoseconds>(end - start).count();
}

void printLatencyRow(
    const std::string& operation,
    std::size_t operations,
    long long totalNanoseconds,
    const LatencyStats& stats) {
    const double seconds = static_cast<double>(totalNanoseconds) / 1'000'000'000.0;
    const double throughput = static_cast<double>(operations) / seconds;

    std::cout << std::left << std::setw(18) << operation
              << std::right << std::setw(12) << operations
              << std::setw(16) << std::fixed << std::setprecision(0) << throughput
              << std::setw(12) << stats.p50
              << std::setw(12) << stats.p99
              << std::setw(12) << stats.p999 << '\n';
}

void printEnvironment() {
    std::cout << "Build: Release (benchmark should be compiled with optimization enabled)\n";
    std::cout << "Compiler: ";
#if defined(_MSC_VER)
    std::cout << "MSVC " << (_MSC_VER / 100) << "." << (_MSC_VER % 100)
              << " (" << _MSC_FULL_VER << ")";
#elif defined(__clang__)
    std::cout << "Clang " << __clang_major__ << "." << __clang_minor__;
#elif defined(__GNUC__)
    std::cout << "GCC " << __GNUC__ << "." << __GNUC_MINOR__;
#else
    std::cout << "unknown";
#endif
    std::cout << '\n';
    std::cout << "Platform: ";
#if defined(_WIN32)
    std::cout << "Windows";
#elif defined(__linux__)
    std::cout << "Linux";
#elif defined(__APPLE__)
    std::cout << "macOS";
#else
    std::cout << "unknown";
#endif
    std::cout << ", " << (sizeof(void*) * 8) << "-bit\n";
    std::cout << "Logical processors: " << std::thread::hardware_concurrency() << "\n\n";
}

std::vector<long long> benchmarkInsertions(long long& totalNanoseconds) {
    std::vector<long long> samples;
    samples.reserve(static_cast<std::size_t>(Repetitions) * PassiveOrders);
    totalNanoseconds = 0;

    for (int repetition = 0; repetition < Repetitions; ++repetition) {
        OrderBook book;
        const auto startTotal = Clock::now();

        for (int i = 0; i < PassiveOrders; ++i) {
            const Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
            const uint64_t price = 1000 + (i % 50) * (side == Side::BUY ? -1 : 1);
            const Order order{static_cast<uint64_t>(repetition) * PassiveOrders + i,
                              price, 100, side};

            const auto start = Clock::now();
            const AddOrderResult result = book.addOrder(order);
            const auto end = Clock::now();

            if (!result.accepted) {
                std::cerr << "Insertion benchmark received a rejected order.\n";
                return {};
            }
            samples.push_back(elapsedNanoseconds(start, end));
        }

        totalNanoseconds += elapsedNanoseconds(startTotal, Clock::now());
    }

    return samples;
}

std::vector<long long> benchmarkMatching(long long& totalNanoseconds) {
    std::vector<long long> samples;
    samples.reserve(static_cast<std::size_t>(Repetitions) * MatchingSamples);
    totalNanoseconds = 0;

    for (int repetition = 0; repetition < Repetitions; ++repetition) {
        std::vector<OrderBook> books;
        books.reserve(MatchingSamples);
        for (int sample = 0; sample < MatchingSamples; ++sample) {
            books.emplace_back();
            OrderBook& book = books.back();
            for (int i = 0; i < 100; ++i) {
                book.addOrder({static_cast<uint64_t>(i), 1000, 100, Side::BUY});
            }
        }

        const auto startBatch = Clock::now();
        for (OrderBook& book : books) {
            const auto start = Clock::now();
            const AddOrderResult result =
                book.addOrder({101, 1000, 5'000, Side::SELL});
            const auto end = Clock::now();

            if (!result.accepted || result.trades.size() != 50 ||
                result.remainingQuantity != 0) {
                std::cerr << "Matching benchmark produced an unexpected result.\n";
                return {};
            }
            samples.push_back(elapsedNanoseconds(start, end));
        }
        totalNanoseconds += elapsedNanoseconds(startBatch, Clock::now());
    }

    return samples;
}

std::vector<long long> benchmarkCancellations(long long& totalNanoseconds) {
    std::vector<long long> samples;
    samples.reserve(static_cast<std::size_t>(Repetitions) * CancellationOrders);
    totalNanoseconds = 0;

    for (int repetition = 0; repetition < Repetitions; ++repetition) {
        OrderBook book;
        for (int i = 0; i < CancellationOrders; ++i) {
            book.addOrder({static_cast<uint64_t>(i), 1000, 100, Side::BUY});
        }

        const auto startTotal = Clock::now();
        for (int i = 0; i < CancellationOrders; ++i) {
            const auto start = Clock::now();
            book.cancelOrder(static_cast<uint64_t>(i));
            const auto end = Clock::now();
            samples.push_back(elapsedNanoseconds(start, end));
        }
        totalNanoseconds += elapsedNanoseconds(startTotal, Clock::now());
    }

    return samples;
}

} // namespace

int main() {
    std::cout << "Limit order book benchmark\n"
              << "Repetitions: " << Repetitions << "\n";
    printEnvironment();

    long long insertionTotal = 0;
    long long matchingTotal = 0;
    long long cancellationTotal = 0;

    const auto insertionSamples = benchmarkInsertions(insertionTotal);
    const auto matchingSamples = benchmarkMatching(matchingTotal);
    const auto cancellationSamples = benchmarkCancellations(cancellationTotal);

    if (insertionSamples.empty() || matchingSamples.empty() ||
        cancellationSamples.empty()) {
        return 1;
    }

    std::cout << std::left << std::setw(18) << "Operation"
              << std::right << std::setw(12) << "Operations"
              << std::setw(16) << "Ops/sec"
              << std::setw(12) << "p50 (ns)"
              << std::setw(12) << "p99 (ns)"
              << std::setw(12) << "p99.9 (ns)" << '\n';
    std::cout << std::string(82, '-') << '\n';

    printLatencyRow("Insertion", insertionSamples.size(), insertionTotal,
                    calculatePercentiles(insertionSamples));
    printLatencyRow("Matching", matchingSamples.size(), matchingTotal,
                    calculatePercentiles(matchingSamples));
    printLatencyRow("Cancellation", cancellationSamples.size(), cancellationTotal,
                    calculatePercentiles(cancellationSamples));

    return 0;
}
