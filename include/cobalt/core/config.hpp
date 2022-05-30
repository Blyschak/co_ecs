#pragma once

#include <cobalt/asl/bits.hpp>
#include <cobalt/asl/hash_map.hpp>
#include <cobalt/asl/string_utils.hpp>

#include <istream>

namespace cobalt {

/// @brief config class that holds a map of keys and values that can be read from .ini file
class config {
public:
    /// @brief Construct a new config object
    config() = default;

    /// @brief Construct a new config object
    config(config&&) = default;

    /// @brief Move assignment operator
    ///
    /// @return config&
    config& operator=(config&&) = default;

    /// @brief Create config from input stream
    ///
    /// @param input Input stream
    /// @return config Config object
    static config from_stream(std::istream& input) {
        return config(input);
    }

    /// @brief Create config from input stream
    ///
    /// @param input Input stream
    /// @return config Config object
    static config from_stream(std::istream&& input) {
        return config(input);
    }

    /// @brief Get config value by key
    ///
    /// @param key Key to read
    /// @return const std::string& Result
    const std::string& get(std::string key) const {
        return _setting_map.at(key);
    }

    /// @brief Set the default value into settings map, does not override if already present
    ///
    /// @tparam T Value type
    /// @param key Key to insert
    /// @param value Value
    template<typename T>
    void set_default(std::string key, T value) {
        if constexpr (std::is_convertible_v<T, std::string>) {
            _setting_map.emplace(key, value);
        } else {
            set_default(key, std::to_string(value));
        }
    }

private:
    config(std::istream& input) {
        std::string temp;
        while (std::getline(input, temp)) {
            if (temp.empty()) {
                continue;
            }

            auto pos = temp.find('=');
            if (pos == std::string::npos) {
                continue;
            }

            auto key = asl::trim(temp.substr(0, pos));
            auto value = asl::trim(temp.substr(pos + 1));
            _setting_map.emplace(key, value);
        }
    }

    asl::hash_map<std::string, std::string> _setting_map;
};

} // namespace cobalt