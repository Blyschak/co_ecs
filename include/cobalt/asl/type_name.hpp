#pragma once

#include <cobalt/asl/platform.hpp>

#include <string_view>

namespace cobalt::asl {

#ifdef COBALT_COMPILER_GCC

template<typename T>
struct type_name {
private:
    static constexpr auto get() noexcept {
        constexpr std::string_view full_name{ __PRETTY_FUNCTION__ };
        constexpr std::string_view left_marker{ "[with T = " };
        constexpr std::string_view right_marker{ "]" };

        constexpr auto left_marker_index = full_name.find(left_marker);
        static_assert(left_marker_index != std::string_view::npos);
        constexpr auto start_index = left_marker_index + left_marker.size();
        constexpr auto end_index = full_name.find(right_marker, left_marker_index);
        static_assert(end_index != std::string_view::npos);
        constexpr auto length = end_index - start_index;

        return full_name.substr(start_index, length);
    }

public:
    using value_type = std::string_view;
    static constexpr value_type value{ get() };

    constexpr operator value_type() const noexcept {
        return value;
    }
    constexpr value_type operator()() const noexcept {
        return value;
    }
};

#else
#ifdef COBALT_COMPILER_CLANG
template<typename T>
struct type_name
{
private:
	static constexpr auto get() noexcept {
		constexpr std::string_view full_name{ __PRETTY_FUNCTION__ };
		constexpr std::string_view left_marker{ "[T = " };
		constexpr std::string_view right_marker{ "]" };

		constexpr auto left_marker_index = full_name.find(left_marker);
		static_assert(left_marker_index != std::string_view::npos);
		constexpr auto start_index = left_marker_index + left_marker.size();
		constexpr auto end_index = full_name.find(right_marker, left_marker_index);
		static_assert(end_index != std::string_view::npos);
		constexpr auto length = end_index - start_index;

		return full_name.substr(start_index, length);
	}

public:
	using value_type = std::string_view;
	static constexpr value_type value{ get() };

	constexpr operator value_type() const noexcept { return value; }
	constexpr value_type operator()() const noexcept { return value; }
};
#else
#error Not supported compiler
#endif
#endif

template<typename T>
inline constexpr auto type_name_v = type_name<T>::value;

} // namespace cobalt::asl