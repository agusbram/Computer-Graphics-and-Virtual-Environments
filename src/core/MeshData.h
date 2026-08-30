// -----------------------------------------------------------------------------
// MeshData.h
//
// Descripcion de una malla poligonal en datos PLANTON (sin OpenGL): la misma
// estructura la rellena el modulo primitives (cara CPU) y la consume Mesh
// (cara GPU). Asi ninguno de los dos depende de OpenGL para describir
// geometria.
//
// - Vertex: tupla de atributos del vertice. Hoy son 6 floats (3 posicion +
//   3 color). Las reglas de la reunion de un vertice (24 vs 8) dependen de
//   TODA la tupla: un vertice se comparte si y solo si coinciden todos los
//   atributos.
// - MeshData: la malla en si. "indices" es opcional: si viene vacia, la
//   malla se dibuja sin indices (glDrawArrays con vertices.size()).
// -----------------------------------------------------------------------------

#pragma once

#include <vector>

struct Vertex {
    float px, py, pz;  // posicion (x, y, z)
    float r, g, b;     // color  (r, g, b)
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices; // opcional: triangulos como indices
};