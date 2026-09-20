// -----------------------------------------------------------------------------
// CameraSystem.h — cámara orbital (Práctico 05)
//
// Administra el estado de la cámara de la escena y produce, por cuadro, el
// par de matrices de visualización (vista + proyección). El resto del
// programa no necesita saber si la cámara es de órbita, de cabina o fija:
// solo pide CameraData y lo usa. Esa separación va a permitir, más adelante,
// agregar otros tipos de vista sin tocar el resto del programa.
//
// Cámara ORBITAL (lo que pide la guía): tres números alrededor de un punto
// objetivo — yaw (acimut), pitch (latitud) y distancia —. La posición de la
// cámara sale de pasar esféricas a cartesianas y siempre mira al objetivo:
//
//     eye = objetivo + d · (cos p·cos y,  cos p·sin y,  sin p)
//     view = glm::lookAt(eye, objetivo, up = (0, 0, 1))
//
// El eje POLAR de la órbita es Z (la vertical de esta escena), no Y como en
// la fórmula genérica de la guía: pitch eleva en Z y yaw rota en el plano
// X-Y.
//
// Decisiones de diseño:
//   1. NO lee el mouse: recibe deltas ya interpretados (CameraCommand). El
//      que lee el mouse es InputHandler; acá solo se acumulan y se acotan.
//   2. NO dibuja: no toca OpenGL (solo glm), por eso vive en core/ sin GL.
//   3. NO sabe qué hay en la escena: recibe un punto al que mirar y nada más.
//   4. El up se fija en (0,0,1): la vertical de esta escena es el eje Z
//      (convención de la aeronave X+ cola, Y+ ala derecha, Z+ arriba). Con
//      up=(0,1,0) el avión quedaría "de costado".
//   5. El pitch se acota a ±81° (= 0.9·π/2): si llegara a ±90°, el up y la
//      dirección de vista serían colineales y lookAt daría una matriz
//      indefinida (producto vectorial nulo).
//   6. La distancia se acota entre un mínimo y un máximo para que la cámara
//      no atraviese el modelo ni lo pierda de vista.
//   7. Proyección en PERSPECTIVA con glm::perspective (fovy 45°, near 0.1,
//      far 100). Se recalcula en set_viewport() cuando cambia el tamaño de
//      la ventana (el aspect depende del framebuffer).
// -----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>

#include "core/CameraCommand.h"
#include "core/CameraData.h"

class CameraSystem {
public:
    // Arma la proyección inicial a partir del tamaño del framebuffer.
    CameraSystem(int width, int height);

    // Recalcula la VISTA a partir del objetivo y de los deltas del usuario.
    // `angulos_avion` hoy NO se usa (la cámara orbital solo mira un punto);
    // queda reservado para una futura vista de cabina.
    void update(const glm::vec3& pos_avion,
                const glm::vec3& angulos_avion,
                const CameraCommand& cmd);

    // Recalcula la PROYECCIÓN al cambiar el tamaño de la ventana. Si el alto
    // llega en 0 (ventana minimizada) se usa 1 para no dividir por cero.
    void set_viewport(int width, int height);

    const CameraData& data() const { return data_; }

private:
    float yaw_       = 0.785f;  // acimut [rad] (~45°, arranca en 3/4)
    float pitch_     = 0.35f;   // latitud [rad] (~20°, algo elevada)
    float distancia_ = 5.0f;    // distancia al objetivo [unidades de escena]
    CameraData data_ {};
};
