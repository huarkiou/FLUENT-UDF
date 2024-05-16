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
#include <vector>

namespace udf {

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

constexpr std::vector<real> deg2rad(const std::vector<real>& degs) {
    std::vector<real> ret = degs;
    for (auto& deg : ret) {
        deg = deg2rad(deg);
    }
    return ret;
}

constexpr double rad2deg(double rad) {
    return rad * 180. / std::numbers::pi;
}

constexpr std::vector<real> rad2deg(const std::vector<real>& rads) {
    std::vector<real> ret = rads;
    for (auto& rad : ret) {
        rad = rad2deg(rad);
    }
    return ret;
}

inline std::string& string_replace(std::string& strBase, std::string strSrc, std::string strDes) {
    std::string::size_type pos = 0;
    std::string::size_type srcLen = strSrc.size();
    std::string::size_type desLen = strDes.size();
    pos = strBase.find(strSrc, pos);
    while ((pos != std::string::npos)) {
        strBase.replace(pos, srcLen, strDes);
        pos = strBase.find(strSrc, (pos + desLen));
    }
    return strBase;
}

inline std::string& string_trim(std::string& text) {
    if (!text.empty()) {
        text.erase(0, text.find_first_not_of(" \n\r\t"));
        text.erase(text.find_last_not_of(" \n\r\t") + 1);
    }
    return text;
}

template <typename T>
void host_to_node_vector(std::vector<T>& data) {
    int64_t size = data.size();
    host_to_node_int64_1(size);
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
        println("host_to_node_vector: Error T in std::vector<T>");
        throw std::logic_error("error type");
    }
}

template <typename T>
void node_to_host_vector(std::vector<T>& data) {
    int64_t size = data.size();
    node_to_host_int64_1(size);
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
        println("node_to_host_vector: Error T in std::vector<T>");
        throw std::logic_error("error type");
    }
}

}  // namespace udf

#endif