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
//
// Práctico 08 — TEXTURAS. El código de dibujo necesita saber además QUÉ
// textura usa cada pieza (o si sigue con color plano). Decisión: un PUNTERO
// NO DUEÑO a una Texture (la propone la guía). nullptr = color plano. Igual que
// `mesh`, apunta a algo que vive en otro lado (las texturas las posee main) y
// el RenderItem se arma y se consume dentro del mismo cuadro. Alternativas
// descartadas: un identificador numérico (el dibujo tendría que buscar la
// textura en una tabla) o un shader distinto por tipo de pieza (multiplica
// programas para un dato que es solo "qué imagen").
// -----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>     // glm::mat4, vec3, vec2: matriz de modelo, color y escala de textura

#include "core/Mesh.h"     // Mesh: la malla que se dibuja (puntero, no es dueño)
#include "core/Texture.h"  // Texture: la textura que se usa (puntero, no es dueño)

struct RenderItem {
    const Mesh* mesh = nullptr;          // malla en la GPU (VAO/VBO/EBO); referencia, no propia
    glm::mat4   model = glm::mat4(1.0f); // matriz de modelo del objeto (calculada en CPU)
    glm::vec3   color = glm::vec3(1.0f); // color de la pieza (uColor)
    // Geomorphing (Práctico 07, LOD, uMorph): (paso del nivel, media extensión en
    // celdas, ancho de transición en celdas). paso = 0 -> SIN morph (el avión
    // y cualquier malla que no sea un nivel del clipmap).
    glm::vec3   morph = glm::vec3(0.0f);
    // Práctico 08: textura de la pieza (no propia). nullptr -> color plano.
    // Con textura, el shader de la aeronave usa  color * textura  (la textura
    // TIÑE el color: aporta detalle), y el del terreno usa la textura sola.
    const Texture* textura = nullptr;
    // Práctico 08: cuántas veces se repite la textura sobre la pieza en (u, v).
    // (1,1) = una sola copia sobre cada cara (lo que se usa para VERIFICAR las
    // coordenadas). El terreno no la usa: su escala es uTamTesela.
    glm::vec2   escala_uv = glm::vec2(1.0f);
};
