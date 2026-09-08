// -----------------------------------------------------------------------------
// Primitives.cpp — implementacion (solo datos, sin OpenGL)
// -----------------------------------------------------------------------------

#include "core/Primitives.h"

#include <cmath>        // cos, sin, tan
#include <cstddef>      // size_t

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

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

}   // namespace (anónimo)

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

    const float kScale[3] = {scale_x, scale_y, scale_z};

    for (int face = 0; face < 6; ++face) {
        const FaceSpec& spec = kFaces[face];

        // Normal de la cara = vector unitario a lo largo del eje, con el
        // signo de la cara (ej. +z -> (0,0,1)). Igual para las 4 esquinas:
        // una cara tiene normal constante, y por eso las aristas del cubo
        // (que separan dos caras) duplican vertices.
        glm::vec3 normal(0.0f);
        normal[spec.axis] = static_cast<float>(spec.sign);

        // Cuatro esquinas (una por vertice unico). Los vertices NO se
        // comparten entre caras porque la normal difiere: la tupla completa
        // "posicion + normal + uv" no coincide en las aristas.
        for (int corner = 0; corner < 4; ++corner) {
            // Orden de recorrido de las esquinas: (u-,v-) (u+,v-) (u+,v+) (u-,v+)
            const int u_sign = (corner == 0 || corner == 3) ? -1 : +1;
            const int v_sign = (corner < 2) ? -1 : +1;

            glm::vec3 pos(0.0f);
            pos[spec.axis]   = static_cast<float>(spec.sign) * 0.5f * kScale[spec.axis];
            pos[spec.u_axis] = static_cast<float>(u_sign)    * 0.5f * kScale[spec.u_axis];
            pos[spec.v_axis] = static_cast<float>(v_sign)    * 0.5f * kScale[spec.v_axis];

            Vertex vertex;
            vertex.position   = pos;
            vertex.normal     = normal;
            // UV por esquina: (0,0) (1,0) (1,1) (0,1), mismo orden que las
            // esquinas. Hoy no se usan, pero dejan el cubo listo para textura.
            vertex.tex_coords = glm::vec2(
                (u_sign < 0) ? 0.0f : 1.0f,
                (v_sign < 0) ? 0.0f : 1.0f);
            mesh.vertices.push_back(vertex);
        }

        // Dos triángulos por cara con los cuatro indices en el mismo
        // sentido (antihorario desde afuera): (0,1,2) y (0,2,3).
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

MeshData cylinder(float radio, float largo, unsigned gajos, unsigned anillos)
{
    MeshData mesh;
    if (gajos < 3U) gajos = 3U;
    if (anillos < 1U) anillos = 1U;

    const unsigned rows = anillos + 1U;              // anillos de vertices
    const unsigned per_ring = gajos + 1U;            // N + 1 por la costura
    mesh.vertices.reserve(static_cast<size_t>(rows) * per_ring);
    mesh.indices.reserve(static_cast<size_t>(anillos) * gajos * 6U);

    // 1) Vértices: un anillo por fila, de abajo hacia arriba.
    for (unsigned r = 0; r < rows; ++r) {
        const float f = static_cast<float>(r) / static_cast<float>(anillos);
        const float y = -largo * 0.5f + f * largo;   // de -largo/2 a +largo/2
        const float v = f;                            // coord. axial de textura

        for (unsigned j = 0; j <= gajos; ++j) {       // <= para cerrar la costura
            const float th = kTwoPi * static_cast<float>(j) / static_cast<float>(gajos);
            const float c = std::cos(th);
            const float s = std::sin(th);

            Vertex vertex;
            vertex.position   = glm::vec3(radio * c, y, radio * s);
            // Normal radial: la posicion sin la componente axial (y), normalizada.
            vertex.normal     = glm::vec3(c, 0.0f, s);
            // u = 0 en la costura de j = 0 y u = 1 en la de j = gajos (posicion
            // repetida, uv distinto: por eso N+1 vertices).
            vertex.tex_coords = glm::vec2(
                static_cast<float>(j) / static_cast<float>(gajos), v);
            mesh.vertices.push_back(vertex);
        }
    }

    // 2) Índices: entre dos anillos consecutivos, cada gajo es un cuadrilatero
    //    de dos triangulos. Orden elegido para dejar las normales hacia
    //    afuera: {b0, t1, b1} y {b0, t0, t1}. El "natural" {b0, b1, t1} queda
    //    invertido.
    for (unsigned r = 0; r < anillos; ++r) {
        const unsigned bottom = r * per_ring;
        const unsigned top    = (r + 1U) * per_ring;
        for (unsigned j = 0; j < gajos; ++j) {
            const unsigned b0 = bottom + j;
            const unsigned b1 = b0 + 1U;
            const unsigned t0 = top + j;
            const unsigned t1 = t0 + 1U;

            mesh.indices.push_back(b0);
            mesh.indices.push_back(t1);
            mesh.indices.push_back(b1);

            mesh.indices.push_back(b0);
            mesh.indices.push_back(t0);
            mesh.indices.push_back(t1);
        }
    }

    return mesh;
}

MeshData cone(float radio, float conicidad, unsigned gajos, unsigned anillos)
{
    MeshData mesh;
    if (gajos < 3U) gajos = 3U;
    if (anillos < 1U) anillos = 1U;

    const float semi = conicidad * 0.5f;             // semiangulo (α/2)
    const float largo = radio / std::tan(semi);      // altura = radio / tan(α/2)
    const float cs = std::cos(semi);                 // proyeccion horizontal de la normal
    const float sn = std::sin(semi);                 // componente axial de la normal

    const unsigned per_ring = gajos + 1U;            // N + 1 por la costura
    // anillos anillos del lateral + 1 vertice del apice.
    mesh.vertices.reserve(static_cast<size_t>(anillos) * per_ring + 1U);
    mesh.indices.reserve(static_cast<size_t>(anillos) * gajos * 3U);

    // 1) Vértices del lateral: anillos que suben achicando el radio hasta el
    //    apice. El apice se agrega al final como un UNICO vertice (normal
    //    axial, no comparte con el lateral).
    for (unsigned r = 0; r < anillos; ++r) {
        const float f = static_cast<float>(r) / static_cast<float>(anillos);
        const float y  = -largo * 0.5f + f * largo;  // base abajo, apice arriba
        const float rr = radio * (1.0f - f);         // radio del anillo

        for (unsigned j = 0; j <= gajos; ++j) {
            const float th = kTwoPi * static_cast<float>(j) / static_cast<float>(gajos);
            const float c = std::cos(th);
            const float s = std::sin(th);

            Vertex vertex;
            vertex.position   = glm::vec3(rr * c, y, rr * s);
            // Normal lateral inclinada por el semiangulo (no es radial).
            vertex.normal     = glm::vec3(cs * c, sn, cs * s);
            vertex.tex_coords = glm::vec2(
                static_cast<float>(j) / static_cast<float>(gajos), f);
            mesh.vertices.push_back(vertex);
        }
    }

    // Ápice: un solo vertice, en la punta, normal axial hacia arriba.
    {
        Vertex apex;
        apex.position   = glm::vec3(0.0f, largo * 0.5f, 0.0f);
        apex.normal     = glm::vec3(0.0f, 1.0f, 0.0f);
        apex.tex_coords = glm::vec2(0.5f, 1.0f);
        mesh.vertices.push_back(apex);
    }
    const unsigned apex_idx = static_cast<unsigned>(anillos) * per_ring;

    // 2) Índices.
    //    - Entre anillos consecutivos: banda de cuadrilateros como en el
    //      cilindro (el radio del anillo superior ya es menor).
    //    - Ultima banda: abanico de triangulos entre el ultimo anillo y el
    //      apice, en sentido antihorario: {b0, apex, b1}.
    for (unsigned r = 0; r < anillos; ++r) {
        const unsigned bottom = r * per_ring;
        const bool last_band = (r == anillos - 1U);

        if (last_band) {
            for (unsigned j = 0; j < gajos; ++j) {
                const unsigned b0 = bottom + j;
                const unsigned b1 = b0 + 1U;
                mesh.indices.push_back(b0);
                mesh.indices.push_back(apex_idx);
                mesh.indices.push_back(b1);
            }
        } else {
            const unsigned top = (r + 1U) * per_ring;
            for (unsigned j = 0; j < gajos; ++j) {
                const unsigned b0 = bottom + j;
                const unsigned b1 = b0 + 1U;
                const unsigned t0 = top + j;
                const unsigned t1 = t0 + 1U;

                mesh.indices.push_back(b0);
                mesh.indices.push_back(t1);
                mesh.indices.push_back(b1);

                mesh.indices.push_back(b0);
                mesh.indices.push_back(t0);
                mesh.indices.push_back(t1);
            }
        }
    }

    return mesh;
}

// Triángulo de ejemplo (plano en xy): 3 vértices, normal +z, 3 índices.
MeshData triangle(void)
{
    MeshData mesh;
    mesh.vertices.resize(3);
    mesh.indices.resize(3);

    mesh.vertices[0].position = glm::vec3(-0.5f, -0.5f, 0.0f);
    mesh.vertices[1].position = glm::vec3( 0.5f, -0.5f, 0.0f);
    mesh.vertices[2].position = glm::vec3( 0.0f,  0.5f, 0.0f);
    mesh.vertices[0].tex_coords = glm::vec2(0.0f, 0.0f);
    mesh.vertices[1].tex_coords = glm::vec2(1.0f, 0.0f);
    mesh.vertices[2].tex_coords = glm::vec2(0.5f, 1.0f);
    for (int i = 0; i < 3; ++i) {
        mesh.vertices[static_cast<size_t>(i)].normal = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    mesh.indices[0] = 0;
    mesh.indices[1] = 1;
    mesh.indices[2] = 2;

    return mesh;
}

MeshData sphere(float radio, unsigned gajos, unsigned anillos)
{
    MeshData mesh;
    if (gajos < 3U) gajos = 3U;
    if (anillos < 2U) anillos = 2U;

    const unsigned per_ring   = gajos + 1U;              // N + 1 por la costura
    const unsigned full_rings = anillos - 1U;            // anillos de latitud (sin los polos)
    const unsigned south_idx  = 0U;
    const unsigned north_idx  = 1U + full_rings * per_ring;

    mesh.vertices.reserve(2U + full_rings * per_ring);
    mesh.indices.reserve(static_cast<size_t>(anillos) * gajos * 3U);

    // 1) Vértices: polo sur, anillos de latitud intermedios y polo norte.
    //    Los polos son un UNICO vertice con normal axial (criterio del apice
    //    del cono): la normal no esta definida en el polo, se elige la axial.
    {
        Vertex pole;
        pole.position   = glm::vec3(0.0f, -radio, 0.0f);
        pole.normal     = glm::vec3(0.0f, -1.0f, 0.0f);
        pole.tex_coords = glm::vec2(0.5f, 0.0f);
        mesh.vertices.push_back(pole);
    }

    for (unsigned i = 1; i < anillos; ++i) {
        const float v  = static_cast<float>(i) / static_cast<float>(anillos);
        const float lat = -kPi * 0.5f + kPi * v;          // latitud: sur .. norte
        const float ct = std::cos(lat);                   // radio del anillo
        const float st = std::sin(lat);                   // altura del anillo

        for (unsigned j = 0; j <= gajos; ++j) {           // <= por la costura
            const float lon = kTwoPi * static_cast<float>(j) / static_cast<float>(gajos);
            const float c = std::cos(lon);
            const float s = std::sin(lon);

            Vertex vertex;
            // La normal de una esfera centrada es radial (posicion normalizada).
            vertex.normal     = glm::vec3(ct * c, st, ct * s);
            vertex.position   = radio * vertex.normal;
            vertex.tex_coords = glm::vec2(
                static_cast<float>(j) / static_cast<float>(gajos), v);
            mesh.vertices.push_back(vertex);
        }
    }

    {
        Vertex pole;
        pole.position   = glm::vec3(0.0f, radio, 0.0f);
        pole.normal     = glm::vec3(0.0f, 1.0f, 0.0f);
        pole.tex_coords = glm::vec2(0.5f, 1.0f);
        mesh.vertices.push_back(pole);
    }

    // 2) Índices, verificados para que las normales queden hacia afuera:
    //    - banda sur : abanico {S, v_j, v_j+1}
    //    - bandas del medio: cuadrilateros {b0,t1,b1} y {b0,t0,t1}
    //      (igual que cilindro/cono)
    //    - banda norte: abanico {v_j, N, v_j+1}  (ojo: orden DISTINTO al sur)
    const unsigned ring1 = 1U;   // primer anillo completo, justo despues del polo sur
    for (unsigned j = 0; j < gajos; ++j) {
        mesh.indices.push_back(south_idx);
        mesh.indices.push_back(ring1 + j);
        mesh.indices.push_back(ring1 + j + 1U);
    }

    for (unsigned i = 1; i + 1U < anillos; ++i) {         // bandas del medio
        const unsigned bottom = 1U + (i - 1U) * per_ring;
        const unsigned top    = 1U + i * per_ring;
        for (unsigned j = 0; j < gajos; ++j) {
            const unsigned b0 = bottom + j;
            const unsigned b1 = b0 + 1U;
            const unsigned t0 = top + j;
            const unsigned t1 = t0 + 1U;

            mesh.indices.push_back(b0);
            mesh.indices.push_back(t1);
            mesh.indices.push_back(b1);

            mesh.indices.push_back(b0);
            mesh.indices.push_back(t0);
            mesh.indices.push_back(t1);
        }
    }

    {
        const unsigned last_ring = 1U + (anillos - 2U) * per_ring;
        for (unsigned j = 0; j < gajos; ++j) {
            mesh.indices.push_back(last_ring + j);
            mesh.indices.push_back(north_idx);
            mesh.indices.push_back(last_ring + j + 1U);
        }
    }

    return mesh;
}

}   // namespace primitives