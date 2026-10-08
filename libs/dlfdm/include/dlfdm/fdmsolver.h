#ifndef FDMSOLVER_H
#define FDMSOLVER_H

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <dlfdm/defines.h>
#include <dlfdm/aerodynamicsmodel.h>
#include <dlfdm/aircraftdynamics.h>
#include <dlfdm/propulsionmodel.h>
#include <dlfdm/models/atmosphere/isa.h>

namespace dlfdm {

class FDMSolver
{
public:
    FDMSolver(const AircraftParameters& p, float dt = 1.0f / 120.0f);

    void update(const ControlInputs& controls);

    const AircraftState& getState() const { return aircraft_state_; }
    void setState(const AircraftState& newState) {
        aircraft_state_ = newState;
        // Update atmosphere to new aircraft state
        atmosphere_.update(-1.0f * aircraft_state_.inertial_position.z);
    }

    const AircraftDynamics::StateDerivatives get_state_dot() const {
        return state_deriv_;
    }

    void setTimeStep(float dt) { time_step_ = dt; }

    float get_sim_time(void) const      { return time_; }

    glm::mat4 getModelMatrix() const;

    void log_titles(std::ostream& os, const char& sep = ',') const;
    void log_state(std::ostream& os, const char& sep = ',') const;

    ///
    /// \brief get_aero_fm Return aerodynamic forces and moments
    /// \return Aerodynamic forces and moments
    ///
    AerodynamicsModel::AeroDynamicForces get_aero_fm(void) { return aero_fm_; }

    ///
    /// \brief get_thrust Total thrust force of all engines [N]
    ///
    float get_thrust(void) const { return propulsion.get_thrust(); }

private:
    AircraftState aircraft_state_;
    AircraftParameters aircraft_data_;
    ISA<float> atmosphere_;
    AerodynamicsModel aerodynamics;
    PropulsionModel propulsion;
    AircraftDynamics dynamics;

    float time_step_;
    float time_;

    AerodynamicsModel::AeroDynamicForces aero_fm_;
    glm::vec3 body_thrust_;
    AircraftDynamics::StateDerivatives state_deriv_;

    void log_state_titles(std::ostream& os, const char& sep = ',') const;
    void log_aircraft_state(std::ostream& os, const char& sep = ',') const;

    void log_atm_titles(std::ostream& os, const char& sep = ',') const;
    void log_atm_state(std::ostream& os, const char& sep = ',') const;
};

}   // End namespace dlfdm

#endif // FDMSOLVER_H
