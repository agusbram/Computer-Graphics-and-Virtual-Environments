#include <dlfdm/models/aircraft/jettrainer.h>

#include <glm/trigonometric.hpp>

namespace dlfdm {

namespace jettrainer {

///
/// \brief Trim condition: steady flight at 5000 [m] on the standard atmosphere
///  - ISA at h = 5000 [m] -> T = 255.65 [K], rho = 0.73611 [kg/m3]
///  - TAS = 150 [m/s], theta = 0 [deg], alpha = +0.581 [deg]
///  - M = 0.468, thrust lapse F/F_SL = 0.4118
/// \return Initial state and controls for the FDM as a TrimPoint struct
///
static TrimPoint tc_ISA5000TAS150(void)
{
    // -------------------------------------------------------------------------
    // The control positions are calculated solving the aircraft equilibrium
    // force and moment equations in the symmetry plane (XZ) for the condition:
    // Altitude: 5000 [m], TAS = 150 [m/s], theta = 0 [deg] -> u = 149.992 m/s
    // & w = 1.521 m/s, rho taken from the ISA model at that altitude.
    // The throttle accounts for the thrust lapse: at this altitude and Mach the
    // engine delivers 0.4118 of its sea level thrust, so the throttle is the
    // one that balanced the aircraft with constant thrust divided by that
    // factor. Altitude and Mach are fixed here, so the lapse is a constant and
    // the aerodynamic part of the trim (alpha, elevator) is untouched by it.
    // Verified over a 60 s run: altitude stays within +-20 m of 5000 m
    // (residual phugoid).
    // Do not change any of these values as it will break the equilibrium!
    // -------------------------------------------------------------------------
    TrimPoint trim_point{};

    trim_point.state.inertial_position = glm::vec3(0.0f,0.0f,-5000.0f);  // [m] - NED Ref. System: North, East, Down
    trim_point.state.body_velocity = glm::vec3(149.992f,0.0f,1.521f);    // [m/s]
    trim_point.state.body_omega = glm::vec3(0.0f,0.0f,0.0f);
    trim_point.state.theta = 0.0f;
    trim_point.state.phi = 0.0f;
    trim_point.state.psi = 0.0f;

    trim_point.controls.elevator   = -0.0937f;      // [rad]
    trim_point.controls.aileron    = 0.0f;
    trim_point.controls.rudder     = 0.0f;
    trim_point.controls.throttle   = 0.49601f;      // [fraction]

    return trim_point;
}

AircraftParameters load_model(void)
{
    AircraftParameters p;

    // Data copied from Airplane Flight Dynamics and Automatic Controls - Part I
    // Roskam, J., pag. 494-500 - Airplane 'C'
    // Mass properties
    p.mass = 1815.0f;           // [kg]
    p.Ixx = 1084.6f;            // [kg·m^2]
    p.Iyy = 6507.9f;            // [kg·m^2]
    p.Izz = 7050.2f;            // [kg·m^2]
    p.Ixz = 271.16f;            // [kg·m^2]

    // Aerodynamic reference
    p.wingArea = 12.63f;        // [m^2]
    p.wingChord = 1.64f;        // [m]
    p.wingSpan = 8.01f;         // [m]

    // Propulsion. Pratt & Whitney Canada JT15D-4B, from Table 7-11, pag. 202,
    // General Aviation Aircraft Design - Gudmundsson, S.: BPR 3.3 (so the high
    // bypass formulation applies) and 2500 lbf = 11.12 kN of T-O thrust.
    p.engine.type           = EngineType::kTurbofan;
    p.engine.max_thrust_sl  = 11120.0f;     // [N]
    p.engine.throttle_ratio = 1.072f;       // [-] TR used in Figure 7-11
    p.engine.bypass_ratio   = 3.3f;         // [-]
    p.engine.afterburner    = false;        // [-] not applicable to high bypass
    p.engine_count          = 1;            // [-]

    // Aerodynamic coefficients
    p.CL0 = 0.15f;              // [-]
    p.CLa = 5.5f;               // [1/rad]
    p.CL_delta_e = 0.38f;       // [1/rad]
    p.CD0 = 0.0205f;            // [-]
    p.CDa = 0.12f;              // [1/rad]
    p.Cm0 = -0.08f;             // [-]
    p.Cma = -0.24f;             // [1/rad]
    p.Cm_q = -15.7f;            // [1/rad]
    p.CY_beta = -1.0f;          // [1/rad]
    p.CY_r = 0.61f;             // [1/rad]
    p.CY_delta_r = 0.028f;      // [1/rad]
    p.Cl_beta = -0.11f;         // [1/rad]
    p.Cl_p = -0.39f;            // [1/rad]
    p.Cl_r = 0.28f;             // [1/rad]
    p.Cn_beta = 0.17f;          // [1/rad]
    p.Cn_p = 0.09f;             // [1/rad]
    p.Cn_r = -0.26f;            // [1/rad]

    // Control effectiveness
    p.Cm_delta_e = -0.88f;      // [1/rad]
    p.Cl_delta_a = 0.10f;       // [1/rad]
    p.Cn_delta_r = -0.12f;      // [1/rad]

    // Min-max surface deflections based on typical values from
    // Table 4.1, pag. 203, Ch. 4 Flight Control system layout design - Roskam, J.
    p.min_elevator = glm::radians(-25.0f);  // [rad]
    p.max_elevator = glm::radians(15.0f);   // [rad]

    p.min_aileron = glm::radians(-20.0f);   // [rad]
    p.max_aileron = glm::radians(20.0f);    // [rad]

    p.max_rudder = glm::radians(30.0f);     // [rad]

    return p;
}

TrimPoint get_trim_condition(TrimCondition condition)
{
    TrimPoint t_point{};

    switch (condition) {
    case TrimCondition::kISA5000TAS150:
        t_point = tc_ISA5000TAS150();
        break;
    // No default case to handle undefined cases for enumeration items with
    // compiler warning [-Wswitch]
    }

    return t_point;
}

} // namespace jettrainer

}   // End namespace dlfdm
