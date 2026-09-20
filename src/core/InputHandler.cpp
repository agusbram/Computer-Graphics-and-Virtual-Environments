// -----------------------------------------------------------------------------
// InputHandler.cpp — manejo del mouse (Práctico 05)
// -----------------------------------------------------------------------------

#include "core/InputHandler.h"

namespace {

// Ganancia px -> rad. Con 0.008, un arrastre de ~400 px rota ~3.2 rad (~180°).
constexpr float kGananciaAngular = 0.008f;

// Ganancia px -> unidades de escena (distancia al objetivo).
constexpr float kGananciaDistancia = 0.03f;

}   // namespace

void InputHandler::update(GLFWwindow* window, float dt)
{
    // Los deltas por píxel ya son independientes del tiempo (el arrastre
    // acumula la misma cantidad de píxeles a cualquier FPS); dt queda para
    // futuros controles dependientes del tiempo.
    (void)dt;

    double x = 0.0, y = 0.0;
    glfwGetCursorPos(window, &x, &y);

    cmd_ = CameraCommand{};   // reinicia los deltas del cuadro

    if (primer_cuadro_) {
        primer_cuadro_ = false;   // no hay posición anterior: delta 0
    } else {
        const double dx = x - prev_x_;
        const double dy = y - prev_y_;

        const bool orbita = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)
                            == GLFW_PRESS;
        const bool zoom   = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)
                            == GLFW_PRESS;

        if (orbita) {
            cmd_.yaw_delta   = static_cast<float>(dx) * kGananciaAngular;
            cmd_.pitch_delta = static_cast<float>(-dy) * kGananciaAngular;
        }
        if (zoom) {
            cmd_.dist_delta = static_cast<float>(dy) * kGananciaDistancia;
        }
    }

    prev_x_ = x;
    prev_y_ = y;
}
