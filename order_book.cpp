#include <string.h>

#include "engine.h"
#include "limits.h"
#include "order_book.h"
#include "constants.h"

namespace OB {

namespace {

/* Report trade execution */
void executeTrade(const Field& symbol, const Field& buyTrader,
                  const Field& sellTrader, t_price tradePrice,
                  t_size tradeSize) {
  t_execution exec;

  if (tradeSize == 0) /* Skip orders that have been cancelled */
    return;

  exec.symbol = symbol;

  exec.price = tradePrice;
  exec.size = tradeSize;

  exec.side = 0;
  exec.trader = buyTrader;
  execution(exec); /* Report the buy-side trade */

  exec.side = 1;
  exec.trader = sellTrader;
  execution(exec); /* Report the sell-side trade */
}
}

OrderBook& OrderBook::get() {
  static OrderBook ob;
  return ob;
}

void OrderBook::initialize() {
  /* Initialize the price point array */
  pricePoints.resize(kMaxPrice);
  for (auto& el : pricePoints) {
    el.clear();
  }

  arenaBookEntries.resize(kMaxNumOrders);

  hotPathVars.curOrderID = 0;
  hotPathVars.askMin = kMaxPrice;
  hotPathVars.bidMax = kMinPrice;
}

void OrderBook::shutdown() {}

t_orderid OrderBook::limit(t_order& order) {
  t_price price = order.price;
  t_size orderSize = order.size;

  if (__builtin_expect(order.side == 0, 1)) {/* Buy order */
    /* Look for outstanding sell orders that cross with the incoming order */
    if (price >= hotPathVars.askMin) {
      auto ppEntry = pricePoints.begin() + hotPathVars.askMin;
      do {
        auto bookEntry = ppEntry->begin();
        while (bookEntry != ppEntry->end()) {
          if (__builtin_expect(bookEntry->size < orderSize, 1)) {
            executeTrade(order.symbol, order.trader, bookEntry->trader, price,
                         bookEntry->size);
            orderSize -= bookEntry->size;
            ++bookEntry;
          } else {
            executeTrade(order.symbol, order.trader, bookEntry->trader, price,
                         orderSize);
            if (bookEntry->size > orderSize)
              bookEntry->size -= orderSize;
            else
              ++bookEntry;

            ppEntry->erase(ppEntry->begin(), bookEntry);
            while (ppEntry->begin() != bookEntry) {
              ppEntry->pop_front();
            }
            return ++hotPathVars.curOrderID;
          }
        }
        /* We have exhausted all orders at the askMin price point. Move on to
           the next price level. */
        ppEntry->clear();
        ppEntry++;
        hotPathVars.askMin++;
      } while (price >= hotPathVars.askMin);
    }

    auto entry = arenaBookEntries.begin() + (++hotPathVars.curOrderID);
    entry->size = orderSize;
    entry->trader = order.trader;
    pricePoints[price].push_back(*entry);
    if (hotPathVars.bidMax < price) hotPathVars.bidMax = price;
    return hotPathVars.curOrderID;

  } else {/* Sell order */
    /* Look for outstanding Buy orders that cross with the incoming order */
    if (price <= hotPathVars.bidMax) {
      auto ppEntry = pricePoints.begin() + hotPathVars.bidMax;
      do {
        auto bookEntry = ppEntry->begin();
        while (bookEntry != ppEntry->end()) {
          if (__builtin_expect(bookEntry->size < orderSize, 1)) {
            executeTrade(order.symbol, bookEntry->trader, order.trader, price,
                         bookEntry->size);
            orderSize -= bookEntry->size;
            ++bookEntry;
          } else {
            executeTrade(order.symbol, bookEntry->trader, order.trader, price,
                         orderSize);
            if (bookEntry->size > orderSize)
              bookEntry->size -= orderSize;
            else
              ++bookEntry;

            while (ppEntry->begin() != bookEntry) {
              ppEntry->pop_front();
            }
            return ++hotPathVars.curOrderID;
          }
        }

        /* We have exhausted all orders at the bidMax price point. Move on to
           the next price level. */
        ppEntry->clear();
        ppEntry--;
        hotPathVars.bidMax--;
      } while (price <= hotPathVars.bidMax);
    }

    auto entry = arenaBookEntries.begin() + (++hotPathVars.curOrderID);
    entry->size = orderSize;
    entry->trader = order.trader;
    pricePoints[price].push_back(*entry);
    if (hotPathVars.askMin > price) hotPathVars.askMin = price;
    return hotPathVars.curOrderID;
  }
}

void OrderBook::cancel(t_orderid orderid) {
  arenaBookEntries[orderid].size = 0;
}
}
