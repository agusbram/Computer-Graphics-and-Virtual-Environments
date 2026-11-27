// -----------------------------------------------------------------------------
// DiaNoche.h — el CICLO DÍA/NOCHE (Práctico 09, opcional 2)
//
// Lo que pide la guía: "la dirección del sol puede variar con el tiempo de la
// simulación: girando alrededor de un eje horizontal, el sol sale, sube hasta
// el mediodía, baja y se pone". Y para que sea creíble:
//   - el color del sol se vuelve más cálido (anaranjado) cerca del horizonte;
//   - debajo del horizonte hay que APAGARLO: Phong no tiene sombras, así que
//     la panza del avión quedaría iluminada a través del terreno;
//   - la luz ambiente y el cielo se oscurecen (paso 2).
//
// Este módulo es el RELOJ del ciclo y las cuentas que salen de la hora. Se
// hace en 4 pasos (ver Practico09-Decisiones.md):
//   1. el sol que se mueve: reloj, recorrido, color y apagado   <- ESTE
//   2. la noche: cielo, niebla y ambiente más oscuros, y la luna;
//   3. el tinte del cielo en el crepúsculo;
//   4. las estrellas.
//
// LA HORA ES UN ÁNGULO: cuánto giró el sol desde que salió.
//     0° = sale (6:00)            90° = mediodía (12:00)
//   180° = se pone (18:00)       270° = medianoche (0:00)
// Un giro completo son 24 h: 15° por hora, y 5° son 20 minutos.
//
// Decisiones (25 a 31; acá, las del paso 1):
//   25. RELOJ con tres velocidades (tecla D): detenido -> normal (un día en
//       2 min) -> rápido (un día en 30 s). Avanza con el tiempo de la
//       SIMULACIÓN: la pausa (Espacio) también lo frena. O/P mueven la hora de
//       a 20 min y H vuelve a la hora del sol pintado. Arranca DETENIDO en esa
//       hora: al abrir el programa, la luz viene de donde se ve el sol.
//   26. RECORRIDO: un eje HORIZONTAL (lo que dice la guía), orientado para que
//       el sol pase por el sol PINTADO del cubemap. Sale del lado opuesto al
//       pintado, pasa por el cenit al mediodía y se pone del lado del pintado,
//       que queda como "el sol de las 16:00" (29,8°): a esa hora el halo
//       coincide con él. Con el eje horizontal, al mediodía el sol pasa por el
//       cenit, como en el ecuador en un equinoccio.
//   27. COLOR: la luz se atenúa al cruzar el aire con la MISMA ley que la
//       niebla de la Parte 4, e^(-τ·camino), pero el aire dispersa más el azul
//       que el rojo (un τ por canal). Con el sol bajo, el camino por el aire
//       es mucho más largo y queda sobre todo el rojo. Ver masa_de_aire() y
//       transmitancia_aire().
//   31. UN MÓDULO PROPIO, sin dependencias: ni OpenGL ni otros módulos, solo
//       glm. Es un "sistema" como los de Arquitectura-Proyecto.md: main lo
//       avanza en cada cuadro y copia lo que sale (la dirección y el color del
//       sol) a la luz de la escena (struct Iluminacion). Las cuentas se pueden
//       probar sin abrir una ventana: tests/main_test_dia_noche.cpp.
// -----------------------------------------------------------------------------

#pragma once

#include <string>        // std::string: la hora como texto ("16:00")

#include <glm/glm.hpp>   // glm::vec3, mat3: direcciones, colores y el giro del sol

class DiaNoche {
public:
    // Velocidades del reloj, en el orden en que las recorre la tecla D.
    enum class Reloj { Detenido, Normal, Rapido };

    // Arma el recorrido con la dirección HACIA el sol pintado del cubemap
    // (mundo, Z arriba, unitaria, por encima del horizonte; la mide el Skybox).
    // La hora arranca en la del sol pintado y el reloj, detenido.
    explicit DiaNoche(const glm::vec3& dir_sol_pintado);

    // --- El reloj -----------------------------------------------------------
    // Avanza la hora según la velocidad del reloj. dt = segundos de simulación
    // de este cuadro (main no lo llama con la pausa puesta). Detenido: nada.
    void  avanzar(float dt);
    // Tecla D: pasa a la velocidad siguiente y la devuelve.
    Reloj cambiar_reloj();
    // Teclas O/P: suma `grados` a la hora (negativo = atrasar). Da la vuelta:
    // la hora queda siempre en [0°, 360°).
    void  mover(float grados);
    // Tecla H: vuelve a la hora del sol pintado. No cambia la velocidad.
    void  volver_al_pintado();

    // --- Lo que sale de la hora -----------------------------------------------
    Reloj     reloj() const { return reloj_; }
    float     angulo() const { return angulo_; }   // la hora como ángulo, [0°, 360°)
    float     hora() const;                        // la hora en un reloj de 24 h, [0, 24)
    float     hora_pintado() const;                // la hora a la que el sol pasa por el pintado
    glm::vec3 eje() const { return eje_; }         // el eje de giro: horizontal, unitario
    // El GIRO del sol desde la salida: angulo_ grados alrededor de eje_. (El
    // paso 4 lo va a usar también para que las estrellas giren con el sol.)
    glm::mat3 rotacion() const;
    glm::vec3 direccion_sol() const;               // HACIA el sol: mundo, Z arriba, unitaria
    float     elevacion_sol() const;               // grados sobre el horizonte (negativa de noche)
    // El color del sol que llega al suelo: el de MEDIODÍA (`color_mediodia`;
    // 0,9 por la decisión 18) atenuado por el aire y, debajo del horizonte,
    // apagado. Es LINEAL (una proporción de luz), como Iluminacion::sol_color.
    glm::vec3 color_sol(const glm::vec3& color_mediodia) const;

private:
    glm::vec3 eje_;              // eje de giro: horizontal, perpendicular al plano del recorrido
    glm::vec3 salida_;           // hacia el sol a las 6:00: en el horizonte, del lado opuesto al pintado
    float     angulo_pintado_;   // la hora (ángulo) en la que el sol pasa por el pintado
    float     angulo_;           // la hora actual (ángulo), en [0°, 360°)
    Reloj     reloj_ {Reloj::Detenido};
};

// --- Funciones libres: el color del sol (decisión 27) -----------------------
// Son puras (no guardan estado): se prueban en tests/main_test_dia_noche.cpp.

// MASA DE AIRE: cuánto aire atraviesa la luz del sol hasta el suelo, relativo
// al que atraviesa con el sol en el cenit: 1 en el cenit, ~40 en el horizonte.
float masa_de_aire(float elevacion_grados);

// Qué parte de la luz llega al suelo, por canal (R, G, B), relativa a la del
// mediodía: e^(-τ·(masa - 1)). Vale (1, 1, 1) en el cenit y, cuando el sol
// baja, cae mucho más rápido en el azul que en el rojo.
glm::vec3 transmitancia_aire(float elevacion_grados);

// Cuánto del sol está encendido: 1 de día y 0 de noche. En el medio, mientras
// el disco (2° de diámetro) se hunde en el horizonte (centro entre +1° y -1°),
// baja suave con smoothstep.
float encendido_sol(float elevacion_grados);

// La hora de un reloj de 24 h como texto "hh:mm" ("16:00").
std::string hora_texto(float hora);
