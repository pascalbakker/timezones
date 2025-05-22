#include <getopt.h>

#include <cctype>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <unistd.h>
#include <vector>

using namespace std::chrono;
using namespace std::chrono_literals;

const std::vector<std::string> important_time_zones_vec = {
    "UTC",
    "America/New_York",
    "America/Los_Angeles",
    "Europe/London",
    "Europe/Amsterdam",
    "Asia/Kolkata",
    "Asia/Shanghai",
    "Australia/Sydney",
    "Asia/Tokyo",
    "America/Sao_Paulo"};

struct command_args {
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

static auto iso_to_utc_time_point(const std::string_view &str)
    -> std::chrono::system_clock::time_point {
    std::istringstream ss{std::string{str}};
    std::chrono::system_clock::time_point tp;
    // ss >> std::chrono::parse("%FT%TZ", tp);
    ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%S%z", tp);
    return tp;
}

static auto format_hour(int hour) -> std::string {
    std::ostringstream oss;
    // If hour is less than 10, prepend 0
    if (hour < 10) {
        oss << "0" << hour;
    } else {
        oss << hour;
    }
    return oss.str();
}

static auto convert_hours_to_string(int hours) {
    return std::string(hours < 0 ? "-" : "+") +
           std::string(hours < 10 ? "0" : "") + std::to_string(abs(hours));
}

static auto convert_minutes_to_string(int minutes) {
    return std::string(minutes < 10 ? "0" : "") + std::to_string(abs(minutes));
}

static auto get_offset_for_timezone(const std::string &tz_name) -> std::string {
    try {
        auto zone = std::chrono::locate_zone(tz_name);
        auto now = std::chrono::system_clock::now();
        auto offset = zone->get_info(now).offset;

        int hours =
            std::chrono::duration_cast<std::chrono::hours>(offset).count();
        int minutes =
            std::chrono::duration_cast<std::chrono::minutes>(offset).count() %
            60;

        return convert_hours_to_string(hours) + ":" +
               convert_minutes_to_string(minutes);
    } catch (const std::exception &e) {
        std::cerr << "Invalid time zone" << std::endl;
    }
}

static void print_table_header() {
    std::cout << std::left << std::setw(10) << "Code" << std::setw(30) << "TZ"
              << std::setw(40) << "ISO" << std::endl;
}

template <class Duration, class TimeZonePtr = const std::chrono::time_zone *>
void process_zoned_time(
    const std::string tz,
    const std::chrono::zoned_time<Duration, TimeZonePtr> &zt) {
    std::cout << std::left << std::setw(10) << std::format("{:%Z}", zt)
              << std::setw(30) << tz << std::setw(40)
              << std::format("{:%FT%T}", zt) << std::endl;
}

static command_args convert_user_input_to_config(int argc, char *argv[]) {
    command_args config;
    int opt;
    std::vector<std::string> output_timezones;
    while ((opt = getopt(argc, argv, "cli:t:o:v")) != -1) {
        switch (opt) {
        case 'c':
            config.c_flag = true;
            break;
        case 'l':
            config.l_flag = false;
            break;
        case 'i':
            config.input_time = optarg;
            std::cout << "\nTime given: " << config.input_time;
            break;
        case 'o':
            output_timezones.push_back(optarg);
            break;
        case 't':
            config.input_timezone = optarg;
            std::cout << "\nTimezone given: " << config.input_timezone;
            break;
        case 'v':
            break; // handle if needed
        case '?':
        default:
            std::cerr << "Invalid option: -" << static_cast<char>(optopt)
                      << '\n';
            exit(EXIT_FAILURE);
        }
    }

    // Catch extra arguments
    for (int i = optind; i < argc; ++i) {
        std::cerr << "Unexpected argument: " << argv[i] << '\n';
    }

    if (config.output_zones.empty()) {
        config.output_zones.push_back("UTC");
    }
    config.output_zones = output_timezones;
    std::cout << "\n";
    return config;
}

static void print_utc_into_given_zones(
    std::chrono::time_point<std::chrono::system_clock> utc_time_point,
    std::vector<std::string> time_zone_list) {
    print_table_header();
    for (const std::string &time_zone_str : time_zone_list) {
        const time_zone *time_zone = std::chrono::locate_zone(time_zone_str);
        const auto current_time_converted =
            zoned_time(time_zone, utc_time_point);
        process_zoned_time(time_zone_str, current_time_converted);
    }
}

static void convert_current_time_to_all_zones() {
    const auto current_time = system_clock::now();
    const auto now_in_seconds = time_point_cast<seconds>(current_time);
    print_table_header();
    for (const auto &time_zone_str : important_time_zones_vec) {
        const auto time_zone = std::chrono::locate_zone(time_zone_str);
        const auto current_time_converted =
            zoned_time(time_zone, now_in_seconds);
        process_zoned_time(time_zone_str, current_time_converted);
    }
}

static void convert_time_zone_with_confg(const command_args &command_args) {
    std::regex tz_regex(R"([+-]\d{2}:\d{2}$)");
    std::string tz_name = "America/Chicago"; // CST or CDT based on time of year
    std::string offset = get_offset_for_timezone(tz_name);
    const auto custom_time_str =
        std::string("2025-03-25T23:55:33") + std::string(offset);
    const time_point custom_time = iso_to_utc_time_point(custom_time_str);

    std::smatch match;
    if ('z' == std::tolower(command_args.input_time.back())) {
        // If input timezone ends with z assume it is UTC format
        // Then convert to the given output format
        std::istringstream ss{command_args.input_time};
        std::chrono::system_clock::time_point tp;
        ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", tp);
        std::vector time_zones_to_print = command_args.output_zones;
        std::chrono::system_clock::time_point utc_time =
            iso_to_utc_time_point(command_args.input_time);
        // time_zones_to_print.push_back(command_args.input_time);
        print_utc_into_given_zones(tp, command_args.output_zones);
    } else if (!command_args.input_timezone.empty()) {
        // If timezone is not 'z' but -t zone is given
        std::string offset_with_given_input_zone =
            get_offset_for_timezone(command_args.input_timezone);
        const auto time_zone_string_with_offset =
            std::string(command_args.input_time) + std::string(offset);
        const time_point time_with_custom_timezone_time_point =
            iso_to_utc_time_point(time_zone_string_with_offset);
        print_utc_into_given_zones(time_with_custom_timezone_time_point,
                                   command_args.output_zones);
    }

    else if (std::regex_search(custom_time_str, match, tz_regex)) {
        // Timezone is included in [+-]xx:xx format
        const auto input_timestamp_with_tz = command_args.input_time;
        const time_point custom_time = time_point_cast<seconds>(
            iso_to_utc_time_point(input_timestamp_with_tz));
        print_utc_into_given_zones(custom_time, command_args.output_zones);
    } else if (!command_args.input_timezone.empty()) {
        // Timezone given in '-t' param
        const std::string tz_offset =
            get_offset_for_timezone(command_args.input_timezone);
        const auto input_timestamp_with_tz =
            command_args.input_time + std::string(tz_offset);
        const time_point custom_time = time_point_cast<seconds>(
            iso_to_utc_time_point(input_timestamp_with_tz));
        print_utc_into_given_zones(custom_time, command_args.output_zones);
    } else {
        // Default to UTC timezone
        std::cout << command_args.input_time;
        time_point tp = iso_to_utc_time_point(command_args.input_time);
        print_utc_into_given_zones(custom_time, command_args.output_zones);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // If no args passed, just display a list of timezones
        // of the current time
        convert_current_time_to_all_zones();
    } else {
        const command_args user_input_args =
            convert_user_input_to_config(argc, argv);
        convert_time_zone_with_confg(user_input_args);
    }
}
