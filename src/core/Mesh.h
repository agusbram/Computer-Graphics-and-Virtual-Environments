// -----------------------------------------------------------------------------
// Mesh.h
//
// Administra la vida de la geometria en la GPU: el VAO (como se interpretan
// los buffers), el VBO (tuplas de vertices) y el EBO (indices); la malla
// viene en datos planos (MeshData) del lado del CPU.
//
// Los tres objetos de OpenGL se manejan por nombres (enteros que apuntan a
// memoria de la GPU). Copiar el entero no copia el objeto, y perder el entero
// no lo libera: lo deja huerfano. De ahi salen las tres decisiones:
//
// Decisiones de diseño (Practico 02, Parte 1):
//   1. DUEÑO UNICO. La copia esta deshabilitada: dos duenos liberando el
//      mismo recurso es un doble free. El movimiento SI se permite y
//      traspasa la propiedad (el movido queda vacio).
//   2. EL OBJETO MOVIDO QUEDA EN CERO (vao_ = vbo_ = ebo_ = 0U). Si no, dos
//      objetos apuntarian al mismo recurso y cada destructor lo liberaria
//      dos veces (y usar el objeto original "por accidente" seria misterioso).
//   3. clear() SIN BANDERAS: glDelete*(0) es legal y no hace nada, asi que
//      borrar dos veces no rompe; despues de borrar los IDs se vuelven a
//      cero y el objeto queda "vacio" y reusable (load() llama clear() al
//      empezar).
//   4. count() = CANTIDAD DE INDICES: es el dato que espera glDrawElements.
//      Para una malla NO indexada (el EBO no se crea) count() cae a la
//      cantidad de vertices y el llamador dibuja con glDrawArrays.
//   5. DESPLAZAMIENTOS DE ATRIBUTOS CON offsetof (no 0 y 12 a mano): si la
//      tupla Vertex gana un campo en el medio, no hay que tocar nada. Si
//      manana el vertice tuviera un atributo mas, solo se agrega un par
//      AttribFormat + EnableVertexArrayAttrib en load() (y location en el
//      vertex shader).
//   6. El EBO se conecta al VAO por su RANURA DEDICADA
//      (glVertexArrayElementBuffer): el VAO "recuerda" cual es el buffer de
//      indices, sin bindear nada.
//
// Ningun glDelete* existe fuera de esta clase (ni sus funciones, ni main).
// -----------------------------------------------------------------------------

#pragma once

#include "core/MeshData.h"

class Mesh {
public:
    Mesh() = default;
    ~Mesh();                                    // libera lo que posee

    Mesh(const Mesh&) = delete;                 // copiar rompe el dueño único
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;                // mover: se traspasa la propiedad
    Mesh& operator=(Mesh&& other) noexcept;

    void load(const MeshData& data);            // sube a la GPU y arma el VAO
    void clear(void);

    // Getters: devuelven los miembros privados (encapsulamiento). Los valores
    // se asignan en load(): vao_ por glCreateVertexArrays (OpenGL guarda el
    // "nombre"/ID del VAO), count_ por la cantidad de índices de la malla.
    // Son const: pueden llamarse sobre un const Mesh* (como el de RenderItem).
    unsigned int vao(void) const { return vao_; }
    int count(void) const         { return count_; }   // cantidad de INDICES
private:
    unsigned int vao_ {0U}, vbo_ {0U}, ebo_ {0U};
    int count_ {0};
};