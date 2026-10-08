#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

namespace dlfdm {

template <typename T>
class Atmosphere {
public:
    explicit Atmosphere(void) : altitude_(T(0.0))
    {}

    virtual ~Atmosphere(void)
    {}

    virtual void update(const T& altitude) = 0;

    inline T get_altitude(void) const       { return altitude_; }
    inline T get_temperature(void) const    { return temperature_; }
    inline T get_pressure(void) const       { return pressure_; }
    inline T get_density(void) const        { return density_; }
    inline T get_sound_speed(void) const    { return snd_spd_; }

protected:
    T altitude_;        /// [m] current altitude used to calculate atm params
    T temperature_;     /// [K] temperature @ altitude_
    T pressure_;        /// [Pa] atm pressure @ altitude_
    T density_;         /// [kg/m3] atm density @ altitude_
    T snd_spd_;         /// [m/s] sound speed @ altitude_
};

} // namespace dlfdm

#endif // ATMOSPHERE_H
