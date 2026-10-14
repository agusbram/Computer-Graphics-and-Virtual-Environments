// -----------------------------------------------------------------------------
// Terrain.cpp — terreno procedural (Práctico 07)
// -----------------------------------------------------------------------------

#include "core/Terrain.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

// -----------------------------------------------------------------------------
// Alturas — ruido procedural con FastNoiseLite
// -----------------------------------------------------------------------------
Alturas::Alturas(int semilla, float frecuencia, float amplitud)
    : amplitud_(amplitud)
{
    ruido_.SetSeed(semilla);
    ruido_.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    ruido_.SetFractalType(FastNoiseLite::FractalType_FBm);  // suma de octavas
    ruido_.SetFractalOctaves(5);                            // detalle fino
    ruido_.SetFrequency(frecuencia);
}

float Alturas::operator()(float x, float z) const
{
    // GetNoise devuelve aprox. [-1, 1]; la amplitud lo lleva a unidades de
    // escena. (OJO: al sumar octavas la salida queda normalizada y en la
    // práctica usa ~1/3 del rango nominal -> el relieve real es menor.)
    return amplitud_ * ruido_.GetNoise(x, z);
}

// -----------------------------------------------------------------------------
// Terrain
// -----------------------------------------------------------------------------
Terrain::Terrain(unsigned N, float paso, float amplitud, float altura_terreno,
                 int semilla, float frecuencia)
    : N_(N)
    , paso_(paso)
    , mitad_(0.5f * static_cast<float>(N) * paso)
    , altura_terreno_(altura_terreno)
    , alturas_(semilla, frecuencia, amplitud)
{
}

Terrain::~Terrain() = default;   // Mesh libera sus recursos solo

glm::vec2 Terrain::esquina() const
{
    return centro_ - glm::vec2(mitad_);
}

void Terrain::init()
{
    // La topología (índices) y los vértices iniciales se arman UNA vez.
    centro_ = glm::vec2(0.0f);
    datos_ = generar_malla_terreno(N_, paso_, esquina(), alturas_);
    malla_.load(datos_);
}

void Terrain::recalcular_vertices()
{
    // Se recalcula SOLO la geometría de los vértices (la topología no cambia).
    // Optimización: las alturas se calculan UNA vez por nodo (en una grilla
    // temporal) y las normales se leen de ahí; si no, cada normal repetiría
    // llamadas al ruido (4 por vértice) y el costo se multiplicaría por ~5.
    const unsigned fila = N_ + 1U;
    const glm::vec2 esq = esquina();

    std::vector<float> h(static_cast<size_t>(fila) * fila);
    for (unsigned i = 0; i <= N_; ++i) {
        for (unsigned j = 0; j <= N_; ++j) {
            h[static_cast<size_t>(i) * fila + j] =
                alturas_(esq.x + static_cast<float>(j) * paso_,
                         esq.y + static_cast<float>(i) * paso_);
        }
    }

    for (unsigned i = 0; i <= N_; ++i) {
        for (unsigned j = 0; j <= N_; ++j) {
            Vertex& v = datos_.vertices[static_cast<size_t>(i) * fila + j];
            v.position = glm::vec3(static_cast<float>(j) * paso_,
                                   static_cast<float>(i) * paso_,
                                   h[static_cast<size_t>(i) * fila + j]);

            // Normal hacia arriba por diferencias (centradas adentro, de un
            // lado en el borde). Para z=f(x,y): n = normalize(-df/dx, -df/dy, 1).
            const float dx = (j == 0U)      ? (h[static_cast<size_t>(i) * fila + j] - h[static_cast<size_t>(i) * fila + (j + 1U)])
                           : (j == N_)      ? (h[static_cast<size_t>(i) * fila + (j - 1U)] - h[static_cast<size_t>(i) * fila + j])
                           :                  (h[static_cast<size_t>(i) * fila + (j - 1U)] - h[static_cast<size_t>(i) * fila + (j + 1U)]);
            const float dy = (i == 0U)      ? (h[static_cast<size_t>(i) * fila + j] - h[static_cast<size_t>(i + 1U) * fila + j])
                           : (i == N_)      ? (h[static_cast<size_t>(i - 1U) * fila + j] - h[static_cast<size_t>(i) * fila + j])
                           :                  (h[static_cast<size_t>(i - 1U) * fila + j] - h[static_cast<size_t>(i + 1U) * fila + j]);
            v.normal = glm::normalize(glm::vec3(dx, dy, 2.0f * paso_));
        }
    }

    malla_.actualizar_vertices(datos_.vertices);
}

void Terrain::update(const glm::vec3& pos_avion)
{
    // VENTANA INFINITA: el centro de la grilla sigue al avión PERO a saltos de
    // celda (múltiplos de paso_) en la HORIZONTAL (x, y). Así cada vértice vuelve
    // a caer sobre un nodo del heightmap y el relieve no "repta" (si el centro
    // siguiera al avión de forma continua, los vértices caerían entre nodos).
    const glm::vec2 nuevo(std::round(pos_avion.x / paso_) * paso_,
                          std::round(pos_avion.y / paso_) * paso_);
    if (primera_ || nuevo != centro_) {
        centro_ = nuevo;
        primera_ = false;
        recalcular_vertices();
    }

    // Matriz de MODELO relativa al avión (origen flotante). La parte HORIZONTAL
    // sigue al avión (ventana); la VERTICAL usa la ALTITUD FIJA del terreno
    // (altura_terreno_): como no sigue al avión en altura, si el avión desciende
    // el terreno queda cada vez más cerca -> se puede volar bajo y tocarlo.
    modelo_ = glm::translate(glm::mat4(1.0f),
                             glm::vec3(esquina().x - pos_avion.x,
                                       esquina().y - pos_avion.y,
                                       altura_terreno_ - pos_avion.z));
}

void Terrain::collect(std::vector<RenderItem>& items) const
{
    RenderItem item;
    item.mesh  = &malla_;
    item.model = modelo_;
    item.color = color_;
    items.push_back(item);
}
