#ifndef AERODYNAMICSMODEL_H
#define AERODYNAMICSMODEL_H

#include <ostream>

#include <glm/glm.hpp>

#include <dlfdm/defines.h>

namespace dlfdm {

template<class T>
class Atmosphere;

class AerodynamicsModel
{
public:
    struct AeroDynamicForces {
        glm::vec3 body_forces;       /// [N] - Body frame
        glm::vec3 body_moments;      /// [N·m] - Body frame - [x=L, y=M, z=N]
    };

    AerodynamicsModel(const AircraftParameters& p, Atmosphere<float> *atm);

    // Calculate angle of attack and sideslip from velocity
    void calculate_angles(const glm::vec3& vel, float& alpha, float& beta) const;

    // Calculate aerodynamic forces and moments
    AeroDynamicForces calculate(const glm::vec3& body_velocity,
                                const glm::vec3& body_omega,
                                const ControlInputs& controls);

    void log_all_titles(std::ostream& os, const char& sep = ',') const;
    void log_all(std::ostream& os, const char& sep = ',') const;

    void log_angles_titles(std::ostream& os, const char& sep = ',') const;
    void log_angles(std::ostream& os, const char& sep = ',') const;
    void log_forces_titles(std::ostream& os, const char& sep = ',') const;
    void log_forces(std::ostream& os, const char& sep = ',') const;
    void log_moments_titles(std::ostream& os, const char& sep = ',') const;
    void log_moments(std::ostream& os, const char& sep = ',') const;

private:
    /// Referenced, not copied: the referent is FDMSolver::aircraft_data_, the
    /// solver's own copy, which outlives this object. Constructing this model
    /// directly from a temporary leaves this reference dangling.
    const AircraftParameters& aircraft_data_;
    Atmosphere<float>* atmosphere_;

    glm::vec3 wind_forces_;         // Forces in wind axis
    glm::vec3 aero_moments_;
    glm::vec2 aero_angles_;

    glm::vec3 body_forces_;
    glm::vec3 body_moments_;
};

} // namespace dlfdm

#endif // AERODYNAMICSMODEL_H
