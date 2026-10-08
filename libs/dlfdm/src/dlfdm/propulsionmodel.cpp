#include <dlfdm/propulsionmodel.h>

#include <cmath>

#include <dlfdm/tools.h>
#include <dlfdm/models/atmosphere/atmosphere.h>
#include <dlfdm/models/propulsion/turbojet.h>
#include <dlfdm/models/propulsion/turbofan.h>

namespace dlfdm {

/// Sea level reference of the standard atmosphere: T_0 and p_0 in the ram
/// ratios, General Aviation Aircraft Design, 1st Ed. - Gudmundsson, Eqs. (7-21)
/// and (7-22) pag. 200.
static constexpr float kSeaLevelTemperature = 288.15f;  // [K]
static constexpr float kSeaLevelPressure    = 101325.0f;// [Pa]
static constexpr float kGamma               = 1.4f;     // [-] air

static std::unique_ptr<Engine> make_engine(const EngineParameters& p)
{
    switch (p.type) {
    case EngineType::kTurbojet:
        return std::unique_ptr<Engine>(new Turbojet(p));
    case EngineType::kTurbofan:
        return std::unique_ptr<Engine>(new Turbofan(p));
    // No default case to handle undefined cases for enumeration items with
    // compiler warning [-Wswitch]
    }

    return nullptr;
}

PropulsionModel::PropulsionModel(const AircraftParameters& p, Atmosphere<float>* atm)
    : aircraft_data_(p), atmosphere_(atm), engine_(make_engine(p.engine)),
      flight_cond_{}, thrust_(0.0f), lapse_(0.0f)
{}

glm::vec3 PropulsionModel::calculate(const glm::vec3& body_velocity,
                                     const ControlInputs& controls)
{
    const float sound_speed = atmosphere_->get_sound_speed();

    flight_cond_.tas     = glm::length(body_velocity);
    flight_cond_.density = atmosphere_->get_density();
    flight_cond_.mach    = (sound_speed > 0.0f)
                         ? flight_cond_.tas / sound_speed
                         : 0.0f;

    // Ram ratios, shared by every engine model
    // General Aviation Aircraft Design, 1st Ed. - Gudmundsson
    // Eq. (7-21) and (7-22) pag. 200
    const float ram = 1.0f + 0.5f * (kGamma - 1.0f)
                           * flight_cond_.mach * flight_cond_.mach;

    flight_cond_.theta_0 = (atmosphere_->get_temperature() / kSeaLevelTemperature)
                         * ram;
    flight_cond_.delta_0 = (atmosphere_->get_pressure() / kSeaLevelPressure)
                         * std::pow(ram, kGamma / (kGamma - 1.0f));

    const float throttle = clamp(controls.throttle, 0.0f, 1.0f);

    lapse_  = engine_->lapse(flight_cond_);
    thrust_ = aircraft_data_.engine_count
            * engine_->thrust(flight_cond_, throttle);

    return glm::vec3(thrust_, 0.0f, 0.0f);
}

void PropulsionModel::log_all_titles(std::ostream &os, const char &sep) const
{
    os << "M [-]" << sep << "F/F_SL [-]" << sep << "thrust [N]";
}

void PropulsionModel::log_all(std::ostream &os, const char &sep) const
{
    os << flight_cond_.mach << sep << lapse_ << sep << thrust_;
}

}   // End namespace dlfdm
