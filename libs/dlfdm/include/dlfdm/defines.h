#ifndef FDM_DEFINES_H
#define FDM_DEFINES_H

#pragma once
#include <glm/glm.hpp>

namespace dlfdm {

struct ControlInputs {
    float throttle;      // [0, 1]
    float elevator;      // [rad] - pitch control
    float aileron;       // [rad] - roll control
    float rudder;        // [rad] - yaw control
};

struct AircraftState {
    glm::vec3 inertial_position;        // [m] - [x=north, y=east, z=down] Position (Inertial frame)
    glm::vec3 body_velocity;            // [m/s] - [x=u, y=v, z=w] Velocity (Body frame)
    glm::vec3 body_omega;               // [rad/s] - [x=p, y=q, z=r] Angular velocity (Body frame)

    // Attitude (Euler angles)
    float phi;                          // [rad] - Roll angle
    float theta;                        // [rad] - Pitch angle
    float psi;                          // [rad] - Yaw angle psi
};

///
/// \brief A trim point: state **and** controls. Both together, because apart
/// they mean nothing - changing one without the other breaks the equilibrium.
///
struct TrimPoint {
    AircraftState  state;
    ControlInputs  controls;
};

///
/// \brief Engine types with a thrust model available in models/propulsion.
///
enum class EngineType {
    kTurbojet = 0,
    kTurbofan,
};

///
/// \brief Engine data an aircraft declares. Each model reads only the fields
/// it needs, so unused ones can be left at zero.
///
struct EngineParameters {
    EngineType type;
    float max_thrust_sl;    // [N] - F_SL, thrust at sea level, static
    float throttle_ratio;   // [-] - TR, throttle ratio
    float bypass_ratio;     // [-] - BPR, turbofan only: selects low/high bypass
    bool  afterburner;      // [-] - maximum (true) or military (false) rating
};

///
/// \brief Flight condition handed to the engine models. Built once per step by
/// PropulsionModel, so every engine shares the same ram ratios.
///
struct FlightCondition {
    float tas;          // [m/s] - true airspeed
    float mach;         // [-]
    float theta_0;      // [-] - total/SL temperature ratio, Gudmundsson Eq. (7-21)
    float delta_0;      // [-] - total/SL pressure ratio, Gudmundsson Eq. (7-22)
    float density;      // [kg/m3] - for propeller models
};

struct AircraftParameters {
    // Mass properties
    float mass;             // [kg]
    float Ixx, Iyy, Izz;    // [kg·m2] Moments of inertia
    float Ixz;              // [kg·m2] Cross moment

    // Aerodynamic reference data
    float wingArea;
    float wingChord;
    float wingSpan;

    // Propulsion
    EngineParameters engine;    // Engine type and its coefficients
    int engine_count;           // [-] Number of identical engines installed

    // Aerodynamic coefficients (linear model)
    float CL0, CLa, CL_delta_e;         // Lift: CL = CL0 + CLa*alpha + CL_delta_e*delta_e
    float CD0, CDa;                     // Drag: CD = CD0 + CDa*alpha
    float Cm0, Cma, Cm_q;               // Pitch moment: Cm = Cm0 + Cma*alpha + Cmq*q*c_bar/2V
    float CY_beta, CY_r, CY_delta_r;    // Side: CY = CY_beta*beta + CY_r*r + CY_delta_r*delta_r
    float Cl_beta;                      // Roll moment from sideslip
    float Cn_beta;                      // Yaw moment from sideslip
    float Cl_p, Cn_r, Cl_r, Cn_p;       // Lateral damping coefficients

    // Control effectiveness      
    float Cm_delta_e;      // Pitch control
    float Cl_delta_a;      // Roll control
    float Cn_delta_r;      // Yaw control

    // Max control deflections
    float min_elevator;     // [rad]
    float max_elevator;     // [rad]
    float min_aileron;      // [rad]
    float max_aileron;      // [rad]
    float max_rudder;       // [rad]
};

}   // End namespace dlfdm

#endif // FDM_DEFINES_H
