// -----------------------------------------------------------------------------
// main_test_primitives.cpp
//
// Verificación por cálculo del orden de los índices (Práctico 03): para cada
// triángulo (A, B, C) de cada primitiva, el producto vectorial (B-A) x (C-A)
// tiene que apuntar para el mismo lado que la normal guardada en sus tres
// vértices (producto punto > 0). Sin esta verificación un orden invertido
// pasaría desapercibido, porque el descarte de caras traseras está apagado.
//
// Compila sin OpenGL (solo glm + primitives):
//   g++ -std=c++17 -Wall -Wextra -I./src src/core/Primitives.cpp tests/main_test_primitives.cpp -o build/test_primitives
// -----------------------------------------------------------------------------

#include "core/MeshData.h"
#include "core/Primitives.h"

#include <glm/glm.hpp>
#include <iostream>

namespace {

bool indices_validos(const MeshData& mesh)
{
    for (unsigned int idx : mesh.indices) {
        if (idx >= mesh.vertices.size()) {
            std::cout << "  [ERROR] indice " << idx << " fuera de rango ("
                      << mesh.vertices.size() << " vertices)" << std::endl;
            return false;
        }
    }
    return true;
}

// true si TODOS los triangulos tienen el producto vectorial apuntando para el
// mismo lado que la normal de sus tres vertices.
bool verificar_orientacion(const MeshData& mesh, const char* nombre)
{
    std::cout << nombre << ": " << mesh.vertices.size() << " vertices, "
              << mesh.indices.size() << " indices" << std::endl;

    if (!indices_validos(mesh)) {
        return false;
    }

    bool ok = true;
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        const glm::vec3& a = mesh.vertices[mesh.indices[i + 0]].position;
        const glm::vec3& b = mesh.vertices[mesh.indices[i + 1]].position;
        const glm::vec3& c = mesh.vertices[mesh.indices[i + 2]].position;

        // Normal geometrica del triangulo (sin normalizar).
        const glm::vec3 geo = glm::cross(b - a, c - a);

        const float da = glm::dot(geo, mesh.vertices[mesh.indices[i + 0]].normal);
        const float db = glm::dot(geo, mesh.vertices[mesh.indices[i + 1]].normal);
        const float dc = glm::dot(geo, mesh.vertices[mesh.indices[i + 2]].normal);

        if (da <= 0.0f || db <= 0.0f || dc <= 0.0f) {
            std::cout << "  [ERROR] triangulo " << (i / 3)
                      << " con orientacion invertida (dots: "
                      << da << ", " << db << ", " << dc << ")" << std::endl;
            ok = false;
        }
    }
    std::cout << (ok ? "  [OK]   todas las caras miran hacia afuera"
                    : "  [FALLO] hay caras invertidas") << std::endl;
    return ok;
}

}   // namespace (anónimo)

int main()
{
    bool ok = true;

    // Cubo (24/36) y triangulo de ejemplo.
    ok &= verificar_orientacion(primitives::cube(), "cubo (24/36)");
    ok &= verificar_orientacion(primitives::triangle(), "triangulo (3/3)");

    // Cilindro: lateral con 8 gajos, 1 y 2 anillos.
    ok &= verificar_orientacion(primitives::cylinder(1.0f, 2.0f, 8U), "cilindro 8 gajos, 1 anillo");
    ok &= verificar_orientacion(primitives::cylinder(1.0f, 2.0f, 8U, 2U), "cilindro 8 gajos, 2 anillos");

    // Cono: 8 gajos, 1 y 3 anillos (3 para ejercitar las bandas intermedias).
    ok &= verificar_orientacion(primitives::cone(1.0f, glm::radians(60.0f), 8U), "cono 8 gajos, 1 anillo");
    ok &= verificar_orientacion(primitives::cone(1.0f, glm::radians(60.0f), 8U, 3U), "cono 8 gajos, 3 anillos");

    // Esfera (opcional): anillos de latitud; ejerce las dos bandas polares y
    // las intermedias.
    ok &= verificar_orientacion(primitives::sphere(1.0f, 8U, 4U), "esfera 8 gajos, 4 anillos");
    ok &= verificar_orientacion(primitives::sphere(1.0f, 12U, 8U), "esfera 12 gajos, 8 anillos");

    // Conteos esperados del lateral de un cilindro de N gajos y 2 anillos.
    const MeshData lateral = primitives::cylinder(1.0f, 2.0f, 8U);
    const bool cuenta = (lateral.vertices.size() == 2 * (8 + 1));
    std::cout << "conteo lateral N=8, 2 anillos: " << lateral.vertices.size()
              << " vertices (se esperan 18) -> "
              << (cuenta ? "[OK]" : "[FALLO]") << std::endl;
    ok &= cuenta;

    std::cout << "\n" << (ok ? "TODAS LAS PRUEBAS PASARON"
                             : "HAY PRUEBAS QUE FALLARON") << std::endl;
    return ok ? 0 : 1;
}