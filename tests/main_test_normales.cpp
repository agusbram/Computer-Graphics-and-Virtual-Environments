// -----------------------------------------------------------------------------
// main_test_normales.cpp — prueba de las NORMALES (Práctico 09, Partes 2 y 3)
//
// Verifica por cálculo, sin dibujar nada (no hace falta OpenGL):
//
// PARTE 2 (pruebas 1 y 2) — la corrección del BORDE x 2 de
// generar_malla_terreno (Terrain.h, decisión 9):
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
// PARTE 3 (pruebas 3 y 4) — las normales del AVIÓN:
//   3. El ÁPICE DEL CONO (decisión 16): todas las normales del cono, también
//      las de la punta, tienen que ser las del lateral (componente axial =
//      sen del semiángulo). Con el ápice único de normal axial (0, 1, 0) del
//      P03, la punta fallaba.
//   4. La MATRIZ NORMAL (decisión 17): el cono unitario de la nariz, estirado
//      con S(0.09, 0.20, 0.09). Una normal tiene que ser PERPENDICULAR a la
//      superficie; en un cono, la generatriz (la recta de la base al ápice)
//      está SOBRE la superficie, así que la normal de cada vértice de la base
//      tiene que ser perpendicular a su generatriz. Con G = (M^-1)^T lo es;
//      con M (lo que muestra la tecla X) no: se mide cuánto se aleja de 90°.
//
// Compilar y correr (desde la raíz del proyecto; es UNA sola línea):
//   g++ -std=c++17 -Wall -Wextra -I./src -I./third_party/FastNoiseLite
//       src/core/Primitives.cpp tests/main_test_normales.cpp -o build/test_normales && ./build/test_normales
// (Sin la barra invertida de continuación: dentro de un comentario // el
// compilador la toma como "el comentario sigue en la línea de abajo" y avisa.)
// -----------------------------------------------------------------------------

#include <algorithm>   // std::max: el peor error
#include <cmath>       // std::sin, std::cos, std::fabs, std::atan, std::acos
#include <cstdlib>     // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>    // std::cout

#include <glm/glm.hpp> // glm::vec3, mat3, normalize, length, dot, inverse, transpose, degrees
#include <glm/gtc/matrix_transform.hpp>   // glm::translate, rotate, scale: la matriz local de la nariz

#include "core/Primitives.h" // primitives::cone (P09, Parte 3: pruebas 3 y 4)
#include "core/Terrain.h"    // generar_malla_terreno (función libre, plantilla: no hace falta Terrain.cpp)

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

    // 3) ÁPICE DEL CONO (P09, Parte 3). Un cono de la forma de la nariz del P04
    //    (radio 0.09, alto 0.20) con 20 gajos y 1 anillo, como el avión. En un
    //    cono la normal forma SIEMPRE el mismo ángulo con el eje: su componente
    //    axial (y) es sen(semiángulo) en todo el lateral, también en la punta.
    std::cout << "3. Cono: todas las normales son las del lateral (tambien en el apice)" << std::endl;
    {
        const unsigned gajos = 20U;
        const float semi = std::atan(0.09f / 0.20f);                 // semiángulo de la nariz
        const MeshData m = primitives::cone(0.09f, 2.0f * semi, gajos);
        float peor = 0.0f;
        for (const Vertex& v : m.vertices) {
            peor = std::max(peor, std::fabs(v.normal.y - std::sin(semi)));
        }
        std::cout << "         " << m.vertices.size() << " vertices (21 de la base + "
                  << gajos << " del apice); mayor diferencia en la componente axial: "
                  << peor << std::endl;
        verificar(m.vertices.size() == (gajos + 1U) + gajos, "un vertice de apice por gajo");
        verificar(peor < 1e-5f, "la componente axial es sen(semiangulo) en TODOS los vertices");
    }

    // 4) MATRIZ NORMAL vs M (P09, Parte 3). La nariz del avión: el cono
    //    UNITARIO (radio 1, alto 1) con su matriz local T · Rz(90°) · S(0.09,
    //    0.20, 0.09). Para cada vértice de la BASE (índices 0..gajos, el anillo
    //    0) se lleva a mundo su generatriz (del vértice al ápice de su gajo) con
    //    M, y la normal con G o con M. El "error" es cuánto se aleja de 90° el
    //    ángulo entre la normal y la generatriz.
    std::cout << "4. Cono unitario estirado: la normal con G es perpendicular, con M no" << std::endl;
    {
        const unsigned gajos = 20U;
        const MeshData m = primitives::cone(1.0f, 2.0f * std::atan(1.0f), gajos);
        const glm::mat4 M = glm::scale(
            glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.10f, 0.0f, 0.0f)),
                        glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::vec3(0.09f, 0.20f, 0.09f));
        const glm::mat3 m3 = glm::mat3(M);
        const glm::mat3 G  = glm::transpose(glm::inverse(m3));
        const glm::vec3 apice = glm::vec3(M * glm::vec4(m.vertices.back().position, 1.0f));

        // Ángulo (en grados) que le falta o le sobra a la normal para ser
        // perpendicular a la generatriz: |90° - ángulo(normal, generatriz)|.
        auto error_grados = [&](const glm::mat3& matriz_normal, const Vertex& v) {
            const glm::vec3 p = glm::vec3(M * glm::vec4(v.position, 1.0f));
            const glm::vec3 gen = glm::normalize(apice - p);
            const glm::vec3 n = glm::normalize(matriz_normal * v.normal);
            return std::fabs(90.0f - glm::degrees(std::acos(glm::dot(n, gen))));
        };
        float peor_g = 0.0f, peor_m = 0.0f;
        for (unsigned j = 0; j <= gajos; ++j) {
            peor_g = std::max(peor_g, error_grados(G, m.vertices[j]));
            peor_m = std::max(peor_m, error_grados(m3, m.vertices[j]));
        }
        std::cout << "         error maximo: con G " << peor_g << " grados, con M "
                  << peor_m << " grados" << std::endl;
        verificar(peor_g < 0.01f, "con la matriz normal G: perpendicular a la superficie");
        verificar(peor_m > 30.0f, "con M: NO es perpendicular (el error que muestra la tecla X)");
    }

    std::cout << std::endl << (fallas == 0 ? "TODAS LAS PRUEBAS PASARON" : "HAY PRUEBAS QUE FALLARON")
              << std::endl;
    return fallas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
