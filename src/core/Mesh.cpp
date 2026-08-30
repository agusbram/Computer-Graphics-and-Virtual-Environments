// -----------------------------------------------------------------------------
// Mesh.cpp — implementacion
// Carga con DSA (Direct State Access, GL 4.5): se nombran los objetos por su
// ID sin bindearlos (no se usa glBindVertexArray/glBindBuffer aca).
// -----------------------------------------------------------------------------

#include "core/Mesh.h"

#include <cstddef>              // offsetof
#include <glad/gl.h>

Mesh::~Mesh()
{
    clear();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vao_(other.vao_)
    , vbo_(other.vbo_)
    , ebo_(other.ebo_)
    , count_(other.count_)
{
    other.vao_ = other.vbo_ = other.ebo_ = 0U;   // el movido queda vacío
    other.count_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        clear();                                 // liberar lo que yo poseía
        vao_ = other.vao_;
        vbo_ = other.vbo_;
        ebo_ = other.ebo_;
        count_ = other.count_;
        other.vao_ = other.vbo_ = other.ebo_ = 0U;
        other.count_ = 0;
    }
    return *this;
}

void Mesh::load(const MeshData& data)
{
    // Si el objeto ya tenia una malla cargada, se libera primero: load() se
    // puede llamar varias veces sin perder memoria en la GPU.
    clear();

    if (data.vertices.empty()) {
        return;                                  // nada que subir
    }

    // VAO: recuerda la organizacion de los buffers y sus atributos.
    glCreateVertexArrays(1, &vao_);

    // VBO: UN solo buffer con la tupla completa entrelazada (posicion +
    // color por vertice, en memorama de Vertex). El stride es sizeof(Vertex).
    glCreateBuffers(1, &vbo_);
    glNamedBufferData(vbo_,
                      static_cast<GLsizeiptr>(data.vertices.size() * sizeof(Vertex)),
                      data.vertices.data(), GL_STATIC_DRAW);
    glVertexArrayVertexBuffer(vao_, 0, vbo_, 0,
                              static_cast<GLsizei>(sizeof(Vertex)));

    // Atributo 0 -> location = 0 del vertex shader (aPos): 3 floats.
    glVertexArrayAttribFormat(vao_, 0, 3, GL_FLOAT, GL_FALSE,
                              static_cast<GLuint>(offsetof(Vertex, px)));
    glVertexArrayAttribBinding(vao_, 0, 0);
    glEnableVertexArrayAttrib(vao_, 0);

    // Atributo 1 -> location = 1 del vertex shader (aColor): 3 floats.
    glVertexArrayAttribFormat(vao_, 1, 3, GL_FLOAT, GL_FALSE,
                              static_cast<GLuint>(offsetof(Vertex, r)));
    glVertexArrayAttribBinding(vao_, 1, 0);
    glEnableVertexArrayAttrib(vao_, 1);

    // EBO (opcional): lista de indices de los triangulos, a la ranura
    // dedicada del VAO.
    if (!data.indices.empty()) {
        glCreateBuffers(1, &ebo_);
        glNamedBufferData(ebo_,
                          static_cast<GLsizeiptr>(data.indices.size() * sizeof(unsigned int)),
                          data.indices.data(), GL_STATIC_DRAW);
        glVertexArrayElementBuffer(vao_, ebo_);
        count_ = static_cast<int>(data.indices.size());
    } else {
        count_ = static_cast<int>(data.vertices.size());
    }
}

void Mesh::clear(void)
{
    // glDelete*(0) no hace nada: borrar dos veces no rompe y no se necesita
    // una bandera auxiliar que diga "tengo o no tengo recursos".
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ebo_);
    vao_ = vbo_ = ebo_ = 0U;
    count_ = 0;
}