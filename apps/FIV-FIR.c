/*
    2D xy-direction vibration and rotation in xy-plane --- a UDF implementation of linear mass-damping-stiffness model
    author: huarkiou
    first created: 2022-03-07
    last modified: 2023-09-07
 */

// clang-format off
#include "udf.h"
#include "hdfio.h"
// clang-format on
#include <math.h>
#include <string.h>

// FSI boundary ID
#define BOUNDARY_ZONE_ID 20
// m - mass(kg)
#define MASS_Y 2.165757
// k - spring stiffness(N/m)
#define STIFFNESS_Y 7.695059
// c - damping(N・s/m)
#define DAMPING_Y 0.024494
// mx - mass(kg)
#define MASS_X MASS_Y
// kx - spring stiffness(N/m)
#define STIFFNESS_X 27.779165
// cx - damping(N・s/m)
#define DAMPING_X 0.046539
// D - character length
#define LENGTH 0.050
// Jz - moment of inertia (of mass center)
#define MOMENT_INERTIA (MASS_Y * (LENGTH * LENGTH) / 8)

// 自由度数量 （y-位移，y-速度, z-转角，z-角速度, x-位移, x-速度）
#define DEGREE_OF_FREEDOM 6
#define N_ELEMENTS (DEGREE_OF_FREEDOM * 2)

// record statistics of kinematics and electricity
// data[i][prev(0) or cur(1)]
static char *param_names[] = {"displacement y",   "velocity y",     "rotation angle",
                              "angular velocity", "displacement x", "velocity x"};
static double data[DEGREE_OF_FREEDOM][2] = {0.};
static real fluid_force[3] = {0., 0., 0.};
static real fluid_moment[3] = {0., 0., 0.};

// user-defined parameters to be reported
DEFINE_REPORT_DEFINITION_FN(bluff_displacement_y) {
    return data[0][1];
}
DEFINE_REPORT_DEFINITION_FN(bluff_nondimensional_displacement_y) {
    return data[0][1] / LENGTH;
}
DEFINE_REPORT_DEFINITION_FN(bluff_velocity_y) {
    return data[1][1];
}
DEFINE_REPORT_DEFINITION_FN(force_y) {
    return fluid_force[1];
}

DEFINE_REPORT_DEFINITION_FN(bluff_rotation_angle_z) {
    return data[2][1];
}
DEFINE_REPORT_DEFINITION_FN(bluff_angular_velocity_z) {
    return data[3][1];
}
DEFINE_REPORT_DEFINITION_FN(moment_z) {
    return fluid_moment[2];
}

DEFINE_REPORT_DEFINITION_FN(bluff_displacement_x) {
    return data[4][1];
}
DEFINE_REPORT_DEFINITION_FN(bluff_nondimensional_displacement_x) {
    return data[4][1] / LENGTH;
}
DEFINE_REPORT_DEFINITION_FN(bluff_velocity_x) {
    return data[5][1];
}
DEFINE_REPORT_DEFINITION_FN(force_x) {
    return fluid_force[0];
}

DEFINE_ON_DEMAND(clear_m_c_k_data) {
    memset(data, 0, N_ELEMENTS * sizeof(double));
}

DEFINE_ZONE_MOTION(zone_motion, omega, axis, origin, velocity, time, dtime) {
    N3V_D(velocity, =, data[5][1], data[1][1], 0.0);
    *omega = data[3][1];
    N3V_D(origin, =, data[4][1], data[0][1], 0.0);
}

DEFINE_CG_MOTION(cg_motion, dt, velocity, omega, time, dtime) {
    N3V_D(velocity, =, data[5][1], data[1][1], 0.0);
    N3V_D(omega, =, 0.0, data[3][1], 0.0);
}

real func_f(int n, real t, double buffer[], real force[], real moment[]) {
    real ret = 0;
    real displacement_y = buffer[0], velocity_y = buffer[1], rotation_angle = buffer[2], angular_velocity_z = buffer[3],
         displacement_x = buffer[4], velocity_x = buffer[5];
    switch (n) {
        case 0:
            // dy/dt=velocity_y
            ret = velocity_y;
            break;
        case 1:
            // dvelocity_y/dt=(Fy-Cy*velocity_y-Ky*displacement_y)/M
            ret = (force[1] - DAMPING_Y * velocity_y - STIFFNESS_Y * displacement_y) / MASS_Y;
            break;
        case 2:
            // dθ/dt=angular_velocity_z
            ret = angular_velocity_z;
            break;
        case 3:
            // dangular_velocity_z/dt=Mc/Jc
            ret = moment[2] / MOMENT_INERTIA;
            break;
        case 4:
            // dx/dt=velocity_x
            ret = velocity_x;
            break;
        case 5:
            // dvelocity_x/dt=(Fx-Cx*velocity_x-Kx*displacement_x)/M
            ret = (force[0] - DAMPING_X * velocity_x - STIFFNESS_X * displacement_x) / MASS_X;
            break;
        default:
            Message("Error in func_f: %d", n);
    }
    return ret;
}

DEFINE_EXECUTE_AT_END(RungeKutta4Method) {
    if (!Data_Valid_P()) return;
    // calculate force
    Domain *domain = Get_Domain(1);                         // fluid domain pointer
    Thread *tf1 = Lookup_Thread(domain, BOUNDARY_ZONE_ID);  // FSI boundary pointer
    // calculate the force applied
    real x_cg[3];
    N3V_D(x_cg, =, data[4][1], data[0][1], 0);
    Compute_Force_And_Moment(domain, tf1, x_cg, fluid_force, fluid_moment, TRUE);
#if RP_HOST  // calculation
    Message("x:%lf, y:%lf, theta:%lf, fx:%lf, fy:%lf, mz:%lf\n", x_cg[0], x_cg[1], data[2][1], fluid_force[0],
            fluid_force[1], fluid_moment[2]);
    real time = RP_Get_Real("flow-time");          // time(s)
    real d_t = RP_Get_Real("physical-time-step");  // timestep size(s)
    // calculate the motion of structure by 4-order Runge-Kutta method
    real coeff_k[4][DEGREE_OF_FREEDOM];
    double buffer[DEGREE_OF_FREEDOM];
    const real coeffs[4] = {0., 0.5, 0.5, 1.};
    for (int k = 0; k < 4; ++k) {
        for (int j = 0; j < DEGREE_OF_FREEDOM; ++j) {
            for (int i = 0; i < DEGREE_OF_FREEDOM; ++i) {
                buffer[i] = data[i][0] + coeff_k[(k - 1) < 0 ? 0 : k - 1][i] * coeffs[k];
            }
            coeff_k[k][j] = d_t * func_f(j, time + d_t * coeffs[k], buffer, fluid_force, fluid_moment);
        }
    }
    for (int j = 0; j < DEGREE_OF_FREEDOM; ++j) {
        data[j][1] = data[j][0] + (coeff_k[0][j] + 2 * coeff_k[1][j] + 2 * coeff_k[2][j] + coeff_k[3][j]) / 6;
        data[j][0] = data[j][1];
    }

#endif
    host_to_node_double((double *)data, DEGREE_OF_FREEDOM * 2);
}

char *DATA_PATH_KEY = "/kineticsparams";
// write data
DEFINE_RW_HDF_FILE(writeDataToHDF, filename) {
    double *ptr = NULL;
#if RP_NODE
    ptr = (double *)CX_Malloc(sizeof(double) * N_ELEMENTS);
    memcpy(ptr, data, N_ELEMENTS * sizeof(double));
#endif
#if RP_HOST
    Message("write hdf5:\n");
    for (int i = 0; i < DEGREE_OF_FREEDOM; ++i) {
        Message("%s: %lf %lf  ", param_names[i], data[i][0], data[i][1]);
    }
    Message("\n");
#endif
    Write_Complete_User_Dataset(filename, DATA_PATH_KEY, ptr, N_ELEMENTS);
    if (NNULLP(ptr)) {
        CX_Free(ptr);
    }
}
// read datas
DEFINE_RW_HDF_FILE(readDataFromHDF, filename) {
    double *ptr = NULL;
#if RP_NODE
    ptr = (double *)CX_Malloc(sizeof(double) * N_ELEMENTS);
#endif
    Read_Complete_User_Dataset(filename, DATA_PATH_KEY, ptr, N_ELEMENTS);
    if (NNULLP(ptr)) {
        memcpy(data, ptr, N_ELEMENTS * sizeof(double));
        CX_Free(ptr);
    }
    node_to_host_double((double *)data, N_ELEMENTS);
    host_to_node_double((double *)data, N_ELEMENTS);
#if RP_HOST
    Message("read hdf5:\n");
    for (int i = 0; i < DEGREE_OF_FREEDOM; ++i) {
        Message("%s: %lf %lf  ", param_names[i], data[i][0], data[i][1]);
    }
    Message("\n");
#endif
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