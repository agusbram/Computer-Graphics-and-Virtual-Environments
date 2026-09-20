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
    // Los deltas por PÍXEL ya son independientes del tiempo: el arrastre
    // acumula la misma cantidad de píxeles a cualquier FPS (a más cuadros por
    // segundo, menos píxeles por cuadro, pero el total es el mismo). Por eso
    // dt no se usa HOY. Queda reservado para futuros controles dependientes
    // del tiempo, p. ej.: girar la cámara a 30°/s con una tecla
    // (yaw_delta = radians(30) * dt), inercia que se frena, o la integración
    // del FDM (velocidad * dt). Es la forma de que la velocidad NO dependa
    // de la máquina (la pista de la filmina: "gira a distinta velocidad").
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
            // dx = desplazamiento horizontal en píxeles; g = ganancia que
            // convierte píxeles -> radianes. Convención "ORBITAR ALREDEDOR"
            // (la cámara se mueve y el avión gira en sentido contrario):
            //   - arrastrar a la DERECHA (dx > 0) -> −dx < 0 -> la cámara se
            //     mueve a la derecha y la nariz gira a la IZQUIERDA.
            //   - arrastrar hacia ARRIBA (dy < 0) -> +dy < 0 -> la cámara
            //     sube y la nariz BAJA (mirás desde más arriba).
            // (Si se prefiere "el avión sigue al mouse", es el signo opuesto.)
            cmd_.yaw_delta   = -static_cast<float>(dx) * kGananciaAngular;
            cmd_.pitch_delta =  static_cast<float>(dy) * kGananciaAngular;
        }
        if (zoom) {
            // El zoom es el TERCER control, pero el mouse solo tiene 2 ejes:
            // mientras se mantiene el botón derecho, el movimiento VERTICAL
            // (dy) cambia la distancia. Usa gd (píxeles -> unidades de
            // escena), distinta de g (radianes), porque la distancia se mide
            // en unidades del mundo. Arrastrar hacia ABAJO (dy > 0) aleja.
            cmd_.dist_delta = static_cast<float>(dy) * kGananciaDistancia;
        }
    }

    prev_x_ = x;
    prev_y_ = y;
}
