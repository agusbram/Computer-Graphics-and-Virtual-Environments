#ifndef HUDDEF_H
#define HUDDEF_H

namespace hud {

// Estructura para datos de puntos de navegación
typedef struct Waypoint {
    float latitude;     // [deg]
    float longitud;     // [deg]
    float altitude;     // [ft]
} Waypoint;

// Estructura para datos de vuelo
typedef struct FlightData {
    float pitch;           // [deg]
    float roll;            // [deg]
    float heading;         // [deg]
    float altitude;        // [ft]
    float speed;           // [kt]
    float vertical_speed;  // [ft/min]
    Waypoint waypoint;
} FlightData;

} // namespace hud

#endif // HUDDEF_H
