#include <math.h>

#include "udf.h"

#define PI 3.14159265358979323846

// angle speed (rad/s) 逆时针为正
static real angular_velocity[3] = {0, 0, 2 * PI};

DEFINE_ZONE_MOTION(zone_rotation, omega, axis, origin, velocity, time, dtime) {
    N3V_D(velocity, =, 0.0, 0.0, 0.0);
    *omega = sqrt(angular_velocity[0] * angular_velocity[0] + angular_velocity[1] * angular_velocity[1] +
                  angular_velocity[2] * angular_velocity[2]);
    NV_V(axis, =, angular_velocity);
    // 此处应获取物体质心位置
    N3V_D(origin, =, 0, 0, 0);
}

// control the rigid motion of mesh
DEFINE_CG_MOTION(cg_rotation, dt, velocity, omega, time, dtime) {
    N3V_D(velocity, =, 0.0, 0.0, 0.0);
    NV_V(omega, =, angular_velocity);
}
