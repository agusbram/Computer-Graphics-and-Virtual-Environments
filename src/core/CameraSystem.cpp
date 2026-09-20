// -----------------------------------------------------------------------------
// CameraSystem.cpp — cámara orbital (Práctico 05)
// -----------------------------------------------------------------------------

#include "core/CameraSystem.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace {

// Parámetros de la proyección en perspectiva (Unidad VII):
//   - fovy (θh): ángulo de visión vertical, en RADIANES (45°).
//   - near/far: planos de recorte. near no debe ser muy chico (precisión del
//     z-buffer, Unidad VIII). La recomendación es "near tan lejos como se
//     pueda sin eliminar lo que hace falta ver".
constexpr float kFovyRad = 0.785398163f;   // 45°
constexpr float kNear    = 0.1f;
constexpr float kFar     = 100.0f;

// Límites del estado de la órbita.
constexpr float kPitchMax = 1.413716694f;  // 0.9 * pi/2 (~81°)
constexpr float kDistMin  = 2.0f;          // no atravesar el modelo
constexpr float kDistMax  = 40.0f;         // no perderlo de vista

}   // namespace

CameraSystem::CameraSystem(int width, int height)
{
    set_viewport(width, height);
}

void CameraSystem::update(const glm::vec3& objetivo,
                          const glm::vec3& angulos_avion,
                          const CameraCommand& cmd)
{
    // angulos_avion hoy no se usa: la cámara orbital solo mira un punto.
    (void)angulos_avion;

    yaw_       += cmd.yaw_delta;
    pitch_     += cmd.pitch_delta;
    distancia_ += cmd.dist_delta;

    // Acotar el estado. El pitch no puede llegar a ±90°: ahí el up y la
    // dirección de vista son colineales y lookAt produce una matriz
    // indefinida (el producto vectorial de adentro se anula).
    pitch_     = glm::clamp(pitch_, -kPitchMax, kPitchMax);
    distancia_ = glm::clamp(distancia_, kDistMin, kDistMax);

    // De esféricas a cartesianas: la posición de la cámara relativa al
    // objetivo, y de ahí la matriz de vista. El eje POLAR de la órbita es Z
    // (la vertical de esta escena), no Y como en la fórmula genérica de la
    // guía: pitch eleva en Z y yaw rota en el plano X-Y.
    const float cp = std::cos(pitch_), sp = std::sin(pitch_);
    const float cy = std::cos(yaw_),   sy = std::sin(yaw_);

    const glm::vec3 eye = objetivo
        + distancia_ * glm::vec3(cp * cy, cp * sy, sp);

    // up = (0,0,1): coincide con el eje polar de la órbita (la vertical de la
    // escena). Con up=(0,1,0) el avión quedaría "de costado" (las alas
    // apuntando hacia arriba en pantalla).
    data_.view = glm::lookAt(eye, objetivo, glm::vec3(0.0f, 0.0f, 1.0f));
}

void CameraSystem::set_viewport(int width, int height)
{
    // Al minimizar la ventana el alto puede llegar en 0: el aspect quedaría
    // indefinido. Se usa 1 como guarda (la proyección se recalcula de nuevo
    // cuando la ventana se restaure).
    if (height == 0) {
        height = 1;
    }
    const float aspect = static_cast<float>(width) / static_cast<float>(height);

    data_.projection = glm::perspective(kFovyRad, aspect, kNear, kFar);
}
