#ifndef DLFDM_ENGINE_H
#define DLFDM_ENGINE_H

#pragma once
#include <dlfdm/defines.h>

namespace dlfdm {

///
/// \brief Common interface for every engine model in models/propulsion.
///
/// A model is a pure function of the flight condition and the throttle: the
/// ram ratios it needs are already in FlightCondition, so it never touches the
/// atmosphere. That keeps each engine type testable on its own.
///
class Engine {
public:
    explicit Engine(const EngineParameters& p) : engine_data_(p)
    {}

    virtual ~Engine(void)
    {}

    ///
    /// \brief Thrust lapse F/F_SL at the given flight condition.
    ///
    virtual float lapse(const FlightCondition& fc) const = 0;

    ///
    /// \brief Thrust [N] of a single engine.
    ///
    float thrust(const FlightCondition& fc, const float& throttle) const {
        return engine_data_.max_thrust_sl * lapse(fc) * throttle;
    }

protected:
    /// Copied, not referenced: the aircraft parameters the solver was built
    /// from may be a temporary.
    const EngineParameters engine_data_;
};

}   // End namespace dlfdm

#endif // DLFDM_ENGINE_H
