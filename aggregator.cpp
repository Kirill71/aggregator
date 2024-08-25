#include "aggregator.h"

#include <sstream>
#include <ranges>

namespace Aggregator
{

RequestType get_request_type(const std::span<std::string> request)
{
    static constexpr auto sel_type = "sel";
    static constexpr auto cnt_type = "cnt";

    const auto& request_type = request[0];

    if (sel_type == request_type)
    {
        return RequestType::sel;
    }

    if (cnt_type == request_type)
    {
        return RequestType::cnt;
    }

    return RequestType::err;
}

std::vector<std::string> tokenize(const std::string_view line)
{
    std::stringstream stream((line.data()));
    std::string token;

    static constexpr char request_type_separator = ':';
    static constexpr char separator = ',';

    std::vector<std::string> tokens{};
    std::getline(stream, token, request_type_separator);
    tokens.emplace_back(token);

    while (std::getline(stream, token, separator))
    {
        tokens.emplace_back(token);
    }

    return tokens;
}

SelRequest create_sel_request(const std::span<std::string> request)
{
    static constexpr int sel_request_size = 5;
    if (request.size() != sel_request_size)
    {
        throw std::invalid_argument("Invalid sel request");
    }

    SelRequest sel_request{};
    // request[0] is ignored here because it's a request type field, and it checked in another place.
    sel_request.event_id = std::stoi(request[1]);
    sel_request.uuid = request[2];
    sel_request.banner_id = request[3];
    sel_request.price = std::stod(request[4]);

    return sel_request;
}

CntRequest create_cnt_request(const std::span<std::string> request)
{
    static constexpr int cnt_request_size = 3;
    if (request.size() != cnt_request_size)
    {
        throw std::invalid_argument("Invalid cnt request");
    }

    CntRequest cnt_request{};

    cnt_request.event_id = std::stoi(request[1]);
    cnt_request.sel_request_uuid = request[2];

    return cnt_request;
}

Banners process_banners(const SelRequestsStorage& sel_storage, const CntRequestsStorage& cnt_storage)
{
    Banners banners;
    const auto is_displayed = [](const int event_id)
    {
        static constexpr int displayed_event_id = 1;
        return event_id == displayed_event_id;
    };
    for (const auto& [event_id, sel_request_uuid]: cnt_storage)
    {
        if (auto it = sel_storage.find(sel_request_uuid); it != sel_storage.end())
        {
            const auto& [sel_event_id, _, banner_id, price] = it->second;
            if (auto banner_it = banners.find(banner_id); banner_it == banners.end())
            {
               Banner new_banner{};
               new_banner.banner_id = banner_id;
               new_banner.price += is_displayed(event_id) ? price : 0.0;
               ++new_banner.events[event_id];
               banners.emplace(banner_id, std::move(new_banner));
            }
            else
            {
                auto& [_, current_price, events] = banner_it->second;
                current_price += is_displayed(event_id) ? price : 0.0;
                ++events[event_id];
            }
        }
    }
    for (const auto& sel_request : sel_storage | std::views::values)
    {
        ++banners[sel_request.banner_id].events[sel_request.event_id];
    }
    return banners;
}

std::string serialize(const Banners& banners)
{
    std::stringstream stream{};
    stream << "<Banners>" << std::endl;
    for (const auto& banner: banners | std::views::values)
    {
        stream << banner;
    }
    stream << "</Banners>"<< std::endl;
    return stream.str();
}

}
