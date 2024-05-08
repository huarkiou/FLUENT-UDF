/**
  这个文件必须要c++20标准才能编译
 */

#ifndef _HUARKIOU_UDF_UTILITIES_HPP
#define _HUARKIOU_UDF_UTILITIES_HPP
extern "C" {
#include "udf.h"
}

#include <format>
#include <numbers>

namespace hku {

template <class... Args>
void print(const std::format_string<Args...> fmt, Args&&... args) {
#if RP_HOST
    Message("Host%-6d: %s", NODE_HOST, std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
#else
    Message("Node%-6d: %s", myid, std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
#endif
}

template <class... Args>
void println(const std::format_string<Args...> fmt, Args&&... args) {
#if RP_HOST
    Message("Host%-6d: %s\n", NODE_HOST, std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
#else
    Message("Node%-6d: %s\n", myid, std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
#endif
}

constexpr double deg2rad(double deg) {
    return deg * std::numbers::pi / 180.;
}

constexpr double rad2deg(double rad) {
    return rad * 180. / std::numbers::pi;
}

}  // namespace hku

#endif