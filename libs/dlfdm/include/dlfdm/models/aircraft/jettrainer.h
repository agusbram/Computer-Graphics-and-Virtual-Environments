#ifndef DLFDM_JETTRAINER_H
#define DLFDM_JETTRAINER_H

#pragma once
#include <dlfdm/defines.h>

namespace dlfdm {

namespace jettrainer {

///
/// \brief Available trim conditions for this aircraft model.
///
/// The name encodes what defines the point: atmosphere model, altitude [m] and
/// true airspeed [m/s].
///
enum class TrimCondition {
    kISA5000TAS150 = 0,
};

///
/// \brief Aircraft parameters of the jet trainer (Roskam's Airplane 'C').
///
AircraftParameters load_model(void);

///
/// \brief Initial state and controls for the requested trim condition.
///
TrimPoint get_trim_condition(TrimCondition condition);

} // namespace jettrainer

}   // End namespace dlfdm

#endif // DLFDM_JETTRAINER_H
