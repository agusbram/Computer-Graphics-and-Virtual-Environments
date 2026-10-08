#include <dlfdm/fdmsolver.h>

namespace dlfdm {

static std::ostream& operator<<(std::ostream& os, const glm::vec3 v){
    os << v.x << "," << v.y << "," << v.z;
    return os;
}

// The submodels keep a reference to the aircraft parameters, so they are
// initialized from aircraft_data_ --- this solver's own copy --- and never from
// p, which may be a temporary that dies with this constructor. aircraft_data_ is
// declared before them in fdmsolver.h, so it is built first and destroyed last.
FDMSolver::FDMSolver(const AircraftParameters& p, float dt)
    : aircraft_data_(p), aerodynamics(aircraft_data_,&atmosphere_),
      propulsion(aircraft_data_,&atmosphere_), dynamics(aircraft_data_),
      time_step_(dt), time_(0.0f), body_thrust_(0.0f)
{
    // Initialize state
    aircraft_state_.inertial_position = glm::vec3(0.0f);
    aircraft_state_.body_velocity = glm::vec3(10.0f, 0.0f, 0.0f);  // Initial forward velocity
    aircraft_state_.phi = 0.0f;
    aircraft_state_.theta = 0.0f;
    aircraft_state_.psi = 0.0f;
    aircraft_state_.body_omega = glm::vec3(0.0f);

    // Update atmosphere
    atmosphere_.update(-1.0f * aircraft_state_.inertial_position.z);
}

void FDMSolver::update(const ControlInputs &controls) {
    // Clamp controls
    ControlInputs clamped_controls = controls;

    clamped_controls.throttle   = glm::clamp(clamped_controls.throttle,
                                             0.0f,
                                             1.0f);
    clamped_controls.elevator   = glm::clamp(clamped_controls.elevator,
                                             aircraft_data_.min_elevator,
                                             aircraft_data_.max_elevator);
    clamped_controls.aileron    = glm::clamp(clamped_controls.aileron,
                                             aircraft_data_.min_aileron,
                                             aircraft_data_.max_aileron);
    clamped_controls.rudder     = glm::clamp(clamped_controls.rudder,
                                             -aircraft_data_.max_rudder,
                                             aircraft_data_.max_rudder);

    // Update atmosphere
    atmosphere_.update(-1.0f * aircraft_state_.inertial_position.z);

    // Calculate aerodynamic forces and moments
    aero_fm_ = aerodynamics.calculate(aircraft_state_.body_velocity,
                                      aircraft_state_.body_omega,
                                      clamped_controls);

    // Calculate thrust force in body axes
    body_thrust_ = propulsion.calculate(aircraft_state_.body_velocity,
                                        clamped_controls);

    // Compute state derivatives
    state_deriv_ = dynamics.compute_derivatives(aircraft_state_, aero_fm_, body_thrust_);

    // Euler integration
    time_ += time_step_;

    // Positions in inertial frame
    aircraft_state_.inertial_position += state_deriv_.ned_position_dot * time_step_;

    // Velocities in body frame
    aircraft_state_.body_velocity += state_deriv_.body_velocity_dot * time_step_;
    aircraft_state_.body_omega    += state_deriv_.body_omega_dot * time_step_;

    // Attitude in body frame
    aircraft_state_.phi   += state_deriv_.euler_dot.x * time_step_;
    aircraft_state_.theta += state_deriv_.euler_dot.y * time_step_;
    aircraft_state_.psi   += state_deriv_.euler_dot.z * time_step_;

    // Clamp pitch to avoid singularities
    aircraft_state_.theta = glm::clamp(aircraft_state_.theta, -1.5f, 1.5f);

    // Normalize yaw to [-pi, pi]
    while (aircraft_state_.psi > 3.14159f) aircraft_state_.psi -= 6.28318f;
    while (aircraft_state_.psi < -3.14159f) aircraft_state_.psi += 6.28318f;
}

glm::mat4 FDMSolver::getModelMatrix() const {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), aircraft_state_.inertial_position);
    model = glm::rotate(model, aircraft_state_.psi, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, aircraft_state_.theta, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, aircraft_state_.phi, glm::vec3(1.0f, 0.0f, 0.0f));
    return model;
}

void FDMSolver::log_titles(std::ostream &os, const char &sep) const
{
    os << "t [seg]" << sep;
    log_state_titles(os,sep);
    os << sep;
    aerodynamics.log_all_titles(os,sep);
    os << sep;
    dynamics.log_state_titles(os,sep);
    os << sep;
    log_atm_titles(os,sep);
    os << sep;
    propulsion.log_all_titles(os,sep);
    os << std::endl;
}

void FDMSolver::log_state(std::ostream& os, const char& sep) const {
    os << time_ << sep;
    log_aircraft_state(os,sep);
    os << sep;
    aerodynamics.log_all(os,sep);
    os << sep;
    dynamics.log_state_derivatives(os,sep);
    os << sep;
    log_atm_state(os,sep);
    os << sep;
    propulsion.log_all(os,sep);
    os << std::endl;
}

void FDMSolver::log_state_titles(std::ostream &os, const char &sep) const
{
    os << "x [m]" << sep << "y [m]" << sep << "z [m]" << sep;
    os << "phi [rad]" << sep << "theta [rad]" << sep << "psi [rad]" << sep;
    os << "u [m/s]" << sep << "v [m/s]" << sep << "w [m/s]" << sep;
    os << "p [rad/s]" << sep << "q [rad/s]" << sep << "r [rad/s]";
}

void FDMSolver::log_aircraft_state(std::ostream &os, const char &sep) const
{
    os << aircraft_state_.inertial_position << sep;
    os << aircraft_state_.phi << sep << aircraft_state_.theta << sep << aircraft_state_.psi << sep;
    os << aircraft_state_.body_velocity << sep;
    os << aircraft_state_.body_omega;
}

void FDMSolver::log_atm_titles(std::ostream &os, const char &sep) const
{
    os << "alt [m]" << sep << "T [K]" << sep << "P [Pa]" << sep << "rho [kg/m3]";
}

void FDMSolver::log_atm_state(std::ostream &os, const char &sep) const
{
    os << atmosphere_.get_altitude() << sep;
    os << atmosphere_.get_temperature() << sep;
    os << atmosphere_.get_pressure() << sep;
    os << atmosphere_.get_density();
}

} // namespace dlfdm
