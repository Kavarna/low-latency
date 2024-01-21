#pragma once

#include "Types.h"

constexpr u32 ME_MAX_TICKERS = 8;

constexpr u32 ME_MAX_CLIENT_UPDATES = /* 256 */ 1 * 1024;
constexpr u32 ME_MAX_MARKET_UPDATES = /* 256 */ 1 * 1024;

constexpr u32 ME_MAX_NUM_CLIENTS = 256;
constexpr u32 ME_MAX_ORDER_IDS = /* 1024 */ 1 * 1024;
constexpr u32 ME_MAX_PRICE_LEVELS = 256;

constexpr u32 ME_MAX_PENDING_REQUESTS = 1024;
