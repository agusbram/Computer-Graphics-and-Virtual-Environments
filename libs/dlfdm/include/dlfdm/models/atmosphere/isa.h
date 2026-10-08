#ifndef ISA_H
#define ISA_H

#include <cmath>

#include <dlfdm/models/atmosphere/atmosphere.h>

namespace dlfdm {

template <typename T = double>
class ISA : public Atmosphere<T>
{
public:
    explicit ISA(void) :
        current_level_(kLevel0)
    {
//        temp_variation_ = kLevelGrad[current_level_];

//        this->altitude_     = T(0.0);
//        this->temperature_  = kTempRange[current_level_];
//        this->pressure_     = kPressureRange[current_level_];
//        this->density_      = calc_density();
//        this->snd_spd_      = calc_sound_speed();

        update(T(0.0));
    }

    ~ISA(void)
    {}

    void update(const T& altitude) final;

#ifdef ISA_TEST_UNIT
protected:
#else
private:
#endif
    typedef enum TempGrad { kConstante = 0, kLinear } TempGrad;
    typedef enum AtmLevel { kLevel0 = 0, kLevel1, kLevel2 , kLevel3 , kLevel4 , kLevel5, kLevel6, kLevelCount} AtmLevel;

    static constexpr T kAltitudeRange[] = {T(0.0),      T(11000.0), T(20000.0), T(32000.0), T(47000.0), T(51000.0), T(71000.0), T(84852.0)};
    static constexpr T kTempRange[]     = {T(288.15),   T(216.65),  T(216.65),  T(228.65),  T(270.65),  T(270.65),  T(214.65),  T(186.946)};
    static constexpr T kPressureRange[] = {T(101325.0), T(22632.0), T(5474.8),  T(868.01),  T(110.9),   T(66.938),  T(3.9564),  T(0.373403)};

    static constexpr T kTempGradient[kLevelCount]       = {T(-0.0065), T(0.0),     T(0.001), T(0.0028), T(0.0),     T(-0.0028), T(-0.002)};
    static constexpr TempGrad kLevelGrad[kLevelCount]   = {kLinear,    kConstante, kLinear,  kLinear,   kConstante, kLinear,    kLinear};

    static constexpr T kgAcc    = T(9.80665);   /// [m/s^2] Gravity accel
    static constexpr T kGamma   = T(1.4);       /// [-] Air specific heat ratio
    static constexpr T kRgas    = T(287.06);    /// [J/(kg·K)] Gas particular constant

    unsigned int current_level_;
    TempGrad temp_variation_;

    inline bool in_altitude_interval(const T& value, const unsigned int& idx) const {
        return (T(std::abs(value - kAltitudeRange[idx]) < T(1.0e-7) || value > kAltitudeRange[idx])
                && value < kAltitudeRange[idx + 1]);
    }

    inline T calc_temp_const_dTdh(void){
        return kTempRange[current_level_];
    }

    inline T calc_temp_linear_dTdh(void){
        return (kTempRange[current_level_] + kTempGradient[current_level_]
                * (this->altitude_ - kAltitudeRange[current_level_]));
    }

    inline T calc_pressure_const_dTdh(void){
        const T kExp = (T(-1.0)*kgAcc) * (this->altitude_ - kAltitudeRange[current_level_])
                        / (kRgas * kTempRange[current_level_]);
        return (kPressureRange[current_level_] * T(std::exp(kExp)));
    }

    inline T calc_pressure_linear_dTdh(void){
        const T kExp = (T(-1.0)*kgAcc) / (kTempGradient[current_level_] * kRgas);
        return (kPressureRange[current_level_]
                * T(std::pow(this->temperature_/kTempRange[current_level_],kExp)));
    }

    inline T calc_density(){
        return (this->pressure_ / (kRgas * this->temperature_));
    }

    inline T calc_sound_speed(){
        return T(std::sqrt(kGamma * kRgas * this->temperature_));
    }
};

template <typename T>
void ISA<T>::update(const T &altitude)
{
    // Search ISA level using altitude and configure computing data
    if( (altitude - kAltitudeRange[0]) < T(0.0) ||
        (altitude - kAltitudeRange[kLevelCount]) > T(1.0e-7) )
    {
        // Altitude out of range - Don't update the atmosphere
        return;
    }

    // Test continuous altitude change first: check neighbors. Then do a linear
    // search
    {
    bool interval_found = false;

    if( !in_altitude_interval(altitude,current_level_) ){
        // Upper neighbor
        if( (current_level_ + 1) <= (kLevelCount - 1) ){
            if( in_altitude_interval(altitude,current_level_ + 1) ){
                ++current_level_;
                interval_found = true;
            }
        }

        // Lower neighbor
        if( current_level_ > 0 && !interval_found ) {
            if( in_altitude_interval(altitude,current_level_ - 1) ){
                --current_level_;
                interval_found = true;
            }
        }

        // Search in all intervals
        if( !interval_found ) {
            for (unsigned int i = 0 ; i < kLevelCount ; i++) {
                if( in_altitude_interval(altitude,i) ){
                    current_level_ = i;
                    interval_found = true;
                    break;
                }
            }
        }
    }
    }

    // Update atmospheric parameters
    this->altitude_ = altitude;                     // Save current altitude
    temp_variation_ = kLevelGrad[current_level_];

    switch (temp_variation_) {
    case kConstante:
        this->temperature_  = calc_temp_const_dTdh();
        this->pressure_     = calc_pressure_const_dTdh();
        break;
    case kLinear:
        this->temperature_  = calc_temp_linear_dTdh();
        this->pressure_     = calc_pressure_linear_dTdh();
        break;
    }

    this->density_ = calc_density();
    this->snd_spd_ = calc_sound_speed();
}

} // namespace dlfdm

#endif // ISA_H
