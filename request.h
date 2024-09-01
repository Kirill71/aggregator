#pragma once

#include <unordered_map>
#include <vector>
#include <iostream>
#include <format>
#include <cstdint>

namespace Aggregator
{

enum class RequestType : uint8_t
{
    sel,
    cnt,
    err
};

struct SelRequest final
{
    int event_id{};
    std::string uuid{};
    std::string banner_id{};
    double price{};
};

struct CntRequest final
{
    int event_id{};
    std::string sel_request_uuid{};
};

struct Banner final
{
    std::string banner_id{};
    double price{};
    std::unordered_map<int, int> events{};
};

using SelRequestsStorage = std::unordered_map<std::string, SelRequest>;
using CntRequestsStorage = std::vector<CntRequest>;
using Banners = std::unordered_map<std::string, Banner>;

inline std::ostream& operator<<(std::ostream& stream, const Banner& banner)
{
    const auto& banner_id = banner.banner_id;
    const auto& price = banner.price;
    const auto& events = banner.events;
    stream << std::format(R"(    <Banner id="{}" revenues="{:1.3f}">)", banner_id, price) << std::endl;
    stream << "        <Events>" << std::endl;
    for (const auto& [event_id, freq]: events)
    {
        stream << std::format(R"(            <Event id="{}">{}</Event>)", event_id, freq) << std::endl;
    }
    stream << "        </Events>" << std::endl;
    stream << "    </Banner>" << std::endl;
    return stream;
}

}
