#include <dlfdm/aircraftdynamics.h>

#include <iostream>

#include <dlfdm/tools.h>

namespace dlfdm {

static std::ostream& operator<<(std::ostream& os, const glm::vec3 v){
    os << v.x << "," << v.y << "," << v.z;
    return os;
}

AircraftDynamics::AircraftDynamics(const AircraftParameters& p) : aircraft_data_(p)
{
    state_deriv_.euler_dot           = glm::vec3(0.0f,0.0f,0.0f);
    state_deriv_.ned_position_dot    = glm::vec3(0.0f,0.0f,0.0f);
    state_deriv_.body_velocity_dot   = glm::vec3(0.0f,0.0f,0.0f);
    state_deriv_.body_omega_dot      = glm::vec3(0.0f,0.0f,0.0f);

    body_total_force_ = glm::vec3(0.0f,0.0f,0.0f);
}

AircraftDynamics::StateDerivatives AircraftDynamics::compute_derivatives(const AircraftState &state,
                                                                        const AerodynamicsModel::AeroDynamicForces &aero,
                                                                        const glm::vec3 &body_thrust)
{
    // Body frame velocities
    float u = state.body_velocity.x;
    float v = state.body_velocity.y;
    float w = state.body_velocity.z;
    float p = state.body_omega.x;
    float q = state.body_omega.y;
    float r = state.body_omega.z;

    float phi   = state.phi;
    float theta = state.theta;
    float psi   = state.psi;

    float cp = glm::cos(phi);
    float sp = glm::sin(phi);
    float ct = glm::cos(theta);
    float st = glm::sin(theta);
    float tt = glm::tan(theta);

    // Position derivative (inertial frame)
    float cy = glm::cos(psi);
    float sy = glm::sin(psi);

    // Transform NED to Body axes
    // Aircraft simulation and control, 1st Ed. - Stevens & Lewis
    // Eq. (1.4-10) pag. 37 (pdf 59)

    // Important! glm matrix are row major order as per opengl standard, but the
    // ned to body transformation presented in the book is given in column major
    // order so it has to be written as a transpose in glm:
    // ie. glm::mat3 ned_to_body = glm::transpose(body_to_ned);
    glm::mat3 body_to_ned(
                ct * cy,                    ct * sy,                   -st,
                sp * st * cy - cp * sy,     sp * st * sy + cp * cy,    sp * ct,
                cp * st * cy + sp * sy,     cp * st * sy - sp * cy,    cp * ct
                );

    // -------------------------------------------------------------------------
    // Flat earth approximation
    // -------------------------------------------------------------------------

    // Navigation equations
    // Aircraft simulation and control, 1st Ed. - Stevens & Lewis
    // Eq. 2.4-5 pag. 81 (pdf 103)
    state_deriv_.ned_position_dot = body_to_ned * state.body_velocity;

    // Velocity derivative (body frame)
    // Aircraft simulation and control, 1st Ed. - Stevens & Lewis
    // Eq. (1.5-4) pag. 37 (pdf 59) y 2.4-2 pag. 81 (pdf 103)
    constexpr float kGravityAcc = 9.80665f;    // [m/s2]

    // Add non aerodynamics forces and moments. Thrust comes already resolved
    // in body axes from PropulsionModel, which is the one that knows the
    // atmosphere.
    body_total_force_ = aero.body_forces + body_thrust;

    state_deriv_.body_velocity_dot.x = (body_total_force_.x / aircraft_data_.mass - kGravityAcc * st) - q * w + r * v;
    state_deriv_.body_velocity_dot.y = (body_total_force_.y / aircraft_data_.mass + kGravityAcc * sp * ct) - r * u + p * w;
    state_deriv_.body_velocity_dot.z = (body_total_force_.z / aircraft_data_.mass + kGravityAcc * cp * ct) - p * v + q * u;

    // Attitude rate (Euler angles)
    // Aircraft simulation and control, 1st Ed. - Stevens & Lewis
    // Eq. 2.4-3 pag. 81 (pdf 103)
    state_deriv_.euler_dot.x = p + (q * sp + r * cp) * tt;
    state_deriv_.euler_dot.y = q * cp - r * sp;
    state_deriv_.euler_dot.z = (q * sp + r * cp) / ct;

    // Angular acceleration (body frame)
    // Aircraft simulation and control, 1st Ed. - Stevens & Lewis
    // Eq. 2.4-5 pag. 81 (pdf 103)
    const float Ixx = aircraft_data_.Ixx;
    const float Iyy = aircraft_data_.Iyy;
    const float Izz = aircraft_data_.Izz;
    const float Ixz = aircraft_data_.Ixz;

    const float gamma = Ixx * Izz - Ixz * Ixz;
    const float c1 = (Izz * (Iyy - Izz) - Ixz * Ixz) / gamma;
    const float c2 = (Ixz * (Ixx - Iyy + Izz)) / gamma;
    const float c3 = Izz / gamma;
    const float c4 = Ixz / gamma;
    const float c5 = (Izz - Ixx) / Iyy;
    const float c6 = Ixz / Iyy;
    const float c8 = (Ixx * (Ixx - Iyy) + Ixz * Ixz) / gamma;
    const float c9 = Ixx / gamma;

    // p dot
    state_deriv_.body_omega_dot.x = (c1 * r + c2 * p) * q
            + c3 * aero.body_moments.x
            + c4 * aero.body_moments.z;

    // q dot
    state_deriv_.body_omega_dot.y = c5 * p * r
            - c6 * (p * p - r * r)
            + aero.body_moments.y / Iyy;

    // r dot
    state_deriv_.body_omega_dot.z = (c8 * p - c2 * r) * q
            + c4 * aero.body_moments.x
            + c9 * aero.body_moments.z;

    return state_deriv_;
}

void AircraftDynamics::log_state_titles(std::ostream &os, const char &sep) const
{
    os << "p_dot2 [rad/s2]" << sep << "q_dot2 [rad/s2]" << sep << "r_dot2 [rad/s2]";
    os << sep;
    os << "u_dot [m/s2]" << sep << "v_dot [m/s2]" << sep << "w_dot [m/s2]";
    os << sep;
    os << "xdot_ned [m/s]" << sep << "ydot_ned [m/s]" << sep << "zdot_ned [m/s]";
//    os << sep;
//    os << "X_t [N]" << sep << "Y_t [N]" << sep << "Z_t [N]";
//    os << "u [m/s]" << sep << "v [m/s]" << sep << "w [m/s]";
//    os << sep;
}

void AircraftDynamics::log_state_derivatives(std::ostream& os, const char &sep) const
{
    os << state_deriv_.body_omega_dot << sep;
    os << state_deriv_.body_velocity_dot << sep;
    os << state_deriv_.ned_position_dot;
//    os << body_total_force_;
//    os << state_deriv_.positionDot << sep;
}

} // namespace dlfdm
