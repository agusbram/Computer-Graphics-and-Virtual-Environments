#include <dlfdm/models/propulsion/turbojet.h>

#include <cmath>

namespace dlfdm {

float Turbojet::lapse(const FlightCondition& fc) const
{
    const float TR = engine_data_.throttle_ratio;
    const float sqrt_mach = std::sqrt(fc.mach);

    float ratio;

    if (engine_data_.afterburner) {
        // Maximum thrust (afterburner on)
        // General Aviation Aircraft Design, 1st Ed. - Gudmundsson
        // Eq. (7-27) pag. 200
        ratio = 1.0f - 0.3f * (fc.theta_0 - 1.0f) - 0.1f * sqrt_mach;

        // Above the throttle ratio the turbine inlet temperature limits the
        // engine, Eq. (7-28) pag. 200
        if (fc.theta_0 > TR) {
            ratio -= 1.5f * (fc.theta_0 - TR) / fc.theta_0;
        }

        ratio *= fc.delta_0;
    } else {
        // Military thrust (afterburner off)
        // General Aviation Aircraft Design, 1st Ed. - Gudmundsson
        // Eq. (7-29) pag. 200
        ratio = 1.0f - 0.16f * sqrt_mach;

        // Eq. (7-30) pag. 200
        if (fc.theta_0 > TR) {
            ratio -= 24.0f * (fc.theta_0 - TR) / ((9.0f + fc.mach) * fc.theta_0);
        }

        ratio *= 0.8f * fc.delta_0;
    }

    // The correlations are fits, so they can go negative well outside the
    // envelope they were built for. An engine does not pull the aircraft back.
    return (ratio > 0.0f) ? ratio : 0.0f;
}

}   // End namespace dlfdm
