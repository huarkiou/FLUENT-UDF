#ifndef _UDFWARPPER_UTIL_HPP
#define _UDFWARPPER_UTIL_HPP

#include <numbers>
#include <ranges>
#include <string>

namespace util {
using std::numbers::pi;

static constexpr double deg2rad_scale_factor = pi / 180.;
static constexpr double rad2deg_scale_factor = 180. / pi;

constexpr double deg2rad(double deg) {
    return deg * std::numbers::pi / 180.;
}

template <std::ranges::range T>
constexpr T deg2rad(const T& degs) {
    T ret = degs;
    for (auto& deg : ret) {
        deg = deg2rad(deg);
    }
    return ret;
}

constexpr double rad2deg(double rad) {
    return rad * 180. / std::numbers::pi;
}

template <std::ranges::range T>
constexpr T rad2deg(const T& rads) {
    T ret = rads;
    for (auto& rad : ret) {
        rad = rad2deg(rad);
    }
    return ret;
}

inline std::string& string_replace(std::string& strBase, std::string_view strSrc, std::string_view strDes) {
    std::string::size_type pos = 0;
    decltype(strSrc)::size_type srcLen = strSrc.size();
    decltype(strDes)::size_type desLen = strDes.size();
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

}  // namespace util

#endif