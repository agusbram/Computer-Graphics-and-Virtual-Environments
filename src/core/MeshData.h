// -----------------------------------------------------------------------------
// MeshData.h
//
// Descripcion de una malla poligonal en datos PLANOS (sin OpenGL): la misma
// estructura la rellena el modulo primitives (cara CPU) y la consume Mesh
// (cara GPU). Asi ninguno de los dos depende de OpenGL para describir
// geometria.
//
// - Vertex: tupla de atributos del vertice. La regla de la reunion de un
//   vertice (24 vs 8, 32 vs 18) depende de TODA la tupla: un vertice se
//   comparte si y solo si coinciden todos sus atributos.
// - MeshData: la malla en si. "indices" es opcional: si viene vacia, la
//   malla se dibuja sin indices (glDrawArrays con vertices.size()).
//
// Práctico 03 (Parte 1): cambio de la tupla.
//   SALE el color (pasa a ser un dato del OBJETO, via uniform) y ENTRAN:
//     - normal:      direccion perpendicular a la superficie en el vertice
//                    (unitaria). Se guarda aunque todavia no se use: la usa
//                    la iluminacion (Unidad IX) y el descarte de caras.
//     - tex_coords:  coordenadas (u, v) en [0,1] donde se lee el color de
//                    una textura. Se guarda aunque todavia no se use.
//   El contenido de la tupla decide cuantos vertices hay que duplicar: el
//   cubo sigue en 24 (la normal cambia en cada arista), pero el lateral de
//   un cilindro baja de 32 a 18 (la normal varia continua y se comparte).
//   Los tipos pasan a ser los de glm, que usa la misma notacion que GLSL.
// -----------------------------------------------------------------------------

#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 position;   // posicion (x, y, z)
    glm::vec3 normal;     // perpendicular a la superficie (unitaria)
    glm::vec2 tex_coords; // coordenadas de textura (u, v) en [0, 1]
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices; // opcional: triangulos como indices
};