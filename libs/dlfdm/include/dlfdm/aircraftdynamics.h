#ifndef AIRCRAFTDYNAMICS_H
#define AIRCRAFTDYNAMICS_H

#include <glm/glm.hpp>

#include <dlfdm/defines.h>
#include <dlfdm/aerodynamicsmodel.h>

namespace dlfdm {

class AircraftDynamics
{
public:
    // Compute state derivatives
    struct StateDerivatives {
        glm::vec3 ned_position_dot;
        glm::vec3 body_velocity_dot;
        glm::vec3 euler_dot;
        glm::vec3 body_omega_dot;
    };

    AircraftDynamics(const AircraftParameters& p);

    StateDerivatives compute_derivatives(const AircraftState& state,
                                         const AerodynamicsModel::AeroDynamicForces& aero,
                                         const glm::vec3& body_thrust);

    void log_state_titles(std::ostream& os, const char& sep = ',') const;
    void log_state_derivatives(std::ostream& os, const char& sep = ',') const;

private:
    /// Referenced, not copied: the referent is FDMSolver::aircraft_data_, the
    /// solver's own copy, which outlives this object. Constructing this model
    /// directly from a temporary leaves this reference dangling.
    const AircraftParameters& aircraft_data_;

    StateDerivatives state_deriv_;
    glm::vec3 body_total_force_;
};

} // namespace dlfdm

#endif // AIRCRAFTDYNAMICS_H
