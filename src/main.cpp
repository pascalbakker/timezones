#include <getopt.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std::chrono;
using namespace std::chrono_literals;

const std::vector<std::string> important_time_zones_vec
    = {"UTC",
       "America/New_York",
       "America/Los_Angeles",
       "Europe/London",
       "Europe/Amsterdam",
       "Asia/Kolkata",
       "Asia/Shanghai",
       "Australia/Sydney",
       "Asia/Tokyo",
       "America/Sao_Paulo"};

struct command_args
{
    // '-c' Use current time to convert
    bool c_flag = false;
    // '-l' List all timezones
    bool l_flag = false;
    // '-i' Input timestamp in ISO format
    std::string input_time;
    // '-o' Timezone to convert to. Default is UTC
    std::vector<std::string> output_zones = {"UTC"};
    // '-t' Timezone used for converting. Default is local
    std::string input_timezone = "local";
};

static auto
iso_to_time_point(const std::string_view& str)
    -> std::chrono::system_clock::time_point
{
    std::istringstream ss{std::string{str}};
    std::chrono::system_clock::time_point tp;
    // ss >> std::chrono::parse("%FT%TZ", tp);
    ss >> parse("%Y-%m-%dT%H:%M:%S%z", tp);
    return tp;
}

static auto
format_hour(int hour) -> std::string
{
    std::ostringstream oss;
    // If hour is less than 10, prepend 0
    if (hour < 10)
    {
        oss << "0" << hour;
    }
    else
    {
        oss << hour;
    }
    return oss.str();
}

static auto
convert_hours_to_string(int hours)
{
    return std::string(hours < 0 ? "-" : "+")
           + std::string(hours < 10 ? "0" : "") + std::to_string(abs(hours));
}

static auto
convert_minutes_to_string(int minutes)
{
    return std::string(minutes < 10 ? "0" : "") + std::to_string(abs(minutes));
}

static auto
get_offset_for_timezone(const std::string& tz_name) -> std::string
{
    try
    {
        auto zone = std::chrono::locate_zone(tz_name);
        auto now = std::chrono::system_clock::now();
        auto offset = zone->get_info(now).offset;

        int hours
            = std::chrono::duration_cast<std::chrono::hours>(offset).count();
        int minutes
            = std::chrono::duration_cast<std::chrono::minutes>(offset).count()
              % 60;

        return convert_hours_to_string(hours) + ":"
               + convert_minutes_to_string(minutes);
    }
    catch (const std::exception& e)
    {
        return "Invalid time zone";
    }
}

void
print_table_header()
{
    std::cout << std::left << std::setw(10) << "Code" << std::setw(30) << "TZ"
              << std::setw(40) << "ISO" << std::endl;
}

template <class Duration, class TimeZonePtr = const std::chrono::time_zone*>
void
process_zoned_time(
    const std::string tz,
    const std::chrono::zoned_time<Duration, TimeZonePtr>& zt)
{
    std::cout << std::left << std::setw(10) << std::format("{:%Z}", zt)
              << std::setw(30) << tz << std::setw(40)
              << std::format("{:%FT%T}", zt) << std::endl;
}

static command_args
convert_user_input_to_config(const int argc, const char* argv[])
{
    command_args config;

    int opt;
    while ((opt = getopt(argc, argv, "ct:v")) != -1)
    {
        switch (opt)
        {
            case 'c':
                config.c_flag = true;
                break;
            case 'l':
                config.l_flag = false;
                break;
            case 'i':
                config.input_time = optarg;
                break;
            case 'o':
                config.output_zones.push_back(optarg);
                break;
            case 't':
                config.input_timezone = optarg;
                break;
            default:
                std::cerr << "Argument: " << opt << " not recognized.";
                return config;
        }
    }
    return config;
}

static void
convert_current_time_to_all_zones()
{
    const auto current_time = system_clock::now();
    const auto now_in_seconds = time_point_cast<seconds>(current_time);
    print_table_header();
    for (const auto& time_zone_str : important_time_zones_vec)
    {
        const auto time_zone = locate_zone(time_zone_str);
        const auto current_time_converted
            = zoned_time(time_zone, now_in_seconds);
        process_zoned_time(time_zone_str, current_time_converted);
    }
}

static void
convert_time_zone_with_confg(const command_args command_args)
{
    time_point tp = iso_to_time_point(command_args.input_time);
}

int
main(const int argc, const char* argv[])
{
    if (argc == 1)
    {
        convert_current_time_to_all_zones();
    }
    else
    {
        const command_args user_input_args
            = convert_user_input_to_config(argc, argv);
    }
}
