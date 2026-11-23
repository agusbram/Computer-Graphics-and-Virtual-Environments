// -----------------------------------------------------------------------------
// main_test_normales.cpp — prueba de las NORMALES del terreno (Práctico 09, Parte 2)
//
// Verifica por cálculo, sin dibujar nada (no hace falta OpenGL), la corrección
// del BORDE x 2 de generar_malla_terreno (Terrain.h, decisión 9):
//   - En un PLANO INCLINADO la normal es la MISMA en todos los puntos. Si se
//     calcula bien, tiene que salir igual en el interior y en el borde. Con la
//     fórmula vieja (diferencia de un solo lado sin el x 2) la normal del borde
//     salía más vertical: la pendiente quedaba a la mitad.
//   - La componente z (la vertical en esta escena Z-arriba) tiene que ser
//     positiva en todos los vértices: si no, la luz iluminaría el lado de abajo
//     (es el control que pide la guía; ahí dice "y" porque usa Y arriba).
//
// Las normales del CLIPMAP (Terrain::recalcular_vertices, con la grilla con
// margen y la normal del nivel grueso) no se prueban acá: necesitan la GPU.
// Se verificaron en pantalla (ver Practico09-Decisiones.md, Parte 2).
//
// Compilar y correr (desde la raíz del proyecto; es UNA sola línea):
//   g++ -std=c++17 -Wall -Wextra -I./src -I./third_party/FastNoiseLite
//       tests/main_test_normales.cpp -o build/test_normales && ./build/test_normales
// (Sin la barra invertida de continuación: dentro de un comentario // el
// compilador la toma como "el comentario sigue en la línea de abajo" y avisa.)
// -----------------------------------------------------------------------------

#include <algorithm>   // std::max: el peor error
#include <cmath>       // std::sin, std::cos, std::fabs
#include <cstdlib>     // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>    // std::cout

#include <glm/glm.hpp> // glm::vec3, normalize, length

#include "core/Terrain.h"   // generar_malla_terreno (función libre, plantilla: no hace falta Terrain.cpp)

static int fallas = 0;

static void verificar(bool ok, const char* texto)
{
    std::cout << (ok ? "  [OK]   " : "  [FALLA] ") << texto << std::endl;
    if (!ok) {
        ++fallas;
    }
}

int main()
{
    const unsigned N = 8U;       // 8 x 8 celdas -> 9 x 9 vértices
    const float paso = 2.5f;     // como el nivel fino del clipmap

    // 1) PLANO INCLINADO z = 0.5·x + 0.25·y. Su normal (exacta) es
    //    normalize(-dz/dx, -dz/dy, 1) = normalize(-0.5, -0.25, 1), en TODOS los
    //    puntos. Las diferencias (centrales o de un lado) son exactas en un
    //    plano, así que cualquier diferencia con la esperada es un error de
    //    escala, que es justo lo que corrige el x 2.
    std::cout << "1. Plano inclinado: la normal del borde igual a la del interior" << std::endl;
    {
        auto plano = [](float x, float y) { return 0.5f * x + 0.25f * y; };
        const MeshData m = generar_malla_terreno(N, paso, glm::vec2(100.0f, -40.0f), plano);
        const glm::vec3 esperada = glm::normalize(glm::vec3(-0.5f, -0.25f, 1.0f));
        float peor_borde = 0.0f, peor_interior = 0.0f;
        for (unsigned i = 0; i <= N; ++i) {
            for (unsigned j = 0; j <= N; ++j) {
                // Error = distancia entre la normal calculada y la exacta.
                const float err = glm::length(m.vertices[i * (N + 1U) + j].normal - esperada);
                const bool borde = (i == 0U || j == 0U || i == N || j == N);
                if (borde) {
                    peor_borde = std::max(peor_borde, err);
                } else {
                    peor_interior = std::max(peor_interior, err);
                }
            }
        }
        std::cout << "         error maximo: interior " << peor_interior
                  << ", borde " << peor_borde << std::endl;
        verificar(peor_interior < 1e-5f, "interior: la normal del plano");
        verificar(peor_borde < 1e-5f, "borde: la MISMA normal (corrección x 2)");
    }

    // 2) Una superficie con relieve (suma de senos, como una fuente de alturas
    //    cualquiera): la normal siempre hacia ARRIBA (z > 0) y unitaria.
    std::cout << "2. Relieve: normales unitarias y hacia arriba (z > 0)" << std::endl;
    {
        auto relieve = [](float x, float y) {
            return 8.0f * std::sin(0.11f * x) * std::cos(0.07f * y) + 3.0f * std::sin(0.31f * x + 0.17f * y);
        };
        const MeshData m = generar_malla_terreno(N, paso, glm::vec2(0.0f), relieve);
        bool arriba = true, unitarias = true;
        for (const Vertex& v : m.vertices) {
            arriba    = arriba && (v.normal.z > 0.0f);
            unitarias = unitarias && (std::fabs(glm::length(v.normal) - 1.0f) < 1e-5f);
        }
        verificar(arriba, "todas las normales con z > 0 (hacia arriba)");
        verificar(unitarias, "todas las normales de largo 1");
    }

    std::cout << std::endl << (fallas == 0 ? "TODAS LAS PRUEBAS PASARON" : "HAY PRUEBAS QUE FALLARON")
              << std::endl;
    return fallas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
