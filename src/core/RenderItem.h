// -----------------------------------------------------------------------------
// RenderItem.h
//
// Item de dibujo: lo único que el resto del programa necesita para dibujar
// UNA pieza ya transformada. Lo produce Aircraft::collect() (Práctico 04).
//
// La malla es una REFERENCIA (no es propia): las mallas las posee el
// Aircraft; el RenderItem se arma y se consume dentro del mismo cuadro.
// La matriz de modelo ya viene COMPUESTA (pose * local), lista para el
// shader. El color viaja a uColor (vec3): este proyecto usa vec3 y no el
// vec4 del molde de la guía porque el fragment shader no usa el canal alfa.
// -----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>

#include "core/Mesh.h"

struct RenderItem {
    const Mesh* mesh = nullptr;          // malla a dibujar (referencia, no es propia)
    glm::mat4   model = glm::mat4(1.0f); // transformacion modelo -> mundo, YA compuesta
    glm::vec3   color = glm::vec3(1.0f); // color de la pieza (uColor)
};
