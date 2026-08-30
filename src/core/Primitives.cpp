// -----------------------------------------------------------------------------
// Primitives.cpp — implementacion (solo datos, sin OpenGL)
// -----------------------------------------------------------------------------

#include "core/Primitives.h"

// Descripcion de una cara del cubo.
// axis + sign identifican la normal exterior (ej.: axis = 2, sign = +1 -> +z).
// u_axis y v_axis son los dos ejes tangentes, en un orden tal que
// unitU x unitV = normal exterior. Con las esquinas recorridas como
// (u-,v-) (u+,v-) (u+,v+) (u-,v+) las caras quedan en sentido antihorario
// mirando la cara desde afuera.
struct FaceSpec {
    int axis;       // 0 = X, 1 = Y, 2 = Z
    int sign;       // +1 cara positiva, -1 cara negativa
    int u_axis;     // primer eje tangente
    int v_axis;     // segundo eje tangente
};

namespace primitives {

MeshData cube(float scale_x, float scale_y, float scale_z)
{
    MeshData mesh;
    mesh.vertices.reserve(24);
    mesh.indices.reserve(36);

    // Orden (u, v) elegido para que unitU x unitV = normal exterior:
    //   frente  (+z):  X x Y  = +Z
    //   atras   (-z):  Y x X  = -Z
    //   derecha (+x):  Y x Z  = +X
    //   izquierda (-x): Z x Y  = -X
    //   arriba  (+y):  Z x X  = +Y
    //   abajo   (-y):  X x Z  = -Y
    const FaceSpec kFaces[6] = {
        {2, +1, 0, 1},   // frente   (+z)
        {2, -1, 1, 0},   // atras    (-z)
        {0, +1, 1, 2},   // derecha  (+x)
        {0, -1, 2, 1},   // izquierda(-x)
        {1, +1, 2, 0},   // arriba   (+y)
        {1, -1, 0, 2},   // abajo    (-y)
    };

    const float kColors[6][3] = {
        {1.0f, 0.0f, 0.0f},   // frente    : rojo
        {0.0f, 1.0f, 0.0f},   // atras     : verde
        {0.0f, 1.0f, 1.0f},   // derecha   : cian
        {1.0f, 0.0f, 1.0f},   // izquierda : magenta
        {0.0f, 0.0f, 1.0f},   // arriba    : azul
        {1.0f, 1.0f, 0.0f},   // abajo     : amarillo
    };

    const float kScale[3] = {scale_x, scale_y, scale_z};

    for (int face = 0; face < 6; ++face) {
        const FaceSpec& spec = kFaces[face];
        const float* color = kColors[face];

        // Cuatro esquinas (una por vértice unico). Los vertices NO se
        // comparten entre caras porque cambia el color: la tupla completa
        // "posicion + color" difiere (regla: se comparte un vertice si y solo
        // si coinciden todos sus atributos).
        for (int corner = 0; corner < 4; ++corner) {
            // Orden de recorrido de las esquinas: (u-,v-) (u+,v-) (u+,v+) (u-,v+)
            const int u_sign = (corner == 0 || corner == 3) ? -1 : +1;
            const int v_sign = (corner < 2) ? -1 : +1;

            float pos[3] = {0.0f, 0.0f, 0.0f};
            pos[spec.axis]   = static_cast<float>(spec.sign) * 0.5f * kScale[spec.axis];
            pos[spec.u_axis] = static_cast<float>(u_sign)    * 0.5f * kScale[spec.u_axis];
            pos[spec.v_axis] = static_cast<float>(v_sign)    * 0.5f * kScale[spec.v_axis];

            Vertex vertex;
            vertex.px = pos[0];
            vertex.py = pos[1];
            vertex.pz = pos[2];
            vertex.r = color[0];
            vertex.g = color[1];
            vertex.b = color[2];
            mesh.vertices.push_back(vertex);
        }

        // Dos triángulos por cara con los cuatro indices en el mismo
        // sentido (antihorario desde afuera): (0,1,2) y (0,2,3).
        // Cada cara ocupa 4 vertices seguidos en la lista.
        const unsigned int base = static_cast<unsigned int>(face * 4);
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }

    return mesh;
}

// Triángulo de ejemplo (plano en xy): 3 vértices, un color por vértice,
// 3 índices. Orientación: la normal del producto vectorial de las dos
// aristas del primer triángulo da +z (antihorario visto desde afuera),
// igual criterio que el cubo.
MeshData triangle(void)
{
    MeshData mesh;
    mesh.vertices.resize(3);
    mesh.indices.resize(3);

    mesh.vertices[0] = {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f};  // esquina inferior izquierda: rojo
    mesh.vertices[1] = { 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f};  // esquina inferior derecha : verde
    mesh.vertices[2] = { 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f};  // vértice superior          : azul

    mesh.indices[0] = 0;
    mesh.indices[1] = 1;
    mesh.indices[2] = 2;

    return mesh;
}

}   // namespace primitives