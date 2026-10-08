// -----------------------------------------------------------------------------
// FlightData.h — datos de vuelo compartidos (Práctico 06)
//
// Estructura de DATOS (sin lógica) que lleva la salida del FDM al sistema de
// la escena. Es el "FlightData" de la arquitectura del proyecto: lo escribe la
// capa del FDM y lo consume la escena (el avión).
//
// El FDM entrega posición y actitud en el sistema NED (North, East, Down),
// mientras que la escena vive en un sistema con Z arriba y el modelo del avión
// construido con la nariz hacia -X, el ala derecha hacia +Y y el arriba +Z
// (Práctico 04). Ninguno de los dos sistemas está "mal": son convenciones
// distintas y hay que hacer una transformación en UN solo lugar (to_world).
//
// Mapeo de POSICIÓN (NED -> escena), elegido para que en la condición de trim
// (rumbo norte, psi=0) la nariz del modelo (-X) apunte al norte y el ala
// derecha (+Y) al este:
//      x_escena = -norte   (el norte cae sobre -X)
//      y_escena =  este    (el este cae sobre +Y)
//      z_escena = -abajo   (la altitud es +Z, hacia arriba)
// Se niegan dos de los tres ejes (nunca uno solo): negar uno daría una
// reflexión y el mundo saldría espejado.
//
// Mapeo de ACTITUD: los ángulos (phi, theta, psi) del FDM se pasan tal cual y
// se componen en la escena con los ejes adaptados al modelo (ver Aircraft::
// update): guiñada alrededor de +Z (negando psi), cabeceo alrededor de +Y y
// alabeo alrededor del eje de la nariz (-X). Se verificó por cálculo que con
// eso: theta>0 sube la nariz, phi>0 baja el ala derecha y psi>0 gira la nariz
// hacia la derecha (convención aeronáutica).
// -----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>

#include <dlfdm/defines.h>

struct FlightData {
    glm::vec3 position = glm::vec3(0.0f);   // posición en la ESCENA (Z arriba)
    float phi   = 0.0f;                     // [rad] alabeo  (roll)
    float theta = 0.0f;                     // [rad] cabeceo (pitch)
    float psi   = 0.0f;                     // [rad] guiñada (yaw)
};

// Transformación NED -> escena. Única y centralizada para evitar errores al
// copiarla en distintos módulos (recomendación de la guía).
inline FlightData to_world(const dlfdm::AircraftState& ned)
{
    FlightData gl;
    gl.position.x = -ned.inertial_position.x;   // -norte
    gl.position.y =  ned.inertial_position.y;   //  este
    gl.position.z = -ned.inertial_position.z;   //  arriba (-abajo)

    // Los ángulos no cambian de signo acá: la adaptación (ejes y sentidos) se
    // resuelve al componer la pose del modelo en Aircraft::update.
    gl.phi   = ned.phi;
    gl.theta = ned.theta;
    gl.psi   = ned.psi;
    return gl;
}
