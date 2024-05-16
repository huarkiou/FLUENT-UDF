extern "C" {
#include "udf.h"
}
extern "C" {
#include "dpm.h"
#include "hdfio.h"
}
#include <format>
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
