#pragma once

#include <span>

#include "request.h"

namespace Aggregator {

RequestType get_request_type(std::span<std::string> request);
std::vector<std::string> tokenize(std::string_view line);
SelRequest create_sel_request(std::span<std::string> request);
CntRequest create_cnt_request(std::span<std::string> request);
BannersPrices process_banners_prices(const SelRequestsStorage& sel_storage, const Banners& banners);
EventsAmount process_events_amount(const SelRequestsStorage& sel_storage, const CntRequestsStorage& cnt_storage);
Banners process_banners(const SelRequestsStorage& sel_storage, const CntRequestsStorage& cnt_storage);
std::string serialize(const Banners& banners, const BannersPrices& banners_prices, const EventsAmount& events_amount);


}
