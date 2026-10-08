#include <dlfdm/aerodynamicsmodel.h>

#include <iostream>

#include <dlfdm/models/atmosphere/atmosphere.h>
#include <dlfdm/tools.h>

namespace dlfdm {

static std::ostream& operator<<(std::ostream& os, const glm::vec2 v){
    os << v.x << "," << v.y;
    return os;
}

static std::ostream& operator<<(std::ostream& os, const glm::vec3 v){
    os << v.x << "," << v.y << "," << v.z;
    return os;
}

AerodynamicsModel::AerodynamicsModel(const AircraftParameters& p,
                                     Atmosphere<float> *atm)
    : aircraft_data_(p), atmosphere_(atm)
{
    wind_forces_    = glm::vec3(0.0f);
    aero_moments_   = glm::vec3(0.0f);
    aero_angles_    = glm::vec2(0.0f);

    body_forces_    = glm::vec3(0.0f);
    body_moments_   = glm::vec3(0.0f);
}

void AerodynamicsModel::calculate_angles(const glm::vec3 &vel, float &alpha, float &beta) const {
    float u = vel.x;
    float v = vel.y;
    float w = vel.z;

    float V = glm::length(vel);
    if (V < 0.1f) {
        alpha = 0.0f;
        beta = 0.0f;
        return;
    }

    alpha = std::atan2(w, u);
    beta = glm::asin(glm::clamp(v / V, -1.0f, 1.0f));
}

AerodynamicsModel::AeroDynamicForces AerodynamicsModel::calculate(const glm::vec3 &body_velocity,
                                                                  const glm::vec3 &body_omega,
                                                                  const ControlInputs &controls)
{
    float p = body_omega.x;
    float q = body_omega.y;
    float r = body_omega.z;

    float V = glm::length(body_velocity);

    if (V < 0.1f) {
        return {{0, 0, 0}, {0, 0, 0}};
    }

    float alpha, beta;
    calculate_angles(body_velocity, alpha, beta);
    aero_angles_ = glm::vec2(alpha,beta);

    float rho = atmosphere_->get_density();

    // Dynamic pressure
    float qbar = 0.5f * rho * V * V;

    // Characteristic lengths for moment non-dimensionalization
    float c_bar = aircraft_data_.wingChord;
    float b = aircraft_data_.wingSpan;

    // Longitudinal aerodynamics forces and moments
    // Lift and Drag coefficients
//    float elevator = clamp(controls.elevator, aircraft_data_.min_elevator, aircraft_data_.max_elevator);

    float CL = aircraft_data_.CL0
            + aircraft_data_.CLa * alpha
            + aircraft_data_.CL_delta_e * controls.elevator;

    float CD = aircraft_data_.CD0 + aircraft_data_.CDa * alpha;

    // Pitch moment
    float Cm = aircraft_data_.Cm0
            + aircraft_data_.Cma * alpha
            + (aircraft_data_.Cm_q * q * c_bar) / (2.0f * V)
            + aircraft_data_.Cm_delta_e * controls.elevator;

    // Lateral aerodynamics forces and moments
    // Side force
//    float rudder = clamp(controls.rudder, -aircraft_data_.max_rudder, aircraft_data_.max_rudder);

    float CY = aircraft_data_.CY_beta * beta
            + aircraft_data_.CY_delta_r * controls.rudder;
//            + params.CY_r * r;

    // Roll moment from sideslip and control
//    float aileron = clamp(controls.aileron, aircraft_data_.min_aileron, aircraft_data_.max_aileron);

    float Cl = aircraft_data_.Cl_beta * beta
            + aircraft_data_.Cl_delta_a * controls.aileron
            + (aircraft_data_.Cl_p * p * b) / (2.0f * V)
            + (aircraft_data_.Cl_r * r * b) / (2.0f * V);

    // Yaw moment from sideslip and control
    float Cn = aircraft_data_.Cn_beta * beta
            + aircraft_data_.Cn_delta_r * controls.rudder
            + (aircraft_data_.Cn_r * r * b) / (2.0f * V)
            + (aircraft_data_.Cn_p * p * b) / (2.0f * V);

    // Forces in aerodynamic axes (lift up, drag back)
    float D = qbar * aircraft_data_.wingArea * CD;
    float Y = qbar * aircraft_data_.wingArea * CY;
    float L = qbar * aircraft_data_.wingArea * CL;

    // The negative sign corresponds to the wind axes definition
    // x_w -> forward, y_w -> right , z_w -> down
    wind_forces_.x = -D;
    wind_forces_.y = Y;
    wind_forces_.z = -L;

    // Transform to body axes
    // Aircraft simulation and control, 1st Ed. - Stevens & Lewis
    // Eq. (2.3-2b) pag. 63 (pdf 85)
    const float ca = glm::cos(alpha);
    const float sa = glm::sin(alpha);
    const float cb = glm::cos(beta);
    const float sb = glm::sin(beta);

    // Important! glm matrix are row major order as per opengl standard, but the
    // body to wing transformation presented in the book is given in column major
    // order so it has to be written as a transpose in glm:
    // ie. glm::mat3 bodyToWind = glm::transpose(windToBody);
    glm::mat3 windToBody( ca * cb,   sb,    sa * cb,
                         -ca * sb,   cb,   -sa * sb,
                         -sa,        0.0f,  ca      );

    body_forces_ = windToBody * wind_forces_;

    // Moments in body frame
    float L_moment = qbar * aircraft_data_.wingArea * b * Cl;
    float M_moment = qbar * aircraft_data_.wingArea * c_bar * Cm;
    float N_moment = qbar * aircraft_data_.wingArea * b * Cn;

    body_moments_ = glm::vec3(L_moment, M_moment, N_moment);

    AeroDynamicForces aero;
    aero.body_forces = body_forces_;
    aero.body_moments = body_moments_;

    return aero;
}

void AerodynamicsModel::log_all_titles(std::ostream &os, const char &sep) const
{
    log_forces_titles(os,sep);
    os << sep;
    log_moments_titles(os,sep);
    os << sep;
    log_angles_titles(os,sep);
}

void AerodynamicsModel::log_all(std::ostream &os, const char &sep) const
{
    log_forces(os, sep);
    os << sep;
    log_moments(os, sep);
    os << sep;
    log_angles(os, sep);
}

void AerodynamicsModel::log_angles_titles(std::ostream &os, const char &sep) const
{
    os << "Alpha [rad]" << sep << "Beta [rad]";
}

void AerodynamicsModel::log_angles(std::ostream& os, const char &) const
{
    os << aero_angles_;
}

void AerodynamicsModel::log_forces_titles(std::ostream &os, const char &sep) const
{
    os << "D [N]" << sep << "Y [N]" << sep << "L [N]";
    os << sep;
    os << "Xb [N]" << sep << "Yb [N]" << sep << "Zb [N]";
}

void AerodynamicsModel::log_forces(std::ostream& os, const char& sep) const
{
    os << wind_forces_ << sep << body_forces_;
}

void AerodynamicsModel::log_moments_titles(std::ostream &os, const char &sep) const
{
    os << "L [N·m]" << sep << "M [N·m]" << sep << "N [N·m]";
}

void AerodynamicsModel::log_moments(std::ostream& os, const char &) const
{
    os << body_moments_;
}

} // namespace dlfdm
