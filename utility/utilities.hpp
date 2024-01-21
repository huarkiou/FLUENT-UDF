/**
  这个文件必须要c++20标准才能编译
 */

#ifndef _HUARKIOU_UDF_UTILITIES_HPP
#define _HUARKIOU_UDF_UTILITIES_HPP
extern "C" {
#include "udf.h"
}

#include <format>

namespace hku {

template <class... Args>
void print(const std::format_string<Args...> fmt, Args&&... args) {
    if (myid == 999999) {
        Message("Host%-6d: %s", myid, std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
    } else {
        Message("Node%-6d: %s", myid, std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
    }
}

template <class... Args>
void println(const std::format_string<Args...> fmt, Args&&... args) {
    if (myid == 999999) {
        Message("Host%-6d: %s\n", myid, std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
    } else {
        Message("Node%-6d: %s\n", myid, std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
    }
}

}  // namespace hku

#endif