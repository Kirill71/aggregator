#include <iostream>
#include <fstream>

#include "aggregator.h"

int main(int argc, char* argv[])
{
    static constexpr int expected_args_count = 3;
    if (argc != expected_args_count)
    {
        std::cerr << "Usage: " << argv[0] << std::endl;
        std::cerr << "The first parameter should be a path to an input csv file." << std::endl;
        std::cerr << "The second parameter should be a path to an output xml file." << std::endl;
        return -1;
    }

    try
    {
        const std::string input_path = argv[1];
        std::ifstream input_file_stream{input_path};

        if (!input_file_stream.is_open())
        {
            std::cerr << "failed to open " << input_path << std::endl;
            return -1;
        }

        Aggregator::SelRequestsStorage sel_requests_storage{};
        Aggregator::CntRequestsStorage cnt_requests_storage{};
        std::string line{};

        while (getline(input_file_stream, line, input_file_stream.widen('\n')))
        {
            auto request = Aggregator::tokenize(line);
            if (request.empty())
            {
                throw std::invalid_argument("The request can't be empty.");
            }

            switch (Aggregator::get_request_type(request))
            {
                case Aggregator::RequestType::sel:
                {
                    auto sel_request = Aggregator::create_sel_request(request);
                    const auto& sel_request_uuid = sel_request.uuid;
                    sel_requests_storage.emplace(sel_request_uuid, std::move(sel_request));
                    break;
                }
                case Aggregator::RequestType::cnt:
                {
                    auto cnt_request = Aggregator::create_cnt_request(request);
                    cnt_requests_storage.emplace_back(std::move(cnt_request));
                    break;
                }
                default:
                    break;
            }
        }

        auto banners = process_banners(sel_requests_storage, cnt_requests_storage);
        auto events_amount = process_events_amount(sel_requests_storage, cnt_requests_storage);
        auto banners_prices = process_banners_prices(sel_requests_storage, banners);

        std::string resulting_xml = Aggregator::serialize(banners, banners_prices, events_amount);
        const std::string output_path = argv[2];
        std::ofstream output_file_stream{output_path};
        output_file_stream << resulting_xml;
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "An error occurred!" << std::endl;
    }

    return 0;
}
