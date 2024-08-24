#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Agregator {
    enum class RequestType : uint8_t {
     sel,
     cnt,
     err
    };

    struct SelRequest {
        int event_id;
        std::string uuid;
        std::string banner_id;
        double price;
    };

    struct CntRequest {
        int event_id;
        std::string sel_request_uuid;
    };

    using SelRequestsStorage = std::unordered_map<std::string, SelRequest>;
    using CntRequestsStorage = std::vector<CntRequest>;

    using Events = std::unordered_set<int>;
    using BannerRequests = std::unordered_map<std::string, Events>;
    using Banners = std::unordered_map<std::string, BannerRequests>;

    using EventsAmount = std::unordered_map<std::string, std::unordered_map<int,int>>;
    using BannersPrices = std::unordered_map<std::string, double>;
}
