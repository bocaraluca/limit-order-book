#include "OrderBook.h"
#include <cassert>

void test_price_time_priority() {
    OrderBook book;
    book.addOrder({ 1, 101, 5, Side::SELL });
    book.addOrder({ 2, 100, 3, Side::SELL });
    book.addOrder({ 3, 100, 4, Side::SELL });

    AddOrderResult result = book.addOrder({ 10, 101, 10, Side::BUY });

    assert(result.accepted);
    assert(result.remainingQuantity == 0);
    assert(result.trades.size() == 3);
    assert(result.trades[0].makerOrderId == 2);
    assert(result.trades[0].price == 100);
    assert(result.trades[0].quantity == 3);
    assert(result.trades[1].makerOrderId == 3);
    assert(result.trades[1].quantity == 4);
    assert(result.trades[2].makerOrderId == 1);
    assert(result.trades[2].price == 101);
    assert(result.trades[2].quantity == 3);
}

void test_partial_fill_and_cancel() {
    OrderBook book;
    book.addOrder({ 20, 99, 10, Side::BUY });

    AddOrderResult fill = book.addOrder({ 21, 99, 4, Side::SELL });
    assert(fill.trades.size() == 1);
    assert(fill.trades[0].makerOrderId == 20);
    assert(fill.trades[0].quantity == 4);

    book.cancelOrder(20);
    AddOrderResult afterCancel = book.addOrder({ 22, 99, 1, Side::SELL });
    assert(afterCancel.trades.empty());
}

void test_invalid_and_duplicate_orders() {
    OrderBook book;
    AddOrderResult first = book.addOrder({ 30, 100, 5, Side::BUY });
    AddOrderResult duplicate = book.addOrder({ 30, 99, 5, Side::SELL });
    AddOrderResult empty = book.addOrder({ 31, 100, 0, Side::BUY });

    assert(first.accepted);
    assert(!duplicate.accepted);
    assert(!empty.accepted);

    AddOrderResult matchingOrder = book.addOrder({ 32, 100, 5, Side::SELL });
    assert(matchingOrder.trades.size() == 1);
    assert(matchingOrder.trades[0].makerOrderId == 30);
}

void test_non_crossing_orders_rest() {
    OrderBook book;
    AddOrderResult bid = book.addOrder({ 40, 100, 2, Side::BUY });
    AddOrderResult ask = book.addOrder({ 41, 101, 2, Side::SELL });

    assert(bid.accepted && bid.trades.empty());
    assert(ask.accepted && ask.trades.empty());

    AddOrderResult crossingBid = book.addOrder({ 42, 101, 1, Side::BUY });
    assert(crossingBid.trades.size() == 1);
    assert(crossingBid.trades[0].makerOrderId == 41);
    assert(crossingBid.trades[0].price == 101);
}

int main() {
    test_price_time_priority();
    test_partial_fill_and_cancel();
    test_invalid_and_duplicate_orders();
    test_non_crossing_orders_rest();
    return 0;
}