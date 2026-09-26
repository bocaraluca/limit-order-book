#pragma once
#include <cstdint>

enum class Side {
    BUY, 
    SELL
};

struct Order {
    uint64_t orderId;
    uint64_t price;
    uint64_t quantity;
    Side side;
};