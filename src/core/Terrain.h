// -----------------------------------------------------------------------------
// Terrain.h — terreno procedural (Práctico 07)
//
// Malla poligonal REGULAR de N x N CELDAS (N+1 vértices por lado) generada por
// programa a partir de una FUENTE DE ALTURAS (heightmap). Es la primera malla
// del proyecto que no se escribe a mano (cubo) ni sale de una primitiva
// paramétrica (cono/cilindro): se genera a partir de DATOS.
//
// Sistema de la escena: Z ARRIBA (igual que la aeronave, Práctico 04). El
// terreno se apoya en el plano X-Y y su relieve va en Z. (La guía usa "y" como
// altura; acá se adapta a Z porque toda la escena es Z-arriba.)
//
// ESCALA (decisión): 1 unidad de escena = 10 m. El modelo del avión (1 unidad)
// representa ~10 m (el real mide ~9 m), así todo queda coherente entre sí.
//
// TERRAIN INFINITO (el opcional de la guía): el heightmap representa el MUNDO
// (estático) y esta malla es una VENTANA que sigue al avión. La TOPOLOGÍA (los
// índices) se arma UNA vez; al cruzar una celda, la ventana se corre un paso
// (múltiplo de la separación) y se recalculan SOLO las alturas. Así cada vértice
// vuelve a caer sobre un nodo del heightmap y el relieve no "repta".
//
// Decisiones de diseño (mismo estilo que los módulos anteriores):
//   1. La generación de la geometría (vértices + índices) es una FUNCIÓN LIBRE
//      (generar_malla_terreno), como pide la guía: recibe N, el paso y la fuente
//      de alturas. La clase Terrain la usa y la mantiene viva.
//   2. La fuente de alturas está DESACOPLADA (clase Alturas): cambiar el ruido
//      por una imagen o un archivo de texto se reduce a cambiar esa clase.
//   3. Winding ANTIHORARIO mirando la cara desde arriba (la normal hacia +Z):
//      es la convención que exige el recorte de cara trasera (GL_CULL_FACE).
// -----------------------------------------------------------------------------

#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "FastNoiseLite.h"

#include "core/Mesh.h"
#include "core/MeshData.h"
#include "core/RenderItem.h"

// -----------------------------------------------------------------------------
// Fuente de alturas por ruido procedural (FastNoiseLite).
// Devuelve la altura YA ESCALADA, en las MISMAS unidades que la escena.
// -----------------------------------------------------------------------------
class Alturas {
public:
    Alturas(int semilla, float frecuencia, float amplitud);

    // Altura (unidades de escena) en la posición del mundo (x, z).
    float operator()(float x, float z) const;

private:
    FastNoiseLite ruido_;
    float amplitud_;    // desnivel máximo, en unidades de escena
};

// -----------------------------------------------------------------------------
// Generación de la malla de terreno (función libre, sin OpenGL: solo datos).
//   N     : cantidad de CELDAS por lado  -> N+1 vértices por lado
//   paso  : separación entre vértices (unidades de escena)
//   esquina: posición (x,z) de la esquina (0,0) de la grilla en el mundo
//   altura : fuente de alturas (x,z) -> altura (unidades de escena)
// Devuelve vértices (posición + normal) e índices. La normal del borde usa una
// diferencia de un solo lado (no se puede dejar en cero: normalizar 0 es indef.).
//
// `template <typename FuenteAltura>`: es GENÉRICO. Solo necesita algo
// "invocable" que dado (x,z) devuelva la altura -> sirve la clase Alturas, una
// lambda, o cualquier otra fuente (imagen, archivo). Desacopla la generación de
// la fuente de alturas: cambiar de fuente no toca esta función.
// -----------------------------------------------------------------------------
template <typename FuenteAltura>
MeshData generar_malla_terreno(unsigned N, float paso,
                               const glm::vec2& esquina_,
                               const FuenteAltura& altura)
{
    MeshData mesh;
    const unsigned fila = N + 1U;                 // ancho en VÉRTICES, no en celdas
    auto idx = [fila](unsigned i, unsigned j) { return i * fila + j; };

    // Altura en el nodo (i, j) — misma que usan vértices y normales.
    auto h = [&](unsigned i, unsigned j) {
        const float x = esquina_.x + static_cast<float>(j) * paso;
        const float z = esquina_.y + static_cast<float>(i) * paso;
        return altura(x, z);
    };

    // Normal hacia arriba por DIFERENCIAS CENTRADAS (o de un lado en el borde).
    // Para z = f(x, y):  n = normalize( (-df/dx, -df/dy, 1) ).
    auto normal = [&](unsigned i, unsigned j) {
        const float dx = (j == 0U)      ? (h(i, j)   - h(i, j + 1U))
                       : (j + 1U == N) ? (h(i, j - 1U) - h(i, j))
                       :                 (h(i, j - 1U) - h(i, j + 1U));
        const float dy = (i == 0U)      ? (h(i, j)   - h(i + 1U, j))
                       : (i + 1U == N) ? (h(i - 1U, j) - h(i, j))
                       :                 (h(i - 1U, j) - h(i + 1U, j));
        // En los bordes el denominador es 1·paso; adentro, 2·paso. Se normaliza,
        // así que el factor común se cancela y no hace falta distinguirlo.
        return glm::normalize(glm::vec3(dx, dy, 2.0f * paso));
    };

    // 1) Vértices: (N+1)^2. Ojo con el límite: <= N (hay N+1 vértices por lado).
    mesh.vertices.reserve(static_cast<size_t>(fila) * fila);
    for (unsigned i = 0; i <= N; ++i) {
        for (unsigned j = 0; j <= N; ++j) {
            Vertex v;
            v.position = glm::vec3(static_cast<float>(j) * paso,
                                   static_cast<float>(i) * paso,
                                   h(i, j));
            v.normal = normal(i, j);
            v.tex_coords = glm::vec2(static_cast<float>(j) / static_cast<float>(N),
                                     static_cast<float>(i) / static_cast<float>(N));
            mesh.vertices.push_back(v);
        }
    }

    // 2) Índices: se recorren CELDAS (< N). Cada celda = 2 triángulos que
    //    comparten la diagonal, en orden ANTIHORARIO visto DESDE ARRIBA (+Z):
    //    a -> c -> d  y  a -> b -> c.
    //    (OJO: la guía usa Y como altura y su orden a->d->c; como acá la escena
    //    es Z-arriba, el mismo orden de la guía daría normales hacia -Z y el
    //    culling descartaría todo el terreno. Por eso se invierte el sentido.)
    mesh.indices.reserve(static_cast<size_t>(N) * N * 6U);
    for (unsigned i = 0; i < N; ++i) {
        for (unsigned j = 0; j < N; ++j) {
            const unsigned a = idx(i, j),     b = idx(i, j + 1U);
            const unsigned d = idx(i + 1U, j), c = idx(i + 1U, j + 1U);
            mesh.indices.push_back(a);
            mesh.indices.push_back(c);
            mesh.indices.push_back(d);
            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);
        }
    }
    return mesh;
}

// -----------------------------------------------------------------------------
// Terrain — ventana de terreno que sigue al avión (terreno "infinito").
// -----------------------------------------------------------------------------
class Terrain {
public:
    // N: celdas por lado; paso: separación (unidades de escena);
    // amplitud: desnivel máximo (unidades de escena);
    // altura_terreno: altitud FIJA del terreno en el mundo (unidades de escena).
    //   NO sigue al avión en vertical: por eso el avión puede descender (con el
    //   elevador) y acercarse hasta tocarlo.
    // mitad_ = (N·paso)/2 = media extensión: sirve para CENTRAR la ventana en el
    //   avión (esquina = centro - mitad_).
    Terrain(unsigned N, float paso, float amplitud, float altura_terreno,
            int semilla, float frecuencia);
    ~Terrain();   // libera lo que posee (el Mesh se libera en su propio destructor)

    // Sin copia: Terrain POSEE un Mesh, y Mesh es de DUEÑO ÚNICO (copiarlo
    // intentaría copiar el VAO -> error). Se borra la copia para que el
    // compilador lo rechace de forma clara.
    Terrain(const Terrain&) = delete;
    Terrain& operator=(const Terrain&) = delete;

    // init: genera la malla en CPU (generar_malla_terreno) y la sube a la GPU
    // (malla_.load). Son dos pasos: calcular los datos -> subirlos.
    void init();
    // update: ventana infinita. Redondea la posición HORIZONTAL del avión a
    // múltiplos de paso_ (así el centro cae sobre nodos del heightmap y el
    // relieve no "repta") y recalcula los vértices SOLO al cruzar una celda.
    // Arma también la matriz de modelo (horizontal sigue al avión; vertical fija).
    void update(const glm::vec3& pos_avion);
    // collect: empaqueta el terreno en un RenderItem (mismo patrón que Aircraft);
    // el main solo recorre la lista y dibuja. NO dibuja (no toca OpenGL).
    void collect(std::vector<RenderItem>& items) const;

    // Información para consola / verificación.
    unsigned celdas() const { return N_; }
    float paso() const { return paso_; }

private:
    void recalcular_vertices();                   // alturas + normales + subir a GPU
    // esquina = centro_ - (mitad_, mitad_): esquina de menor (x,y) de la ventana.
    // Se usa para saber dónde cae cada nodo (leer el heightmap) y para el modelo.
    glm::vec2 esquina() const;

    unsigned N_;                 // celdas por lado
    float    paso_;              // separación entre vértices (unidades de escena)
    float    mitad_;             // media extensión de la ventana = (N/2)·paso
    float    altura_terreno_;    // altitud FIJA del terreno en el mundo (no sigue al avión)
    Alturas  alturas_;           // fuente de alturas (ruido), desacoplada
    Mesh     malla_;             // la malla en la GPU (VAO/VBO/EBO)
    MeshData datos_;             // los datos en CPU (vértices+índices): para recalcular y re-subir
    glm::vec3 color_ {0.33f, 0.45f, 0.28f};  // color plano del terreno
    glm::mat4 modelo_ {1.0f};    // matriz de modelo (la arma update, la usa collect)
    glm::vec2 centro_ {0.0f};    // centro de la ventana (mundo), a saltos de celda
    bool      primera_ {true};   // "¿es la primera actualización?"
};
