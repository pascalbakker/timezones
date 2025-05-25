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

const std::string UTC_TIMEZONE = "UTC";

const std::vector<std::string> IMPORTANT_TIME_ZONES_VEC = {
    UTC_TIMEZONE,       "America/New_York", "America/Los_Angeles",
    "Europe/London",    "Europe/Amsterdam", "Asia/Kolkata",
    "Asia/Shanghai",    "Australia/Sydney", "Asia/Tokyo",
    "America/Sao_Paulo"};

struct command_args {
    // '-l' List all timezones
    bool l_flag = false;
    // '-i' Input timestamp in ISO format
    std::string input_time;
    // '-o' Timezone to convert to. Default is UTC
    std::vector<std::string> output_zones = {UTC_TIMEZONE};
    // '-t' Timezone used for converting. Default is local
    std::string input_timezone = "local";
};

static std::chrono::system_clock::time_point
iso_to_utc_time_point(const std::string_view &str) {
    std::istringstream ss{std::string{str}};
    std::chrono::system_clock::time_point tp;
    // ss >> std::chrono::parse("%FT%TZ", tp);
    ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%S%z", tp);
    return tp;
}

static std::string convert_hours_to_string(int hours) {
    return std::string(hours < 0 ? "-" : "+") +
           std::string(hours < 10 ? "0" : "") + std::to_string(abs(hours));
}

static std::string convert_minutes_to_string(int minutes) {
    return std::string(minutes < 10 ? "0" : "") + std::to_string(abs(minutes));
}

static std::string get_offset_for_timezone(const std::string &tz_name) {
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

    return "";
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
        // list IMPORTANT_TIMEZONES
        case 'l':
            config.l_flag = true;
            break;
        // take input time for conversion
        case 'i':
            config.input_time = optarg;
            break;
        // output time for conversion
        case 'o':
            output_timezones.push_back(optarg);
            break;
        // input timezone for conversion
        case 't':
            config.input_timezone = optarg;
            break;
        default:
            std::cerr << "Invalid option: -" << static_cast<char>(optopt)
                      << '\n';
            exit(EXIT_FAILURE);
        }
    }

    // Catch all other unexpected arguments
    for (int i = optind; i < argc; ++i) {
        std::cerr << "Unexpected argument: " << argv[i] << '\n';
    }

    // set default timezone to input to UTC
    if (config.output_zones.empty()) {
        config.output_zones.push_back(UTC_TIMEZONE);
    }

    config.output_zones = output_timezones;
    return config;
}

static void print_a_list_of_timezones(
    std::vector<std::string> time_zone_list,
    std::chrono::time_point<std::chrono::system_clock> time_point) {
    print_table_header();
    for (const std::string &time_zone_str : time_zone_list) {
        const time_zone *time_zone = std::chrono::locate_zone(time_zone_str);
        const auto current_time_converted = zoned_time(time_zone, time_point);
        process_zoned_time(time_zone_str, current_time_converted);
    }
}

static std::string
get_timezone_based_on_input(const std::string &input_timezone) {
    return input_timezone.empty() ? UTC_TIMEZONE
                                  : get_offset_for_timezone(input_timezone);
}

static bool does_timezone_end_in_z(const command_args &command_args) {
    return !command_args.input_timezone.empty() &&
           'z' == std::tolower(command_args.input_time.back());
}

/*
** Different methods for printing timezones
*/
static void convert_current_time_to_all_zones() {
    const auto current_time = system_clock::now();
    const auto now_in_seconds = time_point_cast<seconds>(current_time);
    print_a_list_of_timezones(IMPORTANT_TIME_ZONES_VEC, now_in_seconds);
}

static void
print_timezone_with_utc_zone_as_input(const command_args &command_args) {
    // If input timezone ends with z assume it is UTC format
    // Then convert to the given output format
    std::istringstream ss{command_args.input_time};
    std::chrono::system_clock::time_point tp;
    ss >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", tp);
    std::vector time_zones_to_print = command_args.output_zones;
    std::chrono::system_clock::time_point utc_time =
        iso_to_utc_time_point(command_args.input_time);
    // print timezones with base zone being UTC
    print_a_list_of_timezones(command_args.output_zones, tp);
}

static void
print_timezone_with_zone_given_in_input_time(const command_args &command_args) {
    const auto input_timestamp_with_tz = command_args.input_time;
    const time_point custom_time = time_point_cast<seconds>(
        iso_to_utc_time_point(input_timestamp_with_tz));
    print_a_list_of_timezones(command_args.output_zones, custom_time);
}

static void
print_timezone_with_unknown_input_timezone(const command_args &command_args) {
    // Default to UTC timezone
    const time_point custom_time =
        iso_to_utc_time_point(command_args.input_time);
    time_point tp = iso_to_utc_time_point(command_args.input_time);
    print_a_list_of_timezones(command_args.output_zones, custom_time);
}

static void
convert_time_zone_with_given_input_timezone(const command_args &command_args) {
    // If timezone is not 'z' but -t zone is given
    std::string offset_with_given_input_zone =
        get_offset_for_timezone(command_args.input_timezone);
    const auto time_zone_string_with_offset =
        std::string(command_args.input_time) +
        std::string(offset_with_given_input_zone);
    const time_point time_with_custom_timezone_time_point =
        iso_to_utc_time_point(time_zone_string_with_offset);
    print_a_list_of_timezones(command_args.output_zones,
                              time_with_custom_timezone_time_point);
}

/*
** Main logic for deciding which timezones to print
*/
static void convert_time_zone_with_config(const command_args &command_args) {
    std::regex tz_regex(R"([+-]\d{2}:\d{2}$)");
    std::smatch match;
    if (command_args.l_flag) {
        // If -l dispaly list of predefined zones
        convert_current_time_to_all_zones();
    } else if (does_timezone_end_in_z(command_args)) {
        // If input timezone ends with z assume it is UTC format
        // Then convert to the given output format
        print_timezone_with_utc_zone_as_input(command_args);
    } else if (!command_args.input_timezone.empty()) {
        // If timezone not z and -t given,
        // then add offset to input timezone
        // before converting to the other timezones
        convert_time_zone_with_given_input_timezone(command_args);
    } else if (std::regex_search(command_args.input_time, match, tz_regex)) {
        // If timezone is included in [+-]xx:xx format
        // Then just convert to time_point
        print_timezone_with_zone_given_in_input_time(command_args);
    } else {
        // Default to UTC timezone if no input timezone is given
        print_timezone_with_unknown_input_timezone(command_args);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // If no args passed, display a list of timezones
        // of the current time
        convert_current_time_to_all_zones();
    } else {
        // If additional args are passed, process them
        const command_args user_input_args =
            convert_user_input_to_config(argc, argv);
        convert_time_zone_with_config(user_input_args);
    }
}
