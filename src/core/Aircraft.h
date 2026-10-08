// -----------------------------------------------------------------------------
// Aircraft.h — modelo de la aeronave (Prácticos 04 y 06)
//
// La aeronave es UNA composición de primitivas que forman un solo objeto
// (a diferencia de la escena del Práctico 03, donde cada pieza era un objeto
// independiente). Este módulo es el DUEÑO de esa composición: de las mallas
// y de las matrices que ubican cada pieza dentro del modelo.
//
// Sistema de referencia del MODELO (decisión 1):
//   - origen en la NARIZ
//   - X+ hacia la cola (hacia atrás): las traslaciones quedan positivas
//   - Y+ hacia el ala derecha
//   - Z+ hacia arriba
//   (mano derecha, convención aeronáutica de la clase)
//
// Dos niveles de jerarquía (decisión 2, no hace falta un árbol todavía):
//   Mlocal(pieza) : FIJA, describe la pieza dentro del modelo; se arma
//                   una sola vez en init().
//   Mpose         : posición + orientación del avión en el mundo; se
//                   recalcula en cada cuadro en update().
//   Mmundo(pieza) = Mpose * Mlocal   ->  lo compone collect().
//
// Decisiones de diseño:
//   1. La cantidad de MALLAS y de MATRICES LOCALES no coincide: piezas
//      distintas pueden compartir malla (las cuatro placas —ala izquierda,
//      ala derecha, estabilizador horizontal y deriva— usan la MISMA malla
//      del cubo) y difieren solo en su matriz local. En este modelo hay
//      4 mallas y 7 piezas (y 7 matrices locales). La separación es la que
//      va a permitir, más adelante, reemplazar el modelo por uno cargado
//      desde un archivo (.obj) sin tocar el resto del programa.
//   2. Ninguna pieza simétrica usa escalado negativo (rompe el winding):
//      el ala derecha es la MISMA malla con la traslación al lado opuesto.
//   3. update() no toca piezas ni matrices locales: solo arma pose_. Desde el
//      Práctico 06 recibe un FlightData (posición de escena + actitud del FDM,
//      ya pasados por to_world) y compone la pose con los ejes y sentidos
//      adaptados a este modelo: ver el detalle en el .cpp (update()).
//   4. collect() entrega RenderItems listos para dibujar: el resto del
//      programa no necesita saber cuántas piezas hay ni cómo están armadas.
// -----------------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

#include "core/FlightData.h"
#include "core/Mesh.h"
#include "core/RenderItem.h"

class Aircraft {
public:
    // Arma las piezas y sus matrices locales UNA sola vez: genera geometría
    // y sube las mallas a la GPU. No llamar por cuadro.
    void init();

    // Recalcula SOLO la pose a partir de los datos de vuelo (posición en la
    // ESCENA + actitud phi/theta/psi del FDM, ya pasados por to_world).
    void update(const FlightData& flight);

    // Agrega a items un RenderItem por pieza, con la transformación ya
    // compuesta (pose * local) y lista para dibujar.
    void collect(std::vector<RenderItem>& items) const;

private:
    std::vector<Mesh>          piezas_;    // mallas DISTINTAS (una por malla)
    std::vector<glm::mat4>     locales_;   // matriz local por PIEZA (fija)
    std::vector<glm::vec3>     colores_;   // color por PIEZA
    std::vector<std::size_t>   malla_de_;  // qué malla usa cada pieza
    glm::mat4                  pose_ {1.0f};
    glm::vec3                  punto_ref_ {0.0f};  // centro de gravedad
};
