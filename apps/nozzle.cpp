#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "aerodynamics/isentropic.hpp"
#include "aerodynamics/material-property.hpp"
#include "udf.hpp"
#include "util.hpp"

namespace id {
// wall id
static constexpr int tj_wallu = 10;
static constexpr int tj_walld = 9;

static constexpr int sj_wallu_l = 25;
static constexpr int sj_wallu_r = 7;
static constexpr int sj_throatd = 2;
static constexpr int sj_throatu = 3;

static constexpr int splitteru_d = 12;
static constexpr int splitteru_u = 11;
static constexpr int splitterd_u = 4;
static constexpr int splitterd_d = 5;

}  // namespace id

static constexpr real eps = 1e-5;

namespace refvalue {
static constexpr real throatArea = 0.902;
static constexpr real sjExitArea = 2.737;
static constexpr real width = 2.227;
}  // namespace refvalue

static real NV_VEC(splitteru_d_origin){};
static constexpr int splitteru_d_id = id::splitteru_d;
static real NV_VEC(splitteru_u_origin){};
static constexpr int splitteru_u_id = id::splitteru_u;
static real NV_VEC(splitteru_end){};  //< 上一时间步结束后当前时间步开始时splitteru端点坐标

static real NV_VEC(splitterd_u_origin){};
static constexpr int splitterd_u_id = id::splitterd_u;
static real NV_VEC(splitterd_d_origin){};
static constexpr int splitterd_d_id = id::splitterd_d;
static real NV_VEC(splitterd_end){};  //< 上一时间步结束后当前时间步开始时splitterd端点坐标

static const hndp::aerodynamics::MaterialProperty mp = hndp::aerodynamics::MaterialProperty::air_piecewise_polynomial();

std::unordered_map<std::string, std::vector<real>> timepoints{};
std::unordered_map<std::string, std::vector<real>> values{};

static void read_valuepairs_from_file(const std::string& key, const std::string& filepath, double scale = 1.0) {
#if RP_HOST
    if (timepoints.count(key) > 0) {
        timepoints[key].clear();
    } else {
        timepoints.insert({key, std::vector<real>{}});
    }
    if (values.count(key) > 0) {
        values[key].clear();
    } else {
        values.insert({key, std::vector<real>{}});
    }
    std::ifstream fin{filepath};
    if (!fin.is_open()) {
        udf::error("Cannot open file {} ", filepath);
        throw std::runtime_error("Cannot open file");
    }
    while (!fin.eof()) {
        std::string line;
        std::getline(fin, line);
        line = util::string_trim(line);
        line = util::string_replace(line, "\t", " ");
        if (line.empty()) {
            continue;
        } else {
            real t{}, value{};
            try {
                std::stringstream ss{line};
                ss >> t >> value;
            } catch (const std::exception& err) {
                udf::info("{}", line);
                udf::info("{}, {}", t, value);
                udf::error("Error: {}", err.what());
                throw err;
            }
            if (timepoints[key].size() > 0 && t < timepoints[key].back()) {
                udf::error("time sequence not monotonic: {} {}", timepoints[key].back(), t);
                throw std::runtime_error("Invalid valuepairs");
            }
            timepoints[key].push_back(t);
            values[key].push_back(scale * value);
        }
    }
#endif
    udf::host_to_node_data(timepoints[key]);
    udf::host_to_node_data(values[key]);
}

static void write_valuepairs_to_file(const std::string& key, const std::string& filepath, double scale = 1.0) {
    udf::node_to_host_data(timepoints[key]);
    udf::node_to_host_data(values[key]);
#if RP_HOST
    std::ofstream fout{filepath, std::ios::out | std::ios::ate};
    if (!fout.is_open()) {
        udf::error("Cannot open file {} ", filepath);
        throw std::runtime_error("Cannot open file");
    }
    const auto& time = timepoints.at(key);
    const auto& value = values.at(key);
    if (time.size() != value.size()) {
        udf::error("Invalid data of {}", key);
        throw std::runtime_error("Invalid data");
    }

    for (size_t i = 0; i < time.size(); ++i) {
        fout << time[i] << ' ' << scale * value[i] << '\n';
    }
#endif
}

static real interpolate_value(const std::vector<real>& v1, const std::vector<real>& v2, real v) {
    assert(v1.size() > 1);
    assert(v1.size() == v2.size());
    assert(v1[0] < v1[1]);
    const int length = static_cast<int>(v2.size());
    if (v <= v1[0]) {
        return v2[0];
    }
    for (int i = 1; i < length; ++i) {
        if (v <= v1[i]) {
            return v2[i - 1] + (v - v1[i - 1]) * (v2[i] - v2[i - 1]) / (v1[i] - v1[i - 1]);
        }
    }
    return v2.back();
}

static inline real get_bcvalue_by_time(const std::string& name, real time) {
    return interpolate_value(timepoints[name], values[name], time);
}

static void cal_nozzle_1D_parameters(const std::string& key_prefix) {
    const std::vector<real> times = [&] {
        std::vector<real> times_tmp;
        const auto func_insert = [&](const std::string& key) {
            const auto& tmp = timepoints.at(key);
            times_tmp.insert(times_tmp.begin(), tmp.begin(), tmp.end());
        };
        func_insert(key_prefix + "_inlet_T_total");
        func_insert(key_prefix + "_inlet_p_total");
        func_insert(key_prefix + "_inlet_Ma");
        std::sort(times_tmp.begin(), times_tmp.end());
        auto new_last = std::unique(times_tmp.begin(), times_tmp.end(), [](const real& left, const real& right) {
            return std::abs(left - right) < 1e-5;
        });
        times_tmp.erase(new_last, times_tmp.end());
        return times_tmp;
    }();

    std::vector<real> vx_es;
    std::vector<real> ers;
    for (auto time : times) {
        const real p_total = get_bcvalue_by_time(key_prefix + "_inlet_p_total", time);
        const real T_total = get_bcvalue_by_time(key_prefix + "_inlet_T_total", time);
        const real Ma_i = get_bcvalue_by_time(key_prefix + "_inlet_Ma", time);
        const real T_static_i = hndp::isentropic::cal_T_static_by_Ma(mp, T_total, Ma_i);
        const real p_static_i = hndp::isentropic::cal_p_static_by_T_total_T_static(mp, p_total, T_total, T_static_i);
        const real rho_static_i = p_static_i / (mp.Rg() * T_static_i);
        const real vx_i = hndp::isentropic::cal_V_by_T_total_T_static(mp, T_total, T_static_i);

        const real p_ambient = get_bcvalue_by_time("farfield_inlet_p_static", time);
        const real T_static_e = hndp::isentropic::cal_T_static_by_p_static(mp, T_static_i, p_static_i, p_ambient);
        const real vx_e = hndp::isentropic::cal_V_by_T_total_T_static(mp, T_total, T_static_e);
        const real rho_static_e = p_ambient / (mp.Rg() * T_static_e);

        const real expansion_ratio_1D = (rho_static_i * vx_i) / (rho_static_e * vx_e);

        vx_es.push_back(vx_e);
        ers.push_back(expansion_ratio_1D);
    }

    timepoints.insert_or_assign(key_prefix + "_1D_vx_e", times);
    values.insert_or_assign(key_prefix + "_1D_vx_e", vx_es);

    timepoints.insert_or_assign(key_prefix + "_1D_er", times);
    values.insert_or_assign(key_prefix + "_1D_er", ers);
}

static void set_values_and_timepoints() {
    read_valuepairs_from_file("turbojet_inlet_T_total", "./parameters/turbojet_inlet_T_total.txt");
    read_valuepairs_from_file("turbojet_inlet_p_total", "./parameters/turbojet_inlet_p_total.txt");
    read_valuepairs_from_file("turbojet_inlet_Ma", "./parameters/turbojet_inlet_Ma.txt");
    read_valuepairs_from_file("turbojet_exit_area", "./parameters/turbojet_exit_area.txt");

    read_valuepairs_from_file("scramjet_inlet_T_total", "./parameters/scramjet_inlet_T_total.txt");
    read_valuepairs_from_file("scramjet_inlet_p_total", "./parameters/scramjet_inlet_p_total.txt");
    read_valuepairs_from_file("scramjet_inlet_Ma", "./parameters/scramjet_inlet_Ma.txt");
    read_valuepairs_from_file("scramjet_exit_area", "./parameters/scramjet_exit_area.txt");

    read_valuepairs_from_file("farfield_inlet_p_static", "./parameters/farfield_inlet_p_static.txt");
    read_valuepairs_from_file("farfield_inlet_Ma", "./parameters/farfield_inlet_Ma.txt");
    read_valuepairs_from_file("farfield_inlet_T_total", "./parameters/farfield_inlet_T_total.txt");

    cal_nozzle_1D_parameters("turbojet");
    cal_nozzle_1D_parameters("scramjet");
    write_valuepairs_to_file("turbojet_1D_vx_e", "./parameters/turbojet_1D_vx_e.gen");
    write_valuepairs_to_file("scramjet_1D_vx_e", "./parameters/scramjet_1D_vx_e.gen");
    write_valuepairs_to_file("turbojet_1D_er", "./parameters/turbojet_1D_er.gen");
    write_valuepairs_to_file("scramjet_1D_er", "./parameters/scramjet_1D_er.gen");

    const bool enable_1D_exit_area_adjust = std::filesystem::exists("./parameters/enable_1D_exit_area_adjust");
    udf::info("Enable 1D exit area adjust? {}", enable_1D_exit_area_adjust);
    if (enable_1D_exit_area_adjust) {  // must after call cal_nozzle_1D_parameters
        // 调节sj
        auto sj_ers = values["scramjet_1D_er"];
        auto times = timepoints["scramjet_1D_er"];
        [&] {
            const auto& areas = values.at("scramjet_exit_area");
            const real area_max = *std::max_element(areas.begin(), areas.end());
            if (std::abs(area_max - refvalue::sjExitArea) > eps) {
                udf::error("Error refvalue::sjExitArea");
            }
        }();
        for (auto& er : sj_ers) {
            real tmpArea = er * refvalue::throatArea;
            if (tmpArea >= eps && tmpArea <= refvalue::sjExitArea) {
                er = tmpArea;
            } else if (tmpArea > refvalue::sjExitArea + eps) {
                er = refvalue::sjExitArea;
            } else {
                er = 0;
            }
        }
        timepoints.insert_or_assign("scramjet_exit_area", times);
        values.insert_or_assign("scramjet_exit_area", sj_ers);

        // 调节tj
        // do nothing

        // 输出结果便于观察
        write_valuepairs_to_file("turbojet_exit_area", "./parameters/turbojet_exit_area.gen");
        write_valuepairs_to_file("scramjet_exit_area", "./parameters/scramjet_exit_area.gen");
    }
}

DEFINE_PROFILE(scramjet_inlet_T_total, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    const real value = get_bcvalue_by_time("scramjet_inlet_T_total", time);
    face_t f = -1;
    begin_f_loop(f, boundary_thread) if PRINCIPAL_FACE_P (f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = value;
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(scramjet_inlet_p_total, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    const real value = get_bcvalue_by_time("scramjet_inlet_p_total", time);
    face_t f = -1;
    begin_f_loop(f, boundary_thread) if PRINCIPAL_FACE_P (f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = value;
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(scramjet_inlet_p_static, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    const real p_total = get_bcvalue_by_time("scramjet_inlet_p_total", time);
    const real T_total = get_bcvalue_by_time("scramjet_inlet_T_total", time);
    const real Ma = get_bcvalue_by_time("scramjet_inlet_Ma", time);
    const real T_static = hndp::isentropic::cal_T_static_by_Ma(mp, T_total, Ma);
    const real value = hndp::isentropic::cal_p_static_by_T_total_T_static(mp, p_total, T_total, T_static);
    face_t f = -1;
    begin_f_loop(f, boundary_thread) if PRINCIPAL_FACE_P (f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = value;
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(turbojet_inlet_T_total, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    const real value = get_bcvalue_by_time("turbojet_inlet_T_total", time);
    face_t f = -1;
    begin_f_loop(f, boundary_thread) if PRINCIPAL_FACE_P (f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = value;
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(turbojet_inlet_p_total, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    const real value = get_bcvalue_by_time("turbojet_inlet_p_total", time);
    face_t f = -1;
    begin_f_loop(f, boundary_thread) if PRINCIPAL_FACE_P (f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = value;
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(turbojet_inlet_p_static, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    const real p_total = get_bcvalue_by_time("turbojet_inlet_p_total", time);
    const real T_total = get_bcvalue_by_time("turbojet_inlet_T_total", time);
    const real Ma = get_bcvalue_by_time("turbojet_inlet_Ma", time);
    const real T_static = hndp::isentropic::cal_T_static_by_Ma(mp, T_total, Ma);
    const real value = hndp::isentropic::cal_p_static_by_T_total_T_static(mp, p_total, T_total, T_static);
    face_t f = -1;
    begin_f_loop(f, boundary_thread) if PRINCIPAL_FACE_P (f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = value;
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(farfield_inlet_p_static, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    const real value = get_bcvalue_by_time("farfield_inlet_p_static", time);
    face_t f = -1;
    begin_f_loop(f, boundary_thread) if PRINCIPAL_FACE_P (f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = value;
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(farfield_inlet_Ma, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    const real value = get_bcvalue_by_time("farfield_inlet_Ma", time);
    ;
    face_t f = -1;
    begin_f_loop(f, boundary_thread) if PRINCIPAL_FACE_P (f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = value;
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(farfield_inlet_T_total, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    const real value = get_bcvalue_by_time("farfield_inlet_T_total", time);
    face_t f = -1;
    begin_f_loop(f, boundary_thread) if PRINCIPAL_FACE_P (f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = value;
    }
    end_f_loop(f, thread);
}

DEFINE_REPORT_DEFINITION_FN(turbojet_1D_vx_e) {
    const real time = RP_Get_Real("flow-time");
    // const real p_total = get_bcvalue_by_time("turbojet_inlet_p_total", time);
    // const real T_total = get_bcvalue_by_time("turbojet_inlet_T_total", time);
    // const real Ma = get_bcvalue_by_time("turbojet_inlet_Ma", time);
    // const real T_static = hndp::isentropic::cal_T_static_by_Ma(mp, T_total, Ma);
    // const real p_static = hndp::isentropic::cal_p_static_by_T_total_T_static(mp, p_total, T_total, T_static);
    // const real p_ambient = get_bcvalue_by_time("farfield_inlet_p_static", time);
    // const real T_static_e = hndp::isentropic::cal_T_static_by_p_static(mp, T_static, p_static, p_ambient);
    // const real vx_e = hndp::isentropic::cal_V_by_T_total_T_static(mp, T_total, T_static_e);
    const real vx_e = get_bcvalue_by_time("turbojet_1D_vx_e", time);
    return vx_e;
}

DEFINE_REPORT_DEFINITION_FN(scramjet_1D_vx_e) {
    const real time = RP_Get_Real("flow-time");
    // const real p_total = get_bcvalue_by_time("scramjet_inlet_p_total", time);
    // const real T_total = get_bcvalue_by_time("scramjet_inlet_T_total", time);
    // const real Ma = get_bcvalue_by_time("scramjet_inlet_Ma", time);
    // const real T_static = hndp::isentropic::cal_T_static_by_Ma(mp, T_total, Ma);
    // const real p_static = hndp::isentropic::cal_p_static_by_T_total_T_static(mp, p_total, T_total, T_static);
    // const real p_ambient = get_bcvalue_by_time("farfield_inlet_p_static", time);
    // const real T_static_e = hndp::isentropic::cal_T_static_by_p_static(mp, T_static, p_static, p_ambient);
    // const real vx_e = hndp::isentropic::cal_V_by_T_total_T_static(mp, T_total, T_static_e);
    const real vx_e = get_bcvalue_by_time("scramjet_1D_vx_e", time);
    return vx_e;
}

static real cal_wall_Fx(Domain* domain, int wall_id, real p_ambient) {
    real Fx{};
#if !RP_HOST
    Thread* ft = Lookup_Thread(domain, wall_id);
    face_t f;
    begin_f_loop(f, ft) if PRINCIPAL_FACE_P (f, ft) {
        real NV_VEC(area){};
        F_AREA(area, f, ft);
        const real wall_shear_force = -F_STORAGE_R_N3V(f, ft, SV_WALL_SHEAR)[0];  // 求的是x方向的摩擦力
        Fx += (F_P(f, ft) - p_ambient) * area[0] + wall_shear_force;
    }
    end_f_loop(f, ft);
#endif
    Fx = PRF_GRSUM1(Fx);
    node_to_host_real_1(Fx);
    return Fx * refvalue::width;
}

DEFINE_REPORT_DEFINITION_FN(scramjet_Fx) {
    if (!Data_Valid_P()) {
        return 0.;
    }
    real Fpx{};
    Domain* domain = Get_Domain(1);
    const real p_ambient = get_bcvalue_by_time("farfield_inlet_p_static", RP_Get_Real("flow-time"));
    Fpx += cal_wall_Fx(domain, id::sj_wallu_l, p_ambient);
    Fpx += cal_wall_Fx(domain, id::sj_wallu_r, p_ambient);
    Fpx += cal_wall_Fx(domain, id::sj_throatd, p_ambient);
    Fpx += cal_wall_Fx(domain, id::sj_throatu, p_ambient);
    Fpx += cal_wall_Fx(domain, id::splitteru_d, p_ambient);
    Fpx += cal_wall_Fx(domain, id::splitterd_u, p_ambient);
    return Fpx;
}

DEFINE_REPORT_DEFINITION_FN(turbojet_Fx) {
    if (!Data_Valid_P()) {
        return 0.;
    }
    real Fpx{};
    Domain* domain = Get_Domain(1);
    const real p_ambient = get_bcvalue_by_time("farfield_inlet_p_static", RP_Get_Real("flow-time"));
    Fpx += cal_wall_Fx(domain, id::tj_wallu, p_ambient);
    Fpx += cal_wall_Fx(domain, id::sj_wallu_r, p_ambient);
    Fpx += cal_wall_Fx(domain, id::tj_walld, p_ambient);
    Fpx += cal_wall_Fx(domain, id::splitteru_u, p_ambient);
    return Fpx;
}

DEFINE_REPORT_DEFINITION_FN(both_Fx) {
    if (!Data_Valid_P()) {
        return 0.;
    }
    real Fpx{};
    Domain* domain = Get_Domain(1);
    const real p_ambient = get_bcvalue_by_time("farfield_inlet_p_static", RP_Get_Real("flow-time"));
    Fpx += cal_wall_Fx(domain, id::tj_wallu, p_ambient);
    Fpx += cal_wall_Fx(domain, id::tj_walld, p_ambient);
    Fpx += cal_wall_Fx(domain, id::splitteru_u, p_ambient);
    Fpx += cal_wall_Fx(domain, id::sj_wallu_l, p_ambient);
    Fpx += cal_wall_Fx(domain, id::sj_wallu_r, p_ambient);
    Fpx += cal_wall_Fx(domain, id::sj_throatd, p_ambient);
    Fpx += cal_wall_Fx(domain, id::sj_throatu, p_ambient);
    Fpx += cal_wall_Fx(domain, id::splitteru_d, p_ambient);
    Fpx += cal_wall_Fx(domain, id::splitterd_u, p_ambient);
    return Fpx;
}

/**
 * @brief 在所给边界Thread中，根据x坐标大小找到左右两端点
 *
 * @param tf 边界Thread 要求边界上节点的x坐标单值
 * @param NV_VEC 所得到的左端点坐标
 * @param NV_VEC 所得到的右端点坐标
 */
static void cal_origin_and_end(Thread* tf, real NV_VEC(origin), real NV_VEC(end)) {
    // Initialization
    NV_S(origin, =, std::numeric_limits<real>::max());
    NV_S(end, =, std::numeric_limits<real>::min());

#if !RP_HOST
    // Find origin point and end point in each compute node
    face_t f;
    begin_f_loop(f, tf) {
        int n;
        f_node_loop(f, tf, n) {
            Node* v = F_NODE(f, tf, n);
            if (NODE_X(v) < origin[0]) {
                NV_V(origin, =, NODE_COORD(v));
            }
            if (NODE_X(v) > end[0]) {
                NV_V(end, =, NODE_COORD(v));
            }
        }
    }
    end_f_loop(f, tf);

    // Global reduction: get global origin point and end point in node0
    if (!I_AM_NODE_ZERO_P) {
        PRF_CSEND_REAL(node_zero, origin, ND_ND, myid);
        PRF_CSEND_REAL(node_zero, end, ND_ND, myid);
    } else {
        int i;
        compute_node_loop_not_zero(i) {
            real NV_VEC(origin_tmp);
            real NV_VEC(end_tmp);
            PRF_CRECV_REAL(i, origin_tmp, ND_ND, i);
            PRF_CRECV_REAL(i, end_tmp, ND_ND, i);
            if (origin_tmp[0] < origin[0]) {
                NV_V(origin, =, origin_tmp);
            }
            if (end_tmp[0] > end[0]) {
                NV_V(end, =, end_tmp);
            }
        }
    }
#endif  // !RP_HOST

    // Pass global origin point and end point to all nodes
    node_to_host_real(origin, ND_ND);
    host_to_node_real(origin, ND_ND);
    node_to_host_real(end, ND_ND);
    host_to_node_real(end, ND_ND);
}

static inline real cal_distance(const real NV_VEC(p1), const real NV_VEC(p2)) {
#if ND_ND == 2
    return std::sqrt(std::pow(p1[1] - p2[1], 2) + std::pow(p1[0] - p2[0], 2));
#elif ND_ND == 3
    return std::sqrt(std::pow(p1[2] - p2[2], 2) + std::pow(p1[1] - p2[1], 2) + std::pow(p1[0] - p2[0], 2));
#else
#error "Wrong dimension!"
#endif
}

static inline real cal_theta(const real NV_VEC(origin), const real NV_VEC(point)) {
    return std::atan2((point[1] - origin[1]), (point[0] - origin[0]));
#if ND_ND != 2
#error "Only for 2D!"
#endif
}

static inline void rotate_point_to_target_theta(real NV_VEC(point), const real NV_VEC(origin), real target_theta,
                                                real scale = 1.0) {
    const real rho = cal_distance(origin, point) * scale;
    point[0] = origin[0] + rho * std::cos(target_theta);
    point[1] = origin[1] + rho * std::sin(target_theta);
#if ND_ND != 2
#error "Only for 2D!"
#endif
}

[[maybe_unused]]
static inline void rotate_point(real NV_VEC(point), const real NV_VEC(origin), real dtheta, real scale = 1.0) {
    const real rho = cal_distance(origin, point) * scale;
    const real theta = cal_theta(splitterd_d_origin, point) + dtheta;
    point[0] = origin[0] + rho * std::cos(theta);
    point[1] = origin[1] + rho * std::sin(theta);
#if ND_ND != 2
#error "Only for 2D!"
#endif
}

static void set_splitters_info() {
    if (!Data_Valid_P())
        return;
    const real time = RP_Get_Real("flow-time");
    Domain* domain = Get_Domain(1);

    cal_origin_and_end(Lookup_Thread(domain, splitteru_u_id), splitteru_u_origin, splitteru_end);
    real NV_VEC(end){};
    cal_origin_and_end(Lookup_Thread(domain, splitteru_d_id), splitteru_d_origin, end);
    if (end[0] > splitteru_end[0]) {
        udf::info("splitteru_end point changed!!!");
        NV_V(splitteru_end, =, end);
    }
    const real thetau_d = cal_theta(splitteru_d_origin, splitteru_end);
    if (const real theta = get_bcvalue_by_time("thetau_d", time); std::abs(thetau_d - theta) > util::deg2rad(0.1)) {
        udf::info(
            "thetau_d was not initialized correctly! "
            "Better value at {} s is {} degree({}). Current setting is {} degree({})",
            time, util::rad2deg(thetau_d), thetau_d, util::rad2deg(theta), theta);
    }

    cal_origin_and_end(Lookup_Thread(domain, splitterd_u_id), splitterd_u_origin, splitterd_end);
    cal_origin_and_end(Lookup_Thread(domain, splitterd_d_id), splitterd_d_origin, end);
    if (end[0] > splitterd_end[0]) {
        udf::info("splitterd_end point changed!!!");
        NV_V(splitterd_end, =, end);
    }
    const real thetad_u = cal_theta(splitterd_u_origin, splitterd_end);
    if (const real theta = get_bcvalue_by_time("thetad_u", time); std::abs(thetad_u - theta) > util::deg2rad(0.1)) {
        udf::info(
            "thetad_u was not initialized correctly! "
            "Better value at {} s is {} degree({}). Current setting {} degree({})",
            time, util::rad2deg(thetad_u), thetad_u, util::rad2deg(theta), theta);
    }

#if RP_HOST
    udf::println("origin point of splitteru_d is ({},{})", splitteru_d_origin[0], splitteru_d_origin[1]);
    udf::println("end point of splitteru is ({},{})", splitteru_end[0], splitteru_end[1]);
    udf::println("origin point of splitteru_u is ({},{})", splitteru_u_origin[0], splitteru_u_origin[1]);
    udf::println("end point of splitteru is ({},{})", splitteru_end[0], splitteru_end[1]);
    udf::println("thetau_d is {} degree", util::rad2deg(thetau_d));

    udf::println("origin point of splitterd_u is ({},{})", splitterd_u_origin[0], splitterd_u_origin[1]);
    udf::println("end point of splitterd is ({},{})", splitterd_end[0], splitterd_end[1]);
    udf::println("origin point of splitterd_d is ({},{})", splitterd_d_origin[0], splitterd_d_origin[1]);
    udf::println("end point of splitterd is ({},{})", splitterd_end[0], splitterd_end[1]);
    udf::println("thetad_u is {} degree", util::rad2deg(thetad_u));
#endif
}

/**
 * @brief 根据出口面积设置角度
 *
 * @param key_area 喉部面积
 * @param key_theta 板子角度
 */
static void set_thetas_by_exit_area(const std::string& key_area, const std::string& key_theta) {
    const auto& times = timepoints[key_area];
    const auto& areas = values[key_area];
    auto& thetas = values[key_theta];
    if (thetas.size() != 3 || times.size() < 2 || times.size() != areas.size()) {
        udf::error("{} {} {} {} {}", key_theta, thetas.size(), key_area, times.size(), areas.size());
    }
    const real theta_init = thetas[0];
    const real theta_mid = thetas[1];
    const real theta_final = thetas[2];
    thetas.resize(times.size());

    const real area_max = *std::max_element(areas.begin(), areas.end());

    if (theta_init > theta_final) {                  // thetad_u
        for (size_t i = 0; i < times.size(); ++i) {  // 调出口面积
            thetas[i] = std::asin(std::sin(theta_init) +
                                  (areas[i] / area_max) * (std::sin(theta_final) - std::sin(theta_init)));
        }

    } else if (theta_init < theta_final) {           // thetau_d
        for (size_t i = 0; i < times.size(); ++i) {  // 调出口面积
            thetas[i] = std::asin(std::sin(theta_init) +
                                  (1. - areas[i] / area_max) * (std::sin(theta_final) - std::sin(theta_init)));
        }
    } else {
        udf::error("thetas.front() and thetas.back() cannot be the same");
    }

    timepoints[key_theta] = times;
}

static void set_thetas() {
    read_valuepairs_from_file("thetau_d", "./parameters/thetau_d.txt", util::deg2rad_scale_factor);
    read_valuepairs_from_file("thetad_u", "./parameters/thetad_u.txt", util::deg2rad_scale_factor);
    // now used
    set_thetas_by_exit_area("turbojet_exit_area", "thetau_d");
    set_thetas_by_exit_area("scramjet_exit_area", "thetad_u");
    // save state log
    write_valuepairs_to_file("thetau_d", "./parameters/thetau_d.gen", util::rad2deg_scale_factor);
    write_valuepairs_to_file("thetad_u", "./parameters/thetad_u.gen", util::rad2deg_scale_factor);
}

DEFINE_EXECUTE_ON_LOADING(initialize_splitters_info, libname) {
    // Don't remove!
    set_values_and_timepoints();
    set_thetas();
    set_splitters_info();
}

DEFINE_ON_DEMAND(get_splitters_end_points) {
    set_values_and_timepoints();
    set_thetas();
    set_splitters_info();

#if RP_HOST
    for (const auto& [key, ts] : timepoints) {
        udf::print("{}: ", key);
        const auto& vs = values[key];
        if (ts.size() != vs.size()) {
            udf::error("Inconsistent size of {}", key);
        }
        for (size_t i = 0; i < ts.size(); ++i) {
            udf::print("({},{}), ", ts[i], vs[i]);
        }
        udf::println("");
    }
#endif
}

DEFINE_ADJUST(update_splitters_info, domain) {
    if (!Data_Valid_P())
        return;

    cal_origin_and_end(Lookup_Thread(domain, splitteru_u_id), splitteru_u_origin, splitteru_end);
    real NV_VEC(end){};
    cal_origin_and_end(Lookup_Thread(domain, splitteru_d_id), splitteru_d_origin, end);
    if (end[0] > splitteru_end[0]) {
        NV_V(splitteru_end, =, end);
    }

    cal_origin_and_end(Lookup_Thread(domain, splitterd_u_id), splitterd_u_origin, splitterd_end);
    cal_origin_and_end(Lookup_Thread(domain, splitterd_d_id), splitterd_d_origin, end);
    if (end[0] > splitterd_end[0]) {
        NV_V(splitterd_end, =, end);
    }

    // udf::println("DEFINE_ADJUST called!");
}

DEFINE_GRID_MOTION(splitteru_u, d, dt, time, dtime) {
    if (!Data_Valid_P())
        return;
    Thread* tf = DT_THREAD(dt);
    SET_DEFORMING_THREAD_FLAG(THREAD_T0(tf));

    real NV_VEC(end){};
    NV_V(end, =, splitteru_end);

    // theta_u的目标角度
    const real theta_target = get_bcvalue_by_time("thetau_d", time);
    // 上一步结束后的rho
    const real rho_prev = cal_distance(splitteru_u_origin, end);
    // 上一步结束后splitterd_d的theta
    const real theta_prev = cal_theta(splitteru_u_origin, end);
    // 理论上当前时间步结束时的端点坐标
    rotate_point_to_target_theta(end, splitteru_d_origin, theta_target);
    // 这一步结束后的rho
    const real rho_cur = cal_distance(splitteru_u_origin, end);
    // 这一步结束后splitterd_d的theta
    const real theta_cur = cal_theta(splitteru_u_origin, end);
    const real scale = rho_cur / rho_prev;
    const real dtheta = theta_cur - theta_prev;

    face_t f;
    begin_f_loop(f, tf) {
        int n;
        f_node_loop(f, tf, n) {
            Node* v = F_NODE(f, tf, n);
            if (NODE_POS_NEED_UPDATE(v)) {
                NODE_POS_UPDATED(v);
                // update node position
                const real theta = cal_theta(splitteru_u_origin, NODE_COORD(v));
                rotate_point_to_target_theta(NODE_COORD(v), splitteru_u_origin, theta + dtheta, scale);
            }
            // udf::info("u_u end: ({},{})", NODE_X(v), NODE_Y(v));
        }
    }
    end_f_loop(f, tf);
}

DEFINE_GRID_MOTION(splitteru_d, d, dt, time, dtime) {
    if (!Data_Valid_P())
        return;
    Thread* tf = DT_THREAD(dt);
    SET_DEFORMING_THREAD_FLAG(THREAD_T0(tf));

    const real theta_target = get_bcvalue_by_time("thetau_d", time);
    const real theta_prev = cal_theta(splitteru_d_origin, splitteru_end);
    const real dtheta = theta_target - theta_prev;

    face_t f;
    begin_f_loop(f, tf) {
        int n;
        f_node_loop(f, tf, n) {
            Node* v = F_NODE(f, tf, n);
            if (NODE_POS_NEED_UPDATE(v)) {
                NODE_POS_UPDATED(v);
                // update node position
                const real theta = cal_theta(splitteru_d_origin, NODE_COORD(v));
                rotate_point_to_target_theta(NODE_COORD(v), splitteru_d_origin, theta + dtheta);
            }
            // udf::info("u_d end: ({},{})", NODE_X(v), NODE_Y(v));
        }
    }
    end_f_loop(f, tf);
}

DEFINE_GRID_MOTION(splitterd_u, d, dt, time, dtime) {
    if (!Data_Valid_P())
        return;
    Thread* tf = DT_THREAD(dt);
    SET_DEFORMING_THREAD_FLAG(THREAD_T0(tf));

    const real theta_target = get_bcvalue_by_time("thetad_u", time);
    const real theta_prev = cal_theta(splitterd_u_origin, splitterd_end);
    const real dtheta = theta_target - theta_prev;

    face_t f;
    begin_f_loop(f, tf) {
        int n;
        f_node_loop(f, tf, n) {
            Node* v = F_NODE(f, tf, n);
            if (NODE_POS_NEED_UPDATE(v)) {
                NODE_POS_UPDATED(v);
                // update node position
                const real theta = cal_theta(splitterd_u_origin, NODE_COORD(v));
                rotate_point_to_target_theta(NODE_COORD(v), splitterd_u_origin, theta + dtheta);
            }
            // udf::info("d_u end: ({},{})", NODE_X(v), NODE_Y(v));
        }
    }
    end_f_loop(f, tf);
}

DEFINE_GRID_MOTION(splitterd_d, d, dt, time, dtime) {
    if (!Data_Valid_P())
        return;
    Thread* tf = DT_THREAD(dt);
    SET_DEFORMING_THREAD_FLAG(THREAD_T0(tf));

    real NV_VEC(end){};
    NV_V(end, =, splitterd_end);

    // theta_d的目标角度
    const real theta_target = get_bcvalue_by_time("thetad_u", time);
    // 上一步结束后的rho
    const real rho_prev = cal_distance(splitterd_d_origin, end);
    // 上一步结束后splitterd_d的theta
    const real theta_prev = cal_theta(splitterd_d_origin, end);
    // 理论上当前时间步结束时的端点坐标
    rotate_point_to_target_theta(end, splitterd_u_origin, theta_target);
    // 这一步结束后的rho
    const real rho_cur = cal_distance(splitterd_d_origin, end);
    // 这一步结束后splitterd_d的theta
    const real theta_cur = cal_theta(splitterd_d_origin, end);
    const real scale = rho_cur / rho_prev;
    const real dtheta = theta_cur - theta_prev;

    face_t f;
    begin_f_loop(f, tf) {
        int n;
        f_node_loop(f, tf, n) {
            Node* v = F_NODE(f, tf, n);
            if (NODE_POS_NEED_UPDATE(v)) {
                NODE_POS_UPDATED(v);
                // update node position
                const real theta = cal_theta(splitterd_d_origin, NODE_COORD(v));
                rotate_point_to_target_theta(NODE_COORD(v), splitterd_d_origin, theta + dtheta, scale);
            }
            // udf::info("d_d end: ({},{})", NODE_X(v), NODE_Y(v));
        }
    }
    end_f_loop(f, tf);
}

DEFINE_SPECIFIC_HEAT(nasa9piecewisepolynomial, T, Tref, h, yi) {
    static const hndp::aerodynamics::MaterialProperty mp =
        hndp::aerodynamics::MaterialProperty::air_nasa9piecewise_polynomial();
    const real cp = mp.Cp(T);
    *h = mp.enthalpy(T, Tref);
    return cp;
}
