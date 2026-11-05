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
// Terrain — GEOMETRY CLIPMAPS, etapa 1 (Práctico 07, LOD, Losasso & Hoppe 2004)
//
// En vez de UNA ventana de N x N celdas, hay `niveles` ventanas ANIDADAS, todas
// centradas en el avión y todas de N x N celdas, pero con separación que se
// DUPLICA por nivel:  paso_l = paso · 2^l.
//   - nivel 0: el más fino, cubre N·paso (detalle máximo cerca del avión)
//   - nivel l: cubre 2^l veces más lado con la misma cantidad de vértices
// Cada nivel l >= 1 NO dibuja la zona que ya cubre el nivel l-1: se le quita un
// HUECO de (N/2) x (N/2) de sus celdas -> se dibuja un ANILLO (la "render
// region" del paper). El nivel 0 se dibuja completo.
//
// ALINEACIÓN (restricción del paper): el borde de un nivel fino debe caer sobre
// vértices del nivel grueso. Por eso el centro de cada nivel se redondea a
// múltiplos de 2·paso_l (no de paso_l): con N múltiplo de 4, la esquina queda en
// un múltiplo de 2·paso_l = paso_(l+1), es decir, sobre un vértice del nivel
// siguiente. El hueco cae así exactamente sobre celdas enteras del grueso.
//
// ETAPA 1: anillos anidados que siguen al avión, SIN transición. En el borde
// entre niveles, los vértices impares del fino no coinciden con la arista del
// grueso -> GRIETAS (T-junctions) y "popping".
//
// ETAPA 2 (esta): se eliminan con dos técnicas del paper (§6.2):
//   a) GEOMORPHING: cada vértice guarda también z_c (altura en el nivel grueso,
//      campo Vertex::altura_gruesa). El vertex shader (solid.vs) mezcla
//      z' = (1-α)·z + α·z_c, con α = 0 adentro y α = 1 en el borde exterior
//      (rampa de ancho w = N/10 celdas). En el borde el fino coincide con el
//      grueso (sin grietas) y, como α depende de la posición CONTINUA del
//      avión, la forma cambia suave (sin popping).
//   b) TRIÁNGULOS DE ÁREA CERO en el borde exterior de cada nivel (salvo el
//      más grueso): unen cada vértice impar con sus dos vecinos pares. Tras el
//      morph los tres quedan alineados (área 0, no se ven), pero tapan las
//      rendijas de un píxel que deja el redondeo de coma flotante.
// Simplificación respecto del paper: no se usa direccionamiento toroidal; al
// moverse un nivel se recalculan todas sus alturas (como la ventana infinita
// del Práctico 07, pero por nivel: los gruesos casi nunca se mueven).
// -----------------------------------------------------------------------------
class Terrain {
public:
    // N: celdas por lado DE CADA NIVEL (múltiplo de 4); paso: separación del
    // nivel 0; niveles: cantidad de anillos; amplitud: desnivel máximo;
    // altura_terreno: altitud FIJA del terreno en el mundo (unidades de escena).
    Terrain(unsigned N, float paso, unsigned niveles, float amplitud,
            float altura_terreno, int semilla, float frecuencia);
    ~Terrain();

    Terrain(const Terrain&) = delete;            // posee Meshes (dueño único)
    Terrain& operator=(const Terrain&) = delete;

    // init: genera vértices + índices de cada nivel y los sube a la GPU.
    void init();
    // update: cada nivel sigue al avión a saltos de 2·paso_l. Recalcula alturas
    // SOLO del nivel que se movió, e índices (hueco) SOLO si se movió el nivel
    // más fino que lo rellena. Arma la matriz de modelo de cada nivel.
    void update(const glm::vec3& pos_avion);
    // collect: un RenderItem por nivel (el main no se entera de cuántos son).
    void collect(std::vector<RenderItem>& items) const;

    // Depuración: pinta cada nivel de un color distinto (tecla L).
    void set_colores_niveles(bool activo) { colores_niveles_ = activo; }
    // Depuración: geomorphing on/off (tecla M), para comparar etapa 1 vs 2.
    void set_morph(bool activo) { morph_ = activo; }

    unsigned celdas() const { return N_; }
    float paso() const { return paso_; }
    unsigned niveles() const { return static_cast<unsigned>(niveles_.size()); }
    // Lado total cubierto (el del nivel más grueso), en unidades de escena.
    float extension() const;

private:
    struct Nivel {
        float     paso {1.0f};          // paso_l = paso · 2^l
        glm::vec2 centro {0.0f};        // centro (mundo), a saltos de 2·paso_l
        glm::ivec2 hueco {-1, -1};      // celda (j,i) donde empieza el hueco (-1: sin hueco)
        Mesh      malla;                // VAO/VBO/EBO del nivel
        MeshData  datos;                // copia en CPU (para recalcular y re-subir)
        glm::mat4 modelo {1.0f};
    };

    glm::vec2 esquina(const Nivel& nv) const;             // centro - (N/2)·paso_l
    glm::vec2 centro_para(float paso_l, const glm::vec3& pos) const;
    void recalcular_vertices(Nivel& nv);                   // alturas + normales
    glm::ivec2 calcular_hueco(std::size_t l) const;        // dónde cae el nivel l-1
    void armar_indices(Nivel& nv, bool cerrar_borde) const; // grilla - hueco (+ área 0)

    unsigned N_;
    float    paso_;
    float    altura_terreno_;
    Alturas  alturas_;
    std::vector<Nivel> niveles_;
    glm::vec3 color_ {0.33f, 0.45f, 0.28f};
    bool      colores_niveles_ {false};
    bool      morph_ {true};
    float     ancho_transicion_ {1.0f};   // w del paper, en celdas (= N/10)
    bool      primera_ {true};
};
