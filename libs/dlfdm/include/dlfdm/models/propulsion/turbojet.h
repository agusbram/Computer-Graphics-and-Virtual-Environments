#ifndef DLFDM_TURBOJET_H
#define DLFDM_TURBOJET_H

#pragma once
#include <dlfdm/models/propulsion/engine.h>

namespace dlfdm {

///
/// \brief Turbojet thrust lapse with altitude and Mach number.
///
/// General Aviation Aircraft Design, Gudmundsson, S., 1st Ed., Section 7.2.3,
/// pag. 199-200. Maximum (afterburner on) and military (afterburner off)
/// ratings, selected by EngineParameters::afterburner.
///
class Turbojet : public Engine {
public:
    explicit Turbojet(const EngineParameters& p) : Engine(p)
    {}

    float lapse(const FlightCondition& fc) const override;
};

}   // End namespace dlfdm

#endif // DLFDM_TURBOJET_H
