// -----------------------------------------------------------------------------
// InputHandler.h — manejo de entradas (Práctico 06)
//
// Traduce teclado y mouse a los datos que consumen los demás módulos:
//   - el MOUSE produce un CameraCommand para la cámara (Práctico 05);
//   - el TECLADO produce las posiciones de los cuatro comandos de la aeronave
//     (throttle, elevator, aileron, rudder) que consume el FDM (Práctico 06).
//
// Qué NO hace: no integra física, no dibuja, no conoce a CameraSystem y no
// sabe qué es un alerón: entrega cuatro números y un comando de cámara.
//
// Decisiones de diseño:
//   1. MOUSE por POLLING (control continuo, P05): botón izquierdo = orbitar,
//      botón derecho (vertical) = zoom. Convención "orbitar alrededor".
//   2. TECLADO por POLLING (control continuo, P06): los cuatro mandos se leen
//      una vez por cuadro. Teclas:
//        ↑ / ↓  -> elevador (cabeceo)
//        → / ←  -> alerones (alabeo)
//        E / Q  -> timón de dirección (guiñada)
//        W / S  -> potencia (throttle)
//      CONVENCIÓN: la tecla representa el BASTÓN (no la nariz), como en un
//      avión real: ↑ = empujar el bastón = baja la nariz (δe > 0). Ver la
//      tabla de signos en el .cpp.
//   3. RAMPA + CENTRADO AUTOMÁTICO:
//      - mientras la tecla está apretada, el comando CAMBIA a velocidad fija
//        (rate*dt): rampa de primer orden, lo que pide la guía.
//      - al soltar, las SUPERFICIES (elevador, alerones, timón) vuelven solas a
//        su posición NEUTRA con un retorno de resorte/exponencial (rápido al
//        principio, se frena al acercarse): el elevador vuelve al valor de
//        trim; alerones y timón a 0.
//      - la POTENCIA (throttle) NO centra: se queda donde se dejó (se reduce
//        a mano, como un acelerador).
//      El centrado de las superficies es el comportamiento real (el bastón
//      vuelve solo); la guía lo menciona como el natural y deja el centrado
//      manual como la alternativa simple.
//   4. CONDICIÓN INICIAL (set_controls): el FDM arranca en un trim que incluye
//      una deflexión de elevador distinta de cero. Si InputHandler arrancara
//      en cero, en el primer cuadro el avión se descompensaría solo. Por eso
//      main carga el trim con set_controls() antes del loop.
//   5. Los deltas del mouse son por PÍXEL (independientes de los FPS). dt se
//      usa para las RAMPIAS del teclado (que sí son dependientes del tiempo).
// -----------------------------------------------------------------------------

#pragma once

#include <GLFW/glfw3.h>

#include <dlfdm/defines.h>

#include "core/CameraCommand.h"

class InputHandler {
public:
    // Lee el estado del mouse y del teclado. Una vez por cuadro.
    void update(GLFWwindow* window, float dt);

    // Posiciones actuales de los cuatro comandos de la aeronave (lo que
    // consume el FDM).
    const dlfdm::ControlInputs& controls() const { return controls_; }

    // Fija las posiciones iniciales de los comandos (la condición de trim) y,
    // a la vez, la posición NEUTRA a la que vuelven las superficies al soltar
    // (el elevador vuelve al valor de trim; alerones y timón a 0).
    void set_controls(const dlfdm::ControlInputs& c) {
        controls_        = c;
        controls_neutro_ = c;
    }

    // Copia los topes de deflexión del modelo (min/max de cada superficie)
    // para acotar las rampas. InputHandler no interpreta qué es cada uno.
    void set_limits(const dlfdm::AircraftParameters& params);

    // Comando de cámara del mouse (Práctico 05).
    CameraCommand camera_cmd() const { return camera_cmd_; }

private:
    // --- Mouse (Práctico 05): cámara orbital ---
    double        prev_x_ = 0.0;
    double        prev_y_ = 0.0;
    bool          primer_cuadro_ = true;
    CameraCommand camera_cmd_ {};

    // --- Teclado (Práctico 06): comandos de la aeronave ---
    dlfdm::ControlInputs controls_ {};          // [rad] y throttle [0,1]
    dlfdm::ControlInputs controls_neutro_ {};   // posición neutra (trim)
    float min_elev_ = -0.35f, max_elev_ = 0.35f; // topes (se copian del modelo)
    float min_ail_  = -0.35f, max_ail_  = 0.35f;
    float max_rud_  = 0.35f;                     // el timón es simétrico: ±max
};
