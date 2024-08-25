#pragma once

#include <span>

#include "request.h"

namespace Aggregator
{

RequestType get_request_type(std::span<std::string> request);

std::vector<std::string> tokenize(std::string_view line);

SelRequest create_sel_request(std::span<std::string> request);

CntRequest create_cnt_request(std::span<std::string> request);

Banners process_banners(const SelRequestsStorage& sel_storage, const CntRequestsStorage& cnt_storage);

std::string serialize(const Banners& banners);

}
