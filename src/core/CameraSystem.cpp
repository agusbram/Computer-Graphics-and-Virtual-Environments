// -----------------------------------------------------------------------------
// CameraSystem.cpp — cámara orbital (Práctico 05)
// -----------------------------------------------------------------------------

#include "core/CameraSystem.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace {

// Parámetros de la proyección en perspectiva (Unidad VII):
//   - fovy (θh): ángulo de visión VERTICAL, en RADIANES (45°). Es la apertura
//     del "embudo" (frustum) en vertical, medida desde la cámara: define
//     CUÁNTO del mundo entra en la pantalla. Ángulo grande = gran angular
//     (más mundo, más distorsión); ángulo chico = teleobjetivo (menos mundo,
//     sin distorsión). 45° aproxima el campo que el ojo humano distingue
//     bien. El ángulo horizontal θw NO se da aparte: sale del vertical más
//     el aspect (tan(θw/2) = aspect · tan(θh/2)). Como fovy queda FIJO, al
//     redimensionar la ventana el tamaño del objeto en pantalla depende solo
//     de la ALTURA (por eso en horizontal no se achica: el aspect absorbe el
//     cambio de ancho).
//   - near/far: planos de recorte. near no debe ser muy chico (precisión del
//     z-buffer, Unidad VIII). La recomendación es "near tan lejos como se
//     pueda sin eliminar lo que hace falta ver".
constexpr float kFovyRad = 0.785398163f;   // 45° (glm::radians(45.0f))
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

    // Acotar el estado. glm::clamp(v, min, max) "sujeta" el valor al rango
    // [min, max]: si v < min devuelve min, si v > max devuelve max, y si está
    // adentro lo deja igual (es un "si se pasa, lo aplasto al borde").
    //   pitch: no puede llegar a ±90° (se acota a ±0.9·π/2 ≈ ±81°): ahí el
    //     up y la dirección de vista serían colineales y lookAt produciría
    //     una matriz indefinida (el producto vectorial de adentro se anula ->
    //     la imagen "explota" o queda negra).
    //   distancia: entre un mínimo (no atravesar el modelo) y un máximo
    //     (no perderlo de vista). Acumular deltas sin límite rompería esto.
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
