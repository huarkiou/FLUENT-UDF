#ifndef _UDFWARPPER_UDF_HPP
#define _UDFWARPPER_UDF_HPP

#include <cassert>
#include <stdexcept>
extern "C" {
// clang-format off
#include "udf.h"
#include "dpm.h"
#include "hdfio.h"
#include "dynamesh_tools.h"
// clang-format on
}
#include <format>
#include <string>
#include <vector>

namespace udf {
// 3.7. Input/Output Functions

template <class... Args>
void print(const std::format_string<Args...> fmt, Args&&... args) {
    Message("%s", std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
}

template <class... Args>
void println(const std::format_string<Args...> fmt, Args&&... args) {
    Message("%s\n", std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
}

template <class... Args>
void info(const std::format_string<Args...> fmt, Args&&... args) {
    Message("id-%-6d: %s\n", myid, std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
}

template <class... Args>
void error(const std::format_string<Args...> fmt, Args&&... args) {
    Error("id-%-6d: %s\n", myid, std::vformat(fmt.get(), std::make_format_args(args...)).c_str());
}
// 7.3.2.2. Communicating Between the Host and Node Processes

template <typename T>
void host_to_node_data(T&) = delete;

template <>
inline void host_to_node_data<std::string>(std::string& str) {
    int64_t size = str.size();
    host_to_node_int64_1(size);
    str.resize(size);
    host_to_node_string(str.data(), size);
}

template <typename T>
inline void host_to_node_data(std::vector<T>& data) {
    int64_t size = data.size();
    host_to_node_int64_1(size);
    assert(size > 0);
    data.resize(size);
    if constexpr (std::is_same_v<T, int>) {
        host_to_node_int(data.data(), size);
    } else if constexpr (std::is_same_v<T, int64_t>) {
        host_to_node_int64(data.data(), size);
    } else if constexpr (std::is_same_v<T, real>) {
        host_to_node_real(data.data(), size);
    } else if constexpr (std::is_same_v<T, bool>) {
        host_to_node_boolean(data.data(), size);
    } else {
        error("host_to_node_vector: Error T in std::vector<T>");
    }
}

template <typename T>
void node_to_host_data(T&) = delete;

template <>
inline void node_to_host_data<std::string>(std::string& str) {
    int64_t size = str.size();
    node_to_host_int64_1(size);
#if RP_HOST
    assert(size > 0);
#endif
    str.resize(size);
    node_to_host_string(str.data(), size);
}

template <typename T>
inline void node_to_host_data(std::vector<T>& data) {
    int64_t size = data.size();
    node_to_host_int64_1(size);
    assert(size > 0);
    data.resize(size);
    if constexpr (std::is_same_v<T, int>) {
        node_to_host_int(data.data(), size);
    } else if constexpr (std::is_same_v<T, int64_t>) {
        node_to_host_int64(data.data(), size);
    } else if constexpr (std::is_same_v<T, real>) {
        node_to_host_real(data.data(), size);
    } else if constexpr (std::is_same_v<T, bool>) {
        node_to_host_boolean(data.data(), size);
    } else {
        error("node_to_host_vector: Error T in std::vector<T>");
    }
}

}  // namespace udf

#endif