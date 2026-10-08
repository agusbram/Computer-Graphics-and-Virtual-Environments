// -----------------------------------------------------------------------------
// InputHandler.cpp — mouse (Práctico 05) + teclado (Práctico 06)
// -----------------------------------------------------------------------------

#include "core/InputHandler.h"

#include <algorithm>   // std::clamp

namespace {

// --- Mouse: ganancias píxel -> unidades ---
// Con 0.008, un arrastre de ~400 px rota ~3.2 rad (~180°).
constexpr float kGananciaAngular   = 0.008f;   // [rad/px]
constexpr float kGananciaDistancia = 0.03f;    // [unidades de escena/px]

// --- Teclado: rampa de velocidad constante + centrado automático ---
constexpr float kRateSuperficies = 0.25f;   // [rad/s] al mantener la tecla
constexpr float kRateThrottle    = 0.30f;   // [1/s]
constexpr float kCentrado        = 5.0f;    // [1/s] velocidad del resorte de retorno

// Rampa de un comando.
//   dir = +1 / -1 según qué tecla está apretada (0 = ninguna)
//   - con tecla (dir != 0): el valor cambia a velocidad constante (rate*dt).
//   - sin tecla y con centrado: vuelve al NEUTRO con un retorno de
//     resorte/exponencial (rápido al principio, se frena al acercarse).
//   - sin tecla y sin centrado: queda donde estaba (p. ej. el throttle).
// Devuelve el valor acotado a [vmin, vmax].
float rampa(float valor, int dir, float rate, float dt,
            float vmin, float vmax, float neutro, bool centra)
{
    if (dir != 0) {
        valor += static_cast<float>(dir) * rate * dt;
    } else if (centra) {
        const float dif = neutro - valor;
        if (dif > -1e-4f && dif < 1e-4f) {
            valor = neutro;                       // ya llegó: se fija
        } else {
            valor += dif * kCentrado * dt;        // retorno de resorte
        }
    }
    return std::clamp(valor, vmin, vmax);
}

}   // namespace

void InputHandler::update(GLFWwindow* window, float dt)
{
    // =================================================================
    // 1) MOUSE (Práctico 05): comando de cámara. Deltas por PÍXEL, ya
    //    independientes del tiempo (el arrastre acumula la misma cantidad de
    //    píxeles a cualquier FPS).
    // =================================================================
    double x = 0.0, y = 0.0;
    glfwGetCursorPos(window, &x, &y);

    camera_cmd_ = CameraCommand{};   // reinicia los deltas del cuadro

    if (primer_cuadro_) {
        primer_cuadro_ = false;      // no hay posición anterior: delta 0
    } else {
        const double dx = x - prev_x_;
        const double dy = y - prev_y_;

        const bool orbita = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)
                            == GLFW_PRESS;
        const bool zoom   = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)
                            == GLFW_PRESS;

        if (orbita) {
            // Convención "orbitar alrededor": la cámara se mueve con el mouse
            // y el avión gira al revés.
            camera_cmd_.yaw_delta   = -static_cast<float>(dx) * kGananciaAngular;
            camera_cmd_.pitch_delta =  static_cast<float>(dy) * kGananciaAngular;
        }
        if (zoom) {
            camera_cmd_.dist_delta = static_cast<float>(dy) * kGananciaDistancia;
        }
    }

    prev_x_ = x;
    prev_y_ = y;

    // =================================================================
    // 2) TECLADO (Práctico 06): los cuatro comandos de la aeronave. Mientras
    //    la tecla está apretada el comando cambia a velocidad constante; al
    //    soltarla, las superficies vuelven al neutro (resorte) y la potencia
    //    se queda donde se dejó.
    //
    //    Convención: la tecla representa el BASTÓN (como en un avión real).
    //    Signos (de la tabla de la guía):
    //      Elevador: empujar el bastón -> baja la nariz -> δe > 0
    //      Alerones: bastón a la derecha -> ala derecha abajo -> δa > 0
    //      Timón:    pedal derecho -> la nariz a la derecha -> δr < 0
    // =================================================================
    const int dir_elev = (glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS ? 1 : 0)
                       - (glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS ? 1 : 0);
    const int dir_ail  = (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ? 1 : 0)
                       - (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS ? 1 : 0);
    const int dir_rud  = (glfwGetKey(window, GLFW_KEY_Q)     == GLFW_PRESS ? 1 : 0) // pedal izq
                       - (glfwGetKey(window, GLFW_KEY_E)     == GLFW_PRESS ? 1 : 0); // pedal der
    const int dir_thr  = (glfwGetKey(window, GLFW_KEY_W)     == GLFW_PRESS ? 1 : 0)
                       - (glfwGetKey(window, GLFW_KEY_S)     == GLFW_PRESS ? 1 : 0);

    // Superficies: centran al neutro al soltar (elevador -> trim, a/a -> 0).
    controls_.elevator = rampa(controls_.elevator, dir_elev, kRateSuperficies, dt,
                               min_elev_, max_elev_, controls_neutro_.elevator, true);
    controls_.aileron  = rampa(controls_.aileron,  dir_ail,  kRateSuperficies, dt,
                               min_ail_,  max_ail_,  controls_neutro_.aileron,  true);
    controls_.rudder   = rampa(controls_.rudder,   dir_rud,  kRateSuperficies, dt,
                               -max_rud_, max_rud_,  controls_neutro_.rudder,   true);

    // Potencia: NO centra (se queda donde se dejó; se reduce a mano).
    controls_.throttle = rampa(controls_.throttle, dir_thr,  kRateThrottle,    dt,
                               0.0f, 1.0f, 0.0f, false);
}

void InputHandler::set_limits(const dlfdm::AircraftParameters& params)
{
    min_elev_ = params.min_elevator;
    max_elev_ = params.max_elevator;
    min_ail_  = params.min_aileron;
    max_ail_  = params.max_aileron;
    max_rud_  = params.max_rudder;
}
