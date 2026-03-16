#include <string.h>

#include "engine.h"
#include "limits.h"
#include "order_book.h"
#include "constants.h"

namespace OB {

namespace {

/* Note: executeTrade has been inlined into the matching loops for better performance */
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

t_orderid OrderBook::limit(t_order order) {
  t_price price = order.price;
  t_size orderSize = order.size;
  Field orderSymbol = order.symbol;  // Cache symbol to avoid repeated member access
  PricePoint* pricePointsPtr = pricePoints.data();  // Raw pointer for faster access

  if (__builtin_expect(order.side == 0, 1)) {/* Buy order */
    /* Look for outstanding sell orders that cross with the incoming order */
    if (price >= hotPathVars.askMin) {
      auto ppEntry = pricePointsPtr + hotPathVars.askMin;
      do {
        auto bookEntry = ppEntry->begin();
        while (bookEntry != ppEntry->end()) {
          t_size entrySize = bookEntry->size;
          if (__builtin_expect(entrySize == 0, 0)) {
            ++bookEntry;
          } else if (__builtin_expect(entrySize < orderSize, 1)) {
            // Inline executeTrade for buy order
            {
              t_execution exec;
              exec.symbol = orderSymbol;
              exec.price = price;
              exec.size = entrySize;
              exec.side = 0;
              exec.trader = order.trader;
              execution(exec);
              exec.side = 1;
              exec.trader = bookEntry->trader;
              execution(exec);
            }
            orderSize -= entrySize;
            ++bookEntry;
          } else {
            // Inline executeTrade for buy order (entrySize >= orderSize)
            {
              t_execution exec;
              exec.symbol = orderSymbol;
              exec.price = price;
              exec.size = orderSize;
              exec.side = 0;
              exec.trader = order.trader;
              execution(exec);
              exec.side = 1;
              exec.trader = bookEntry->trader;
              execution(exec);
            }
            if (entrySize > orderSize)
              bookEntry->size = entrySize - orderSize;
            else
              ++bookEntry;

            // Optimized pop_front loop - remove processed entries
            auto it = ppEntry->begin();
            while (it != bookEntry) {
              ppEntry->erase_after(ppEntry->before_begin());
              it = ppEntry->begin();
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
    pricePointsPtr[price].push_back(*entry);
    if (hotPathVars.bidMax < price) hotPathVars.bidMax = price;
    return hotPathVars.curOrderID;

  } else {/* Sell order */
    /* Look for outstanding Buy orders that cross with the incoming order */
    if (price <= hotPathVars.bidMax) {
      auto ppEntry = pricePointsPtr + hotPathVars.bidMax;
      do {
        auto bookEntry = ppEntry->begin();
        while (bookEntry != ppEntry->end()) {
          t_size entrySize = bookEntry->size;
          if (__builtin_expect(entrySize == 0, 0)) {
            ++bookEntry;
          } else if (__builtin_expect(entrySize < orderSize, 1)) {
            // Inline executeTrade for sell order
            {
              t_execution exec;
              exec.symbol = orderSymbol;
              exec.price = price;
              exec.size = entrySize;
              exec.side = 0;
              exec.trader = bookEntry->trader;
              execution(exec);
              exec.side = 1;
              exec.trader = order.trader;
              execution(exec);
            }
            orderSize -= entrySize;
            ++bookEntry;
          } else {
            // Inline executeTrade for sell order (entrySize >= orderSize)
            {
              t_execution exec;
              exec.symbol = orderSymbol;
              exec.price = price;
              exec.size = orderSize;
              exec.side = 0;
              exec.trader = bookEntry->trader;
              execution(exec);
              exec.side = 1;
              exec.trader = order.trader;
              execution(exec);
            }
            if (entrySize > orderSize)
              bookEntry->size = entrySize - orderSize;
            else
              ++bookEntry;

            // Optimized pop_front loop - remove processed entries
            auto it = ppEntry->begin();
            while (it != bookEntry) {
              ppEntry->erase_after(ppEntry->before_begin());
              it = ppEntry->begin();
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
    pricePointsPtr[price].push_back(*entry);
    if (hotPathVars.askMin > price) hotPathVars.askMin = price;
    return hotPathVars.curOrderID;
  }
}

void OrderBook::cancel(t_orderid orderid) {
  arenaBookEntries[orderid].size = 0;
}
}
