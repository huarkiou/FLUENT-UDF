extern "C" {
#include "udf.h"
}

#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include "utilities.hpp"

real NV_VEC(splitteru_d_origin){};
const int splitteru_d_id = 8;
real NV_VEC(splitteru_u_origin){};
const int splitteru_u_id = 9;
real NV_VEC(splitteru_end){};  //< 上一时间步结束后当前时间步开始时splitteru端点坐标

real NV_VEC(splitterd_u_origin){};
const int splitterd_u_id = 6;
real NV_VEC(splitterd_d_origin){};
const int splitterd_d_id = 7;
real NV_VEC(splitterd_end){};  //< 上一时间步结束后当前时间步开始时splitterd端点坐标

/**
 *  splitteru_u 0°                 ~ 18.4°
 *  splitteru_d 2.095957260605246° ~ ?
 *  splitterd_u 39.58885965959722° ~ 0°
 *  splitterd_d 41.5662°           ~ ?
 */

std::unordered_map<std::string, std::vector<real>> timepoints{
    {"turbojet_inlet_T_total",
     {0.00, 5.01, 5.14, 5.59, 5.83, 6.98, 7.11, 7.61, 7.83, 8.95, 9.11, 9.53, 12.01, 14.00, 15.52, 17.01, 24.95}},
    {"turbojet_inlet_p_total", {0.00,  5.10,  5.62,  7.10,  7.54,  8.95,  9.47,  12.20, 14.10, 14.90, 15.30, 15.80,
                                16.30, 16.70, 17.10, 17.80, 20.00, 20.70, 20.90, 21.10, 21.20, 21.40, 21.60, 21.80,
                                22.00, 22.00, 22.30, 22.50, 22.60, 23.00, 23.50, 24.10, 24.60, 24.90, 24.90}},
    {"turbojet_inlet_p_static", {0.00,  5.10,  5.62,  7.10,  7.54,  8.95,  9.47,  12.20, 14.10, 14.90, 15.30, 15.80,
                                 16.30, 16.70, 17.10, 17.80, 20.00, 20.70, 20.90, 21.10, 21.20, 21.40, 21.60, 21.80,
                                 22.00, 22.00, 22.30, 22.50, 22.60, 23.00, 23.50, 24.10, 24.60, 24.90, 24.90}},
    {"scramjet_inlet_T_total", {0.00, 4.93, 5.14, 5.56, 5.88, 7.00, 7.59, 7.91, 8.97, 9.56, 9.85, 24.92}},
    {"scramjet_inlet_p_total", {0.00, 0.07, 0.22, 0.37, 0.44, 0.52, 0.67, 0.81, 1.18, 1.55, 1.92, 2.22, 2.59,
                                2.96, 3.25, 3.48, 3.62, 3.85, 3.92, 3.99, 4.07, 4.14, 4.22, 4.29, 4.36, 4.51,
                                4.96, 5.03, 5.25, 5.40, 5.55, 5.70, 7.10, 7.54, 9.02, 9.54, 9.84, 9.91, 25.00}},
    {"scramjet_inlet_p_static", {0.00, 0.07, 0.22, 0.37, 0.44, 0.52, 0.67, 0.81, 1.18, 1.55, 1.92, 2.22, 2.59,
                                 2.96, 3.25, 3.48, 3.62, 3.85, 3.92, 3.99, 4.07, 4.14, 4.22, 4.29, 4.36, 4.51,
                                 4.96, 5.03, 5.25, 5.40, 5.55, 5.70, 7.10, 7.54, 9.02, 9.54, 9.84, 9.91, 25.00}},
    {"farfield_inlet_p_static", {0., 25.}},
    {"farfield_inlet_Ma", {0., 25.}},
    {"farfield_inlet_T_total", {0., 25.}},
    {"thetau_d", {0., 25.}},
    {"thetad_u", {0., 25.}},
};

std::unordered_map<std::string, std::vector<real>> values{
    {"turbojet_inlet_T_total",
     {2160.05, 2160.05, 2123.54, 1731.08, 1700.66, 1700.66, 1670.24, 1323.41, 1299.07, 1299.07, 1274.74, 937.04, 937.04,
      876.19, 842.72, 812.30, 812.30}},
    {"turbojet_inlet_p_total",
     {433967.86, 433967.86, 443401.95, 443401.95, 445974.88, 445974.88, 452836.03, 452836.03, 430537.29,
      424533.78, 421103.20, 417672.63, 415099.70, 411669.12, 408238.54, 405665.61, 405665.61, 433967.86,
      439971.37, 443401.95, 445974.88, 445974.88, 443401.95, 436540.80, 430537.29, 421103.20, 389370.38,
      361068.12, 345630.53, 254720.27, 188681.68, 144941.84, 122643.09, 109778.43, 109778.43}},
    {"turbojet_inlet_p_static",
     {384003.11, 384003.11, 392351.01, 392351.01, 394627.70, 394627.70, 400698.90, 400698.90, 380967.51,
      375655.22, 372619.62, 369584.02, 367307.32, 364271.73, 361236.13, 358959.43, 358959.43, 384003.11,
      389315.41, 392351.01, 394627.70, 394627.70, 392351.01, 386279.81, 380967.51, 372619.62, 344540.34,
      319496.66, 305836.47, 225393.13, 166957.87, 128254.00, 108522.62, 97139.13,  97139.13}},
    {"scramjet_inlet_T_total",
     {806.22, 806.22, 851.85, 1265.61, 1296.03, 1296.03, 1627.65, 1655.03, 1655.03, 2001.85, 2035.32, 2035.32}},
    {"scramjet_inlet_p_total",
     {97771.42,  97771.42,  100344.35, 103774.92, 107205.50, 109778.43, 113209.01, 119212.52, 132077.18, 150945.34,
      173244.09, 201546.34, 241855.61, 311324.77, 398804.46, 436540.80, 458839.54, 481138.28, 487141.79, 493145.30,
      496575.88, 500006.45, 502579.38, 506009.96, 509440.54, 512013.47, 512013.47, 512013.47, 506009.96, 500006.45,
      493145.30, 490572.37, 490572.37, 471704.20, 471704.20, 443401.95, 443401.95, 439971.37, 439971.37}},
    {"scramjet_inlet_p_static",
     {54782.01,  54782.01,  56223.65,  58145.82,  60068.00,  61509.63,  63431.80,  66795.61,  74003.77,  84575.74,
      97069.88,  112927.83, 135513.40, 174437.46, 223452.95, 244596.88, 257091.03, 269585.17, 272948.98, 276312.79,
      278234.96, 280157.14, 281598.77, 283520.95, 285443.12, 286884.75, 286884.75, 286884.75, 283520.95, 280157.14,
      276312.79, 274871.15, 274871.15, 264299.19, 264299.19, 248441.23, 248441.23, 246519.06, 246519.06}},
    {"farfield_inlet_p_static", {8576.44, 8576.44}},
    {"farfield_inlet_Ma", {3.75, 3.75}},
    {"farfield_inlet_T_total", {216.65, 216.65}},
    {"thetau_d", hku::deg2rad({2.095957260605246, 20.5})},
    {"thetad_u", hku::deg2rad({39.58885965959722, 0.})},
};

double interpolate_value(const std::vector<real>& v1, const std::vector<real>& v2, real v) {
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

double get_bcvalue_by_time(const std::string& name, real time) {
    return interpolate_value(timepoints[name], values[name], time);
}

DEFINE_PROFILE(scramjet_inlet_T_total, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    face_t f = -1;
    begin_f_loop(f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = get_bcvalue_by_time("scramjet_inlet_T_total", time);
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(scramjet_inlet_p_total, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    face_t f = -1;
    begin_f_loop(f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = get_bcvalue_by_time("scramjet_inlet_p_total", time);
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(scramjet_inlet_p_static, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    face_t f = -1;
    begin_f_loop(f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = get_bcvalue_by_time("scramjet_inlet_p_static", time);
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(turbojet_inlet_T_total, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    face_t f = -1;
    begin_f_loop(f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = get_bcvalue_by_time("turbojet_inlet_T_total", time);
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(turbojet_inlet_p_total, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    face_t f = -1;
    begin_f_loop(f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = get_bcvalue_by_time("turbojet_inlet_p_total", time);
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(turbojet_inlet_p_static, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    face_t f = -1;
    begin_f_loop(f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = get_bcvalue_by_time("turbojet_inlet_p_static", time);
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(farfield_inlet_p_static, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    face_t f = -1;
    begin_f_loop(f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = get_bcvalue_by_time("farfield_inlet_p_static", time);
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(farfield_inlet_Ma, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    face_t f = -1;
    begin_f_loop(f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = get_bcvalue_by_time("farfield_inlet_Ma", time);
    }
    end_f_loop(f, thread);
}

DEFINE_PROFILE(farfield_inlet_T_total, boundary_thread, variable_index) {
    const real time = RP_Get_Real("flow-time");
    face_t f = -1;
    begin_f_loop(f, boundary_thread) {
        F_PROFILE(f, boundary_thread, variable_index) = get_bcvalue_by_time("farfield_inlet_T_total", time);
    }
    end_f_loop(f, thread);
}

DEFINE_CG_MOTION(splitteru_cg_test, dt, vel, omega, time, dtime) {
    // origin (52.769095346 mm, 57.404978300 mm, 0)
    omega[2] = 0.1;
}

DEFINE_CG_MOTION(splitterd_cg_test, dt, vel, omega, time, dtime) {
    // origin (0, -1.98 mm, 0)
    omega[2] = -0.1;
}

/**
 * @brief 在所给边界Thread中，根据x坐标大小找到左右两端点
 *
 * @param tf 边界Thread 要求边界上节点的x坐标单值
 * @param NV_VEC 所得到的左端点坐标
 * @param NV_VEC 所得到的右端点坐标
 */
void cal_origin_and_end(Thread* tf, real NV_VEC(origin), real NV_VEC(end)) {
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

inline double cal_distance(const real NV_VEC(p1), const real NV_VEC(p2)) {
#if ND_ND == 2
    return std::sqrt(std::pow(p1[1] - p2[1], 2) + std::pow(p1[0] - p2[0], 2));
#elif ND_ND == 3
    return std::sqrt(std::pow(p1[2] - p2[2], 2) + std::pow(p1[1] - p2[1], 2) + std::pow(p1[0] - p2[0], 2));
#else
#error "Wrong dimension!"
#endif
}

inline double cal_theta(const real NV_VEC(origin), const real NV_VEC(point)) {
    return std::atan2((point[1] - origin[1]), (point[0] - origin[0]));
#if ND_ND != 2
#error "Only for 2D!"
#endif
}

inline void rotate_point_to_target_theta(real NV_VEC(point), const real NV_VEC(origin), double target_theta,
                                         double scale = 1.0) {
    const double rho = cal_distance(origin, point) * scale;
    point[0] = origin[0] + rho * std::cos(target_theta);
    point[1] = origin[1] + rho * std::sin(target_theta);
#if ND_ND != 2
#error "Only for 2D!"
#endif
}

inline void rotate_point(real NV_VEC(point), const real NV_VEC(origin), double dtheta, double scale = 1.0) {
    const double rho = cal_distance(origin, point) * scale;
    const double theta = cal_theta(splitterd_d_origin, point) + dtheta;
    point[0] = origin[0] + rho * std::cos(theta);
    point[1] = origin[1] + rho * std::sin(theta);
#if ND_ND != 2
#error "Only for 2D!"
#endif
}

DEFINE_ON_DEMAND(get_splitters_end_points) {
    if (!Data_Valid_P()) return;

    Domain* domain = Get_Domain(1);

    cal_origin_and_end(Lookup_Thread(domain, splitteru_u_id), splitteru_u_origin, splitteru_end);
    real NV_VEC(end){};
    cal_origin_and_end(Lookup_Thread(domain, splitteru_d_id), splitteru_d_origin, end);
    if (end[0] > splitteru_end[0]) {
        NV_V(splitteru_end, =, end);
    }
    const double thetau_d = cal_theta(splitteru_d_origin, splitteru_end);
    if (std::abs(thetau_d - get_bcvalue_by_time("thetau_d", 0)) > hku::deg2rad(0.1)) {
        hku::println("Error: thetau_d was not initialized correctly! Better value is {} degree", thetau_d);
    }

    cal_origin_and_end(Lookup_Thread(domain, splitterd_u_id), splitterd_u_origin, splitterd_end);
    cal_origin_and_end(Lookup_Thread(domain, splitterd_d_id), splitterd_d_origin, end);
    if (end[0] > splitterd_end[0]) {
        NV_V(splitterd_end, =, end);
    }
    const double thetad_u = cal_theta(splitterd_u_origin, splitterd_end);
    if (std::abs(thetad_u - get_bcvalue_by_time("thetad_u", 0)) > hku::deg2rad(0.1)) {
        hku::println("Error: thetad_u was not initialized correctly! Better value is {} degree", thetad_u);
    }

#if RP_HOST
    hku::println("origin point of splitteru_d is ({},{})", splitteru_d_origin[0], splitteru_d_origin[1]);
    hku::println("end point of splitteru is ({},{})", splitteru_end[0], splitteru_end[1]);
    hku::println("origin point of splitteru_u is ({},{})", splitteru_u_origin[0], splitteru_u_origin[1]);
    hku::println("end point of splitteru is ({},{})", splitteru_end[0], splitteru_end[1]);
    hku::println("thetau_d is {} degree", hku::rad2deg(thetau_d));

    hku::println("origin point of splitterd_u is ({},{})", splitterd_u_origin[0], splitterd_u_origin[1]);
    hku::println("end point of splitterd is ({},{})", splitterd_end[0], splitterd_end[1]);
    hku::println("origin point of splitterd_d is ({},{})", splitterd_d_origin[0], splitterd_d_origin[1]);
    hku::println("end point of splitterd is ({},{})", splitterd_end[0], splitterd_end[1]);
    hku::println("thetad_u is {} degree", hku::rad2deg(thetad_u));
#endif
}

DEFINE_ADJUST(update_splitters_info, domain) {
    if (!Data_Valid_P()) return;

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

    // hku::println("DEFINE_ADJUST called!");
}

DEFINE_GRID_MOTION(splitteru_u, d, dt, time, dtime) {
    if (!Data_Valid_P()) return;
    Thread* tf = DT_THREAD(dt);
    SET_DEFORMING_THREAD_FLAG(THREAD_T0(tf));

    real NV_VEC(end){};
    NV_V(end, =, splitteru_end);

    // theta_u的目标角度
    const double theta_target = get_bcvalue_by_time("thetau_d", time);
    // 上一步结束后的rho
    const double rho_prev = cal_distance(splitteru_u_origin, end);
    // 上一步结束后splitterd_d的theta
    const double theta_prev = cal_theta(splitteru_u_origin, end);
    // 理论上当前时间步结束时的端点坐标
    rotate_point_to_target_theta(end, splitteru_d_origin, theta_target);
    // 这一步结束后的rho
    const double rho_cur = cal_distance(splitteru_u_origin, end);
    // 这一步结束后splitterd_d的theta
    const double theta_cur = cal_theta(splitteru_u_origin, end);
    const double scale = rho_cur / rho_prev;
    const double dtheta = theta_cur - theta_prev;

    face_t f;
    begin_f_loop(f, tf) {
        int n;
        f_node_loop(f, tf, n) {
            Node* v = F_NODE(f, tf, n);
            if (NODE_POS_NEED_UPDATE(v)) {
                NODE_POS_UPDATED(v);
                // update node position
                const double theta = cal_theta(splitteru_u_origin, NODE_COORD(v));
                rotate_point_to_target_theta(NODE_COORD(v), splitteru_u_origin, theta + dtheta, scale);
            }
            // hku::println("u_u end: ({},{})", NODE_X(v), NODE_Y(v));
        }
    }
    end_f_loop(f, tf);
}

DEFINE_GRID_MOTION(splitteru_d, d, dt, time, dtime) {
    if (!Data_Valid_P()) return;
    Thread* tf = DT_THREAD(dt);
    SET_DEFORMING_THREAD_FLAG(THREAD_T0(tf));

    const double theta_target = get_bcvalue_by_time("thetau_d", time);
    const double theta_prev = cal_theta(splitteru_d_origin, splitteru_end);
    const double dtheta = theta_target - theta_prev;

    face_t f;
    begin_f_loop(f, tf) {
        int n;
        f_node_loop(f, tf, n) {
            Node* v = F_NODE(f, tf, n);
            if (NODE_POS_NEED_UPDATE(v)) {
                NODE_POS_UPDATED(v);
                // update node position
                const double theta = cal_theta(splitteru_d_origin, NODE_COORD(v));
                rotate_point_to_target_theta(NODE_COORD(v), splitteru_d_origin, theta + dtheta);
            }
            // hku::println("u_d end: ({},{})", NODE_X(v), NODE_Y(v));
        }
    }
    end_f_loop(f, tf);
}

DEFINE_GRID_MOTION(splitterd_u, d, dt, time, dtime) {
    if (!Data_Valid_P()) return;
    Thread* tf = DT_THREAD(dt);
    SET_DEFORMING_THREAD_FLAG(THREAD_T0(tf));

    const double theta_target = get_bcvalue_by_time("thetad_u", time);
    const double theta_prev = cal_theta(splitterd_u_origin, splitterd_end);
    const double dtheta = theta_target - theta_prev;

    face_t f;
    begin_f_loop(f, tf) {
        int n;
        f_node_loop(f, tf, n) {
            Node* v = F_NODE(f, tf, n);
            if (NODE_POS_NEED_UPDATE(v)) {
                NODE_POS_UPDATED(v);
                // update node position
                const double theta = cal_theta(splitterd_u_origin, NODE_COORD(v));
                rotate_point_to_target_theta(NODE_COORD(v), splitterd_u_origin, theta + dtheta);
            }
            // hku::println("d_u end: ({},{})", NODE_X(v), NODE_Y(v));
        }
    }
    end_f_loop(f, tf);
}

DEFINE_GRID_MOTION(splitterd_d, d, dt, time, dtime) {
    if (!Data_Valid_P()) return;
    Thread* tf = DT_THREAD(dt);
    SET_DEFORMING_THREAD_FLAG(THREAD_T0(tf));

    real NV_VEC(end){};
    NV_V(end, =, splitterd_end);

    // theta_d的目标角度
    const double theta_target = get_bcvalue_by_time("thetad_u", time);
    // 上一步结束后的rho
    const double rho_prev = cal_distance(splitterd_d_origin, end);
    // 上一步结束后splitterd_d的theta
    const double theta_prev = cal_theta(splitterd_d_origin, end);
    // 理论上当前时间步结束时的端点坐标
    rotate_point_to_target_theta(end, splitterd_u_origin, theta_target);
    // 这一步结束后的rho
    const double rho_cur = cal_distance(splitterd_d_origin, end);
    // 这一步结束后splitterd_d的theta
    const double theta_cur = cal_theta(splitterd_d_origin, end);
    const double scale = rho_cur / rho_prev;
    const double dtheta = theta_cur - theta_prev;

    face_t f;
    begin_f_loop(f, tf) {
        int n;
        f_node_loop(f, tf, n) {
            Node* v = F_NODE(f, tf, n);
            if (NODE_POS_NEED_UPDATE(v)) {
                NODE_POS_UPDATED(v);
                // update node position
                const double theta = cal_theta(splitterd_d_origin, NODE_COORD(v));
                rotate_point_to_target_theta(NODE_COORD(v), splitterd_d_origin, theta + dtheta, scale);
            }
            // hku::println("d_d end: ({},{})", NODE_X(v), NODE_Y(v));
        }
    }
    end_f_loop(f, tf);
}
