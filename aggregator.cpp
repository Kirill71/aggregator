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

BannersPrices process_banners_prices(const SelRequestsStorage& sel_storage, const Banners& banners)
{
    static constexpr int displayed_event = 1;
    BannersPrices banners_prices;
    for (const auto& [banner_id, requests_to_events]: banners)
    {
        double banner_calculated_price = 0.0;
        for (const auto& [request_uuid, events]: requests_to_events)
        {
            if (events.contains(displayed_event))
            {
                if (auto it_sel = sel_storage.find(request_uuid); it_sel != sel_storage.end())
                {
                    banner_calculated_price += it_sel->second.price;
                }
            }
        }
        banners_prices[banner_id] = banner_calculated_price;
    }

    return banners_prices;
}

EventsAmount process_events_amount(const SelRequestsStorage& sel_storage, const CntRequestsStorage& cnt_storage)
{
    EventsAmount events_amount;
    for (const auto& sel_request: sel_storage | std::views::values)
    {
        ++events_amount[sel_request.banner_id][sel_request.event_id];
    }

    for (const auto& [event_id, sel_request_uuid]: cnt_storage)
    {
        if (auto it = sel_storage.find(sel_request_uuid); it != sel_storage.cend())
        {
            ++events_amount[it->second.banner_id][event_id];
        }
    }

    return events_amount;
}

Banners process_banners(const SelRequestsStorage& sel_storage, const CntRequestsStorage& cnt_storage)
{
    Banners banners;
    for (const auto& [event_id, sel_request_uuid]: cnt_storage)
    {
        if (auto it = sel_storage.find(sel_request_uuid); it != sel_storage.end())
        {
            const auto& sel_request = it->second;
            if (auto it_banners = banners.find(sel_request.banner_id); it_banners == banners.end())
            {
                BannerRequests banner_requests{};
                Events events{};
                events.emplace(sel_request.event_id);
                events.emplace(event_id);
                banner_requests.emplace(sel_request_uuid, std::move(events));
                banners.emplace(sel_request.banner_id, std::move(banner_requests));
            }
            else
            {
                auto& banner_requests = it_banners->second;
                if (auto it_req = banner_requests.find(sel_request_uuid); it_req != banner_requests.end())
                {
                    auto& events = it_req->second;
                    events.emplace(event_id);
                }
                else
                {
                    banner_requests[sel_request_uuid].emplace(event_id);
                }
            }
        }
    }
    return banners;
}

std::string serialize(const Banners& banners, const BannersPrices& banners_prices, const EventsAmount& events_amount)
{
    std::stringstream stream{};
    stream << "<Banners>\n";
    for (const auto& banner_id: banners | std::views::keys)
    {
        stream << "    <Banner id=\"" << banner_id << "\"" << " revenues=\"" << banners_prices.at(banner_id) << "\"" <<
                ">\n";
        stream << "        <Events>\n";
        const auto& banner_events = events_amount.at(banner_id);
        for (const auto& event_id: banner_events | std::views::keys)
        {
            stream << "            <Event id=\"" << event_id << "\">" << banner_events.at(event_id) << "</Event>\n";
        }
        stream << "        </Events>\n";
        stream << "    </Banner>\n";
    }
    stream << "</Banners>";
    return stream.str();
}

}
