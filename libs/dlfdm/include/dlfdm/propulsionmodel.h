#ifndef PROPULSIONMODEL_H
#define PROPULSIONMODEL_H

#include <memory>
#include <ostream>

#include <glm/glm.hpp>

#include <dlfdm/defines.h>
#include <dlfdm/models/propulsion/engine.h>

namespace dlfdm {

template<class T>
class Atmosphere;

///
/// \brief Owns the aircraft's engine and turns throttle into a body force.
///
/// Builds the FlightCondition once per step from the atmosphere injected by the
/// solver (ram ratios are shared by every engine model) and hands it to the
/// engine the aircraft declared in AircraftParameters::engine.
///
class PropulsionModel
{
public:
    PropulsionModel(const AircraftParameters& p, Atmosphere<float>* atm);

    ///
    /// \brief Thrust force in body axes [N]. Installed along the body x axis
    /// through the cg, so it produces no moment.
    ///
    glm::vec3 calculate(const glm::vec3& body_velocity,
                        const ControlInputs& controls);

    float get_thrust(void) const    { return thrust_; }
    float get_lapse(void) const     { return lapse_; }

    void log_all_titles(std::ostream& os, const char& sep = ',') const;
    void log_all(std::ostream& os, const char& sep = ',') const;

private:
    /// Referenced, not copied: the referent is FDMSolver::aircraft_data_, the
    /// solver's own copy, which outlives this object. Constructing this model
    /// directly from a temporary leaves this reference dangling.
    const AircraftParameters& aircraft_data_;
    Atmosphere<float>* atmosphere_;
    std::unique_ptr<Engine> engine_;

    FlightCondition flight_cond_;
    float thrust_;      // [N] total, all engines
    float lapse_;       // [-] F/F_SL of a single engine
};

}   // End namespace dlfdm

#endif // PROPULSIONMODEL_H
