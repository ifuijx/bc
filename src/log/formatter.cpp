#include <utility>

#include <bc/log/record.hpp>
#include <bc/log/formatter.hpp>

namespace bc::log {

auto level_string(level level) -> std::string_view {
    using namespace std::string_view_literals;

    static std::string_view levels[] {
        "DEBUG"sv, "INFO"sv, "WARNING"sv, "ERROR"sv, "FATAL"sv,
    };

    return levels[std::to_underlying(level)];
}

formatter::formatter(std::string format) : format_(std::move(format)) {
    enum State {
        RAW,
        META,
    };
    State state = RAW;

    std::size_t s = 0;
    for (std::size_t i = 0; i < format_.size(); ++i) {
        char c = format_[i];
        if (state == RAW && c == '%' && i < format_.size() - 1 && format_[i + 1] == '{') {
            if (i != s) {
                segments_.emplace_back(std::string_view(format_.data() + s, format_.data() + i));
            }
            s = i + 2;
            i = i + 1;
            state = META;
        }
        else if (state == META && c == '}') {
            std::string_view segment(format_.data() + s, format_.data() + i);
            if (segment == "datetime") {
                segments_.push_back(meta_type::DATETIME);
            }
            else if (segment == "level") {
                segments_.push_back(meta_type::LEVEL);
            }
            else if (segment == "topic") {
                segments_.push_back(meta_type::TOPIC);
            }
            else if (segment == "thread") {
                segments_.push_back(meta_type::THREAD);
            }
            else if (segment == "location") {
                segments_.push_back(meta_type::LOCATION);
            }
            else if (segment == "message") {
                segments_.push_back(meta_type::MESSAGE);
            }
            s = i + 1;
            state = RAW;
        }
    }
    if (state == RAW && s != format_.size()) {
        if (state == RAW) {
            segments_.emplace_back(std::string_view(format_.data() + s, format_.data() + format_.size()));
        }
    }
}

auto formatter::repr() const -> std::string {
    std::string repr;
    for (auto const &segment : segments_) {
        std::visit(utils::overload([&](std::string_view segment) {
            repr += segment;
        }, [&](meta_type meta) {
            switch (meta) {
                case meta_type::DATETIME: repr.append("%{datetime}"); break;
                case meta_type::LEVEL: repr.append("%{level}"); break;
                case meta_type::TOPIC: repr.append("%{topic}"); break;
                case meta_type::THREAD: repr.append("%{thread}"); break;
                case meta_type::LOCATION: repr.append("%{location}"); break;
                case meta_type::MESSAGE: repr.append("%{message}"); break;
            }
        }), segment);
    }
    return repr;
}

} /* namespace bc::log */
