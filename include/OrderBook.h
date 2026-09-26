#pragma once
#include "Order.h"
#include <map>
#include <list>
#include <unordered_map>
#include <vector>

struct Trade {
    uint64_t makerOrderId;
    uint64_t takerOrderId;
    uint64_t price;
    uint64_t quantity;
};

struct AddOrderResult {
    bool accepted;
    uint64_t remainingQuantity;
    std::vector<Trade> trades;
};

class OrderBook {
    public:
        AddOrderResult addOrder(const Order& order);
        void cancelOrder(uint64_t orderId);
        void printOrderBook() const;

    private:
        std::vector<Trade> match(Order& order);

        std::map<uint64_t, std::list<Order>, std::greater<uint64_t>> bidOrders;
        std::map<uint64_t, std::list<Order>> askOrders;

        struct OrderLocation {
            Side side;
            uint64_t price;
            std::list<Order>::iterator iterator;
        };

        std::unordered_map<uint64_t, OrderLocation> orderMap;
};