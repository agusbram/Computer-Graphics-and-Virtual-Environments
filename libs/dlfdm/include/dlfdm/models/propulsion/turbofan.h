#ifndef DLFDM_TURBOFAN_H
#define DLFDM_TURBOFAN_H

#pragma once
#include <dlfdm/models/propulsion/engine.h>

namespace dlfdm {

///
/// \brief Turbofan thrust lapse with altitude and Mach number.
///
/// General Aviation Aircraft Design, Gudmundsson, S., 1st Ed., Section 7.2.4,
/// pag. 201. Low-bypass engines have maximum and military ratings; high-bypass
/// ones have a single formulation. EngineParameters::bypass_ratio picks which,
/// with the book's threshold of BPR = 1.
///
class Turbofan : public Engine {
public:
    /// Below this bypass ratio the engine is treated as low-bypass, pag. 201.
    static constexpr float kLowBypassLimit = 1.0f;

    explicit Turbofan(const EngineParameters& p) : Engine(p)
    {}

    float lapse(const FlightCondition& fc) const override;
};

}   // End namespace dlfdm

#endif // DLFDM_TURBOFAN_H
