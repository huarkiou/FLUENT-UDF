/* clang-format off */
#include <string>
extern "C"
{
#include "udf.h"
#include "hdfio.h"
#include "dpm.h"
}
/* clang-format on */
#include "utilities.hpp"
constexpr int BOUNDARY_ID = 1;

/* clang-format off */
int a =3; double b =2;DEFINE_EXECUTE_ON_LOADING(on_loading, libname) {Message(" \n");double a[3]={0,};}
/* clang-format on */

DEFINE_ON_DEMAND(on_demand) {
#if RP_HOST
    hku::println("no cell or face");
#endif

#if !RP_HOST

    if (!Data_Valid_P()) return;
    Domain *domain = Get_Domain(1);

    Thread *ct;
    thread_loop_c(ct, domain) {
        hku::println("cell name is {}", ct->name);
    }
    Thread *ft;
    thread_loop_f(ft, domain) {
        hku::println("face name is {}", ft->name);
    }

    Thread *ft1 = Lookup_Thread(domain, BOUNDARY_ID);
    face_t f;
    begin_f_loop(f, ft1) {}
    end_f_loop(f, tf1);
    cell_t c;
    begin_c_loop(c, ft1) {}
    end_c_loop(c, tf1);

#endif
}

DEFINE_ZONE_MOTION(zone_motion, omega, axis, origin, velocity, time, dtime) {
    velocity[0] = 0;
    velocity[1] = 0;
}

DEFINE_CG_MOTION(cg_motion, dt, velocity, omega, time, dtime) {
    velocity[0] = 0;
    velocity[1] = 0;
    omega[2] = 0;
}

DEFINE_EXECUTE_AT_END(exec_at_end) {
    if (!Data_Valid_P()) return;
    Domain *domain = Get_Domain(1);                    // fluid domain pointer
    Thread *tf1 = Lookup_Thread(domain, BOUNDARY_ID);  // boundary pointer
    // calculate the force applied
    real x_cg[3], force[3], moment[3];
    N3V_D(x_cg, =, 0, 0, 0);
    Compute_Force_And_Moment(domain, tf1, x_cg, force, moment, TRUE);

    real time = RP_Get_Real("flow-time");          // time(s)
    real d_t = RP_Get_Real("physical-time-step");  // timestep size(s)
}

std::string DATA_PATH_KEY{"/params"};
// write data
DEFINE_RW_HDF_FILE(writeDataToHDF, filename) {
#if RP_HOST
    Message("write data to hdf5:\n");
    Message("\n");
#endif
}

// read datas
DEFINE_RW_HDF_FILE(readDataFromHDF, filename) {
#if RP_HOST
#endif
}

// user-defined parameters to be reported
DEFINE_REPORT_DEFINITION_FN(rpd1) {
    return 31;
}
DEFINE_REPORT_DEFINITION_FN(rpd2) {
    return -3;
}
DEFINE_REPORT_DEFINITION_FN(rpd3) {
    return 99;
}
DEFINE_REPORT_DEFINITION_FN(rpd4) {
    return 10;
}

DEFINE_REPORT_DEFINITION_FN(rpd5) {
    return 8;
}
DEFINE_REPORT_DEFINITION_FN(rpd6) {
    return 6;
}
DEFINE_REPORT_DEFINITION_FN(rpd7) {
    return 5;
}

DEFINE_REPORT_DEFINITION_FN(rpd8) {
    return 4;
}
DEFINE_REPORT_DEFINITION_FN(rpd9) {
    return 3;
}
DEFINE_REPORT_DEFINITION_FN(rpd10) {
    return 2;
}
DEFINE_REPORT_DEFINITION_FN(rpd11) {
    return 1;
}

static real velocity_in = 0.0;

DEFINE_PROFILE(inlet_x_velocity, thread, position) {
    face_t f;
    real t, u;
    t = RP_Get_Real("flow-time");
    const double velocities[] = {0.,      0.00762, 0.01524, 0.02286, 0.03048, 0.0381,  0.04572, 0.05334, 0.06096,
                                 0.06858, 0.0762,  0.08382, 0.09144, 0.09906, 0.10668, 0.1143,  0.12192, 0.12954,
                                 0.13716, 0.14478, 0.1524,  0.16002, 0.16764, 0.17526, 0.18288, 0.1905};
    int index = (int)(t) / 25;
    double rate = ((int)(t) % 25 - 20) / 5.0;
    begin_f_loop(f, thread) {
        u = velocities[index];
        if (rate > 0) {
            u += (velocities[index + 1] - velocities[index]) * rate;
        }
        F_PROFILE(f, thread, position) = u;
    }
    end_f_loop(f, thread);
    velocity_in = u;
#if RP_HOST
    Message("u_in = %.4lf\n", u);
#endif
}

DEFINE_REPORT_DEFINITION_FN(velocity_inlet) {
    return velocity_in;
}
