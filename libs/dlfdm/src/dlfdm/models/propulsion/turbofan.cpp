#include <dlfdm/models/propulsion/turbofan.h>

#include <cmath>

namespace dlfdm {

float Turbofan::lapse(const FlightCondition& fc) const
{
    const float TR = engine_data_.throttle_ratio;

    float ratio;

    if (engine_data_.bypass_ratio < kLowBypassLimit) {
        if (engine_data_.afterburner) {
            // Low bypass, maximum thrust (afterburner on)
            // General Aviation Aircraft Design, 1st Ed. - Gudmundsson
            // Eq. (7-31) pag. 201
            ratio = 1.0f;

            // Eq. (7-32) pag. 201
            if (fc.theta_0 > TR) {
                ratio -= 3.5f * (fc.theta_0 - TR) / fc.theta_0;
            }

            ratio *= fc.delta_0;
        } else {
            // Low bypass, military thrust (afterburner off)
            // Eq. (7-33) pag. 201
            ratio = 1.0f;

            // Eq. (7-34) pag. 201
            if (fc.theta_0 > TR) {
                ratio -= 3.8f * (fc.theta_0 - TR) / fc.theta_0;
            }

            ratio *= 0.6f * fc.delta_0;
        }
    } else {
        // High bypass: a single formulation, no afterburner rating
        // General Aviation Aircraft Design, 1st Ed. - Gudmundsson
        // Eq. (7-35) pag. 201
        ratio = 1.0f - 0.49f * std::sqrt(fc.mach);

        // Eq. (7-36) pag. 201. Note the denominator is (1.5 + M), it is not
        // divided by theta_0 as in the other models.
        if (fc.theta_0 > TR) {
            ratio -= 3.0f * (fc.theta_0 - TR) / (1.5f + fc.mach);
        }

        ratio *= fc.delta_0;
    }

    // The correlations are fits, so they can go negative well outside the
    // envelope they were built for. An engine does not pull the aircraft back.
    return (ratio > 0.0f) ? ratio : 0.0f;
}

}   // End namespace dlfdm
