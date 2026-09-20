// -----------------------------------------------------------------------------
// InputHandler.h — manejo de entradas (Práctico 05)
//
// Hoy solo el mouse, y solo para la cámara orbital. Convierte el movimiento
// del mouse (píxeles) en los deltas angulares/de distancia que consume
// CameraSystem (radianes y unidades de escena). NO mueve la cámara y NO
// conoce a CameraSystem: entrega un CameraCommand.
//
// Decisiones de diseño:
//   1. POLLING (no callback): mover la cámara es un control CONTINUO, así
//      que en cada cuadro se consulta el estado del mouse (glfwGetCursorPos
//      + glfwGetMouseButton). El callback queda para eventos discretos (el
//      redimensionado de la ventana).
//   2. MAPEO (dos de los tres controles al mouse; el tercero con un botón):
//        - botón IZQUIERDO arrastrado  -> yaw (eje X) y pitch (eje Y)
//        - botón DERECHO arrastrado    -> distancia (eje Y vertical)
//      Deltas: yaw_delta = −dx·g, pitch_delta = +dy·g, dist_delta = +dy·gd
//      Convención "orbitar alrededor": la cámara se mueve con el mouse y el
//      avión gira en sentido contrario (arrastrar → nariz a la izquierda;
//      arrastrar ↑ nariz baja). Arrastrar hacia abajo aleja.
//   3. PRIMER CUADRO: no hay "posición anterior", el delta vale 0 (solo se
//      guarda la posición para el cuadro siguiente).
//   4. Deltas por PÍXEL: como el arrastre acumula píxeles, el total no
//      depende de los FPS (la cantidad de píxeles recorridos es la misma
//      aunque cambie la cantidad de cuadros). El parámetro dt queda para
//      futuros controles dependientes del tiempo.
// -----------------------------------------------------------------------------

#pragma once

#include <GLFW/glfw3.h>

#include "core/CameraCommand.h"

class InputHandler {
public:
    // Lee el mouse por polling y deja los deltas del cuadro en command().
    void update(GLFWwindow* window, float dt);

    const CameraCommand& command() const { return cmd_; }

private:
    double prev_x_ = 0.0;
    double prev_y_ = 0.0;
    bool   primer_cuadro_ = true;
    CameraCommand cmd_ {};
};
