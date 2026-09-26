#include "OrderBook.h"
#include <iostream>
#include <algorithm>

std::vector<Trade> OrderBook::match(Order& order) {
    std::vector<Trade> trades;

    if (order.side == Side::BUY) {
        auto it = askOrders.begin();

        while (it != askOrders.end() && order.quantity > 0 && it->first <= order.price) {
            auto& askList = it->second;

            while (!askList.empty() && order.quantity > 0) {
                Order& askOrder = askList.front();

                uint64_t matchedQuantity = std::min(order.quantity, askOrder.quantity);

                trades.push_back({ askOrder.orderId, order.orderId, askOrder.price, matchedQuantity });

                order.quantity -= matchedQuantity;
                askOrder.quantity -= matchedQuantity;

                if (askOrder.quantity == 0) {
                    orderMap.erase(askOrder.orderId);
                    askList.pop_front();
                }
            }

            if (askList.empty()) {
                it = askOrders.erase(it);
            } else {
                ++it;
            }
        }
    }
    else {
        auto it = bidOrders.begin();

        while (it != bidOrders.end() && order.quantity > 0 && it->first >= order.price) {
            auto& bidList = it->second;

            while (!bidList.empty() && order.quantity > 0) {
                Order& bidOrder = bidList.front();

                uint64_t matchedQuantity = std::min(order.quantity, bidOrder.quantity);

                trades.push_back({ bidOrder.orderId, order.orderId, bidOrder.price, matchedQuantity });

                order.quantity -= matchedQuantity;
                bidOrder.quantity -= matchedQuantity;

                if (bidOrder.quantity == 0) {
                    orderMap.erase(bidOrder.orderId);
                    bidList.pop_front();
                }
            }

            if (bidList.empty()) {
                it = bidOrders.erase(it);
            } else {
                ++it;
            }
        }
    }

    return trades;
}

AddOrderResult OrderBook::addOrder(const Order& order) {
    if (order.quantity == 0 || orderMap.find(order.orderId) != orderMap.end()) {
        return { false, order.quantity, {} };
    }

    Order newOrder = order;

    std::vector<Trade> trades = match(newOrder);

    if (newOrder.quantity > 0) {
        if (newOrder.side == Side::BUY) {
            bidOrders[newOrder.price].push_back(newOrder);
            orderMap[newOrder.orderId] = { Side::BUY, newOrder.price, std::prev(bidOrders[newOrder.price].end()) };
        } else {
            askOrders[newOrder.price].push_back(newOrder);
            orderMap[newOrder.orderId] = { Side::SELL, newOrder.price, std::prev(askOrders[newOrder.price].end()) };
        }
    }

    return { true, newOrder.quantity, std::move(trades) };
}

void OrderBook::cancelOrder(uint64_t orderId) {
    auto it = orderMap.find(orderId);

    if (it != orderMap.end()) {
        const OrderLocation& location = it->second;

        if (location.side == Side::BUY) {
            auto& bidList = bidOrders[location.price];
            bidList.erase(location.iterator);

            if (bidList.empty()) {
                bidOrders.erase(location.price);
            }
        } else {
            auto& askList = askOrders[location.price];
            askList.erase(location.iterator);

            if (askList.empty()) {
                askOrders.erase(location.price);
            }
        }
        orderMap.erase(it);
    }
}

void OrderBook::printOrderBook() const {
    std::cout << "Order Book:" << '\n';

    std::cout << "Bids:" << '\n';
    for (const auto& [price, orders] : bidOrders) {
        for (const auto& order : orders) {
            std::cout << "Order ID: " << order.orderId << ", Price: " << order.price << ", Quantity: " << order.quantity << '\n';
        }
    }

    std::cout << "Asks:" << '\n';
    for (const auto& [price, orders] : askOrders) {
        for (const auto& order : orders) {
            std::cout << "Order ID: " << order.orderId << ", Price: " << order.price << ", Quantity: " << order.quantity << '\n';
        }
    }
}