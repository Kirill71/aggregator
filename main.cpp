#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <ranges>

#include "request.h"

Agregator::RequestType get_request_type(const std::span<std::string> request) {
   const std::string sel_type = "sel";
   const std::string cnt_type = "cnt";
   const auto& request_type = request[0];
   if (sel_type == request_type) {
      return Agregator::RequestType::sel;
   }
   if (cnt_type == request_type) {
      return Agregator::RequestType::cnt;
   }
   return Agregator::RequestType::err;
}

auto process_banners_prices(const Agregator::SelRequestsStorage& sel_storage, const Agregator::Banners& banners) {

   static constexpr int displayed_event = 1;
   Agregator::BannersPrices banners_prices;
   for (const auto& [banner_id, requests_to_events]: banners) {
      double banner_calculated_price = 0.0;
      for (const auto& [request_uuid, events] : requests_to_events) {
         if (events.contains(displayed_event)) {
            if (auto it_sel = sel_storage.find(request_uuid); it_sel != sel_storage.end()) {
               banner_calculated_price += it_sel->second.price;
            }
         }
      }
      banners_prices[banner_id] = banner_calculated_price;
   }
   return banners_prices;
}

auto process_events_amount(const Agregator::SelRequestsStorage& sel_storage, const Agregator::CntRequestsStorage& cnt_storage) {

   Agregator::EventsAmount sel_freq;
   for (const auto& sel_request : sel_storage | std::views::values) {
      ++sel_freq[sel_request.banner_id][sel_request.event_id];
   }

   for (const auto&[event_id, sel_request_uuid] : cnt_storage) {
      if (auto it = sel_storage.find(sel_request_uuid); it != sel_storage.cend()) {
         ++sel_freq[it->second.banner_id][event_id];
      }
   }

   return sel_freq;
}

std::vector<std::string> tokenize(const std::string_view line) {
   std::stringstream sstream((line.data()));
   std::string token;

   static constexpr char request_type_separator = ':';
   static constexpr char separator = ',';

   std::vector<std::string> tokens{};
   std::getline(sstream, token, request_type_separator);
   tokens.emplace_back(token);
   while (std::getline(sstream, token, separator)) {
      tokens.emplace_back(token);
   }

   return tokens;
}

Agregator::SelRequest create_sel_request(const std::span<std::string> request) {
   static constexpr int SelRequestSize = 5;
   if (request.size() != SelRequestSize) {
      throw std::invalid_argument("Invalid sel request");
   }
   Agregator::SelRequest sel_request{};
   // request[0] is ignored here because it's a request type field, and it checked in another place.
   sel_request.event_id = std::stoi(request[1]);
   sel_request.uuid = request[2];
   sel_request.banner_id = request[3];
   sel_request.price = std::stod(request[4]);

   return sel_request;
}

Agregator::CntRequest create_cnt_request(const std::span<std::string> request) {

   static constexpr int CntRequestSize = 3;
   if (request.size() != CntRequestSize) {
      throw std::invalid_argument("Invalid cnt request");
   }
   Agregator::CntRequest cnt_request{};

   cnt_request.event_id = std::stoi(request[1]);
   cnt_request.sel_request_uuid = request[2];

   return cnt_request;
}

Agregator::Banners process_banners(const Agregator::SelRequestsStorage& sel_storage, const Agregator::CntRequestsStorage& cnt_storage)
{
   Agregator::Banners banners;
   for (const auto&[event_id, sel_request_uuid] : cnt_storage) {
      if (auto it = sel_storage.find(sel_request_uuid);it != sel_storage.end()) {
         const auto& sel_request = it->second;
         if (auto it_banners = banners.find(sel_request.banner_id); it_banners == banners.end()) {
            Agregator::BannerRequests banner_requests{};
            Agregator::Events events{};
            events.emplace(sel_request.event_id);
            events.emplace(event_id);
            banner_requests.emplace(sel_request_uuid, std::move(events));
            banners.emplace(sel_request.banner_id, std::move(banner_requests));
         }
         else {
            auto& banner_requests = it_banners->second;
            if (auto it_req = banner_requests.find(sel_request_uuid); it_req != banner_requests.end()) {
               auto& events = it_req->second;
               events.emplace(event_id);
            }
            else {
               banner_requests[sel_request_uuid].emplace(event_id);
            }
         }
      }
   }
   return banners;
}

std::string serialize(const Agregator::Banners& banners, const Agregator::BannersPrices& banners_prices, const Agregator::EventsAmount& events_amount) {
   std::stringstream sstream;
   sstream << "<Banners>\n";
   for (const auto& banner_id : banners | std::views::keys) {
       sstream << "    <Banner id=\"" << banner_id << "\"" << " revenues=\"" << banners_prices.at(banner_id) << "\"" << ">\n";
       sstream << "        <Events>\n";
       const auto& banner_events = events_amount.at(banner_id);
       for (const auto& event_id : banner_events | std::views::keys) {
             sstream << "            <Event id=\"" << event_id << "\">" << banner_events.at(event_id) << "</Event>\n";
       }
       sstream << "        </Events>\n";
      sstream << "    </Banner>\n";
   }
   sstream << "</Banners>";
   return sstream.str();
}

int main() {
   try {
      const std::string input_path = "/Users/tatyana.ryabkova/CLionProjects/aggregator/data/data.log";
      std::ifstream input_file_stream {input_path};
      Agregator::SelRequestsStorage sel_requests_storage;
      Agregator::CntRequestsStorage cnt_requests_storage;

      if (!input_file_stream.is_open()) {
         std::cout << "failed to open " << input_path << '\n';
      }
      else {
         std::string line;
         while(getline(input_file_stream, line, input_file_stream.widen('\n'))) {
            auto request = tokenize(line);
            if (request.empty()) {
               throw std::invalid_argument("The request can't be empty.");
            }
            switch (get_request_type(request)) {
               case Agregator::RequestType::sel: {
                  auto sel_request = create_sel_request(request);
                  const auto& sel_request_uuid = sel_request.uuid;
                  sel_requests_storage.emplace(sel_request_uuid, std::move(sel_request));
                  break;
               }
               case Agregator::RequestType::cnt: {
                  auto cnt_request = create_cnt_request(request);
                  cnt_requests_storage.emplace_back(std::move(cnt_request));
                  break;
               }
               default:
                  break;
            }
         }
      }
      auto banners = process_banners(sel_requests_storage, cnt_requests_storage);
      auto events_amount = process_events_amount(sel_requests_storage, cnt_requests_storage);
      auto banners_prices = process_banners_prices(sel_requests_storage, banners);
      std::string resulting_xml = serialize(banners, banners_prices, events_amount);
      const std::string output_path = "/Users/tatyana.ryabkova/CLionProjects/aggregator/data/aggregation_result.xml";
      std::ofstream output_file_stream{output_path};
      output_file_stream << resulting_xml;
   }
   catch (const std::exception& ex) {
      std::cerr << ex.what() << std::endl;
   }
   catch(...) {
      std::cerr << "An error occurred!" << std::endl;
   }
   return 0;
}