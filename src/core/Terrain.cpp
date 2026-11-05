// -----------------------------------------------------------------------------
// Terrain.cpp — terreno procedural (Práctico 07)
// -----------------------------------------------------------------------------

#include "core/Terrain.h"

#include <algorithm>
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
// Terrain — geometry clipmaps (etapa 1)
// -----------------------------------------------------------------------------
Terrain::Terrain(unsigned N, float paso, unsigned niveles, float amplitud,
                 float altura_terreno, int semilla, float frecuencia)
    : N_(N)
    , paso_(paso)
    , altura_terreno_(altura_terreno)
    , alturas_(semilla, frecuencia, amplitud)
{
    // N múltiplo de 4: así N/2 (lado del hueco) es par y las esquinas caen
    // sobre vértices del nivel siguiente.
    if (N_ % 4U != 0U) {
        N_ += 4U - N_ % 4U;
    }
    if (niveles == 0U) {
        niveles = 1U;
    }
    // Ancho de la región de transición: el paper encontró que w = n/10 anda
    // bien (más chico -> se notan los bordes; más grande -> se pierde detalle).
    ancho_transicion_ = std::max(1.0f, static_cast<float>(N_) / 10.0f);

    niveles_.resize(niveles);
    for (unsigned l = 0; l < niveles; ++l) {
        niveles_[l].paso = paso_ * static_cast<float>(1U << l);
    }
}

Terrain::~Terrain() = default;

float Terrain::extension() const
{
    return static_cast<float>(N_) * niveles_.back().paso;
}

glm::vec2 Terrain::esquina(const Nivel& nv) const
{
    return nv.centro - glm::vec2(0.5f * static_cast<float>(N_) * nv.paso);
}

glm::vec2 Terrain::centro_para(float paso_l, const glm::vec3& pos) const
{
    // Saltos de 2·paso_l (ver alineación en Terrain.h).
    const float salto = 2.0f * paso_l;
    return glm::vec2(std::round(pos.x / salto) * salto,
                     std::round(pos.y / salto) * salto);
}

glm::ivec2 Terrain::calcular_hueco(std::size_t l) const
{
    // Posición de la esquina del nivel fino (l-1) dentro del nivel l, en CELDAS
    // del nivel l. Por la alineación es un entero exacto (lround solo absorbe
    // el error de coma flotante).
    if (l == 0U) {
        return glm::ivec2(-1);
    }
    const glm::vec2 d = (esquina(niveles_[l - 1U]) - esquina(niveles_[l])) / niveles_[l].paso;
    return glm::ivec2(static_cast<int>(std::lround(d.x)),
                      static_cast<int>(std::lround(d.y)));
}

void Terrain::armar_indices(Nivel& nv, bool cerrar_borde) const
{
    // Misma triangulación que el Práctico 07 (a->c->d, a->b->c, antihorario
    // visto desde +Z), pero se SALTEAN las celdas del hueco: (N/2)x(N/2) celdas
    // a partir de nv.hueco. Como el hueco siempre tiene el mismo tamaño, la
    // cantidad de índices es constante -> se puede re-subir con SubData.
    const unsigned fila = N_ + 1U;
    const int h = static_cast<int>(N_ / 2U);
    nv.datos.indices.clear();
    for (unsigned i = 0; i < N_; ++i) {
        for (unsigned j = 0; j < N_; ++j) {
            const int ii = static_cast<int>(i), jj = static_cast<int>(j);
            if (nv.hueco.x >= 0 &&
                jj >= nv.hueco.x && jj < nv.hueco.x + h &&
                ii >= nv.hueco.y && ii < nv.hueco.y + h) {
                continue;                                    // lo dibuja el nivel fino
            }
            const unsigned a = i * fila + j,          b = a + 1U;
            const unsigned d = (i + 1U) * fila + j,   c = d + 1U;
            nv.datos.indices.insert(nv.datos.indices.end(), {a, c, d, a, b, c});
        }
    }

    // ETAPA 2 — TRIÁNGULOS DE ÁREA CERO sobre el borde exterior. Por cada
    // vértice IMPAR k del borde, el triángulo (k-1, k, k+1): con el morph
    // (α = 1 en el borde) el vértice impar queda en la recta entre sus vecinos
    // pares, o sea sobre la arista del nivel grueso -> el triángulo tiene área
    // cero (no se dibuja nada), pero comparte las aristas exactas del fino y
    // "cose" la T-junction ante errores de redondeo. Su cantidad es fija
    // (4 lados · N/2), así que el total de índices sigue siendo constante.
    if (cerrar_borde) {
        const unsigned u = N_;                       // último índice de vértice por lado
        auto at = [fila](unsigned i, unsigned j) { return i * fila + j; };
        for (unsigned k = 1; k < N_; k += 2U) {
            nv.datos.indices.insert(nv.datos.indices.end(), {at(0, k - 1U), at(0, k), at(0, k + 1U)});   // abajo
            nv.datos.indices.insert(nv.datos.indices.end(), {at(u, k + 1U), at(u, k), at(u, k - 1U)});   // arriba
            nv.datos.indices.insert(nv.datos.indices.end(), {at(k + 1U, 0), at(k, 0), at(k - 1U, 0)});   // izquierda
            nv.datos.indices.insert(nv.datos.indices.end(), {at(k - 1U, u), at(k, u), at(k + 1U, u)});   // derecha
        }
    }
}

void Terrain::init()
{
    primera_ = true;
    for (std::size_t l = 0; l < niveles_.size(); ++l) {
        Nivel& nv = niveles_[l];
        nv.centro = glm::vec2(0.0f);
        // generar_malla_terreno da los vértices (con este paso_l) y una grilla
        // completa de índices; los índices se rehacen con el hueco.
        nv.datos = generar_malla_terreno(N_, nv.paso, esquina(nv), alturas_);
        nv.hueco = calcular_hueco(l);
        armar_indices(nv, l + 1U < niveles_.size());   // el más grueso no tiene vecino afuera
        recalcular_vertices(nv);                       // completa z_c (malla aún sin VBO: no sube)
        nv.malla.load(nv.datos);
    }
}

void Terrain::recalcular_vertices(Nivel& nv)
{
    // Igual que el Práctico 07, con el paso del nivel: alturas UNA vez por nodo
    // en una grilla temporal y normales por diferencias desde ahí.
    const unsigned fila = N_ + 1U;
    const float p = nv.paso;
    const glm::vec2 esq = esquina(nv);
    auto at = [fila](unsigned i, unsigned j) { return static_cast<std::size_t>(i) * fila + j; };

    std::vector<float> h(static_cast<std::size_t>(fila) * fila);
    for (unsigned i = 0; i <= N_; ++i) {
        for (unsigned j = 0; j <= N_; ++j) {
            h[at(i, j)] = alturas_(esq.x + static_cast<float>(j) * p,
                                   esq.y + static_cast<float>(i) * p);
        }
    }
    for (unsigned i = 0; i <= N_; ++i) {
        for (unsigned j = 0; j <= N_; ++j) {
            Vertex& v = nv.datos.vertices[at(i, j)];
            v.position = glm::vec3(static_cast<float>(j) * p, static_cast<float>(i) * p, h[at(i, j)]);
            const float dx = (j == 0U) ? (h[at(i, j)] - h[at(i, j + 1U)])
                           : (j == N_) ? (h[at(i, j - 1U)] - h[at(i, j)])
                           :             (h[at(i, j - 1U)] - h[at(i, j + 1U)]);
            const float dy = (i == 0U) ? (h[at(i, j)] - h[at(i + 1U, j)])
                           : (i == N_) ? (h[at(i - 1U, j)] - h[at(i, j)])
                           :             (h[at(i - 1U, j)] - h[at(i + 1U, j)]);
            v.normal = glm::normalize(glm::vec3(dx, dy, 2.0f * p));

            // ETAPA 2 — z_c: altura de este punto en el NIVEL GRUESO (paso 2p).
            // Los vértices de índice PAR coinciden con vértices del grueso
            // (por la alineación, la esquina cae sobre uno). Los impares caen
            // sobre una arista o en el centro de una celda gruesa, y el grueso
            // ahí vale la INTERPOLACIÓN LINEAL de sus extremos:
            //   (par, par)     -> z mismo (es un vértice del grueso)
            //   (par, impar)   -> promedio izquierda/derecha (arista horizontal)
            //   (impar, par)   -> promedio abajo/arriba     (arista vertical)
            //   (impar, impar) -> promedio sobre la DIAGONAL de la celda gruesa,
            //                     la misma que usa la triangulación (a -> c, de
            //                     (i-1, j-1) a (i+1, j+1)).
            // N es par, así que un índice impar siempre tiene vecinos i±1.
            const bool ii = (i % 2U) != 0U, jj = (j % 2U) != 0U;
            if (!ii && !jj)     v.altura_gruesa = h[at(i, j)];
            else if (!ii && jj) v.altura_gruesa = 0.5f * (h[at(i, j - 1U)] + h[at(i, j + 1U)]);
            else if (ii && !jj) v.altura_gruesa = 0.5f * (h[at(i - 1U, j)] + h[at(i + 1U, j)]);
            else                v.altura_gruesa = 0.5f * (h[at(i - 1U, j - 1U)] + h[at(i + 1U, j + 1U)]);
        }
    }
    nv.malla.actualizar_vertices(nv.datos.vertices);
}

void Terrain::update(const glm::vec3& pos_avion)
{
    // 1) Nuevos centros de TODOS los niveles (cada uno a saltos de 2·paso_l).
    std::vector<bool> movido(niveles_.size(), false);
    for (std::size_t l = 0; l < niveles_.size(); ++l) {
        const glm::vec2 nuevo = centro_para(niveles_[l].paso, pos_avion);
        if (primera_ || nuevo != niveles_[l].centro) {
            niveles_[l].centro = nuevo;
            movido[l] = true;
        }
    }
    primera_ = false;

    for (std::size_t l = 0; l < niveles_.size(); ++l) {
        Nivel& nv = niveles_[l];
        // 2) Alturas: solo si ESTE nivel se movió (los gruesos casi nunca).
        if (movido[l]) {
            recalcular_vertices(nv);
        }
        // 3) Hueco: se corre si se movió este nivel o el fino que lo rellena.
        if (l > 0U && (movido[l] || movido[l - 1U])) {
            const glm::ivec2 hueco = calcular_hueco(l);
            if (hueco != nv.hueco) {
                nv.hueco = hueco;
                armar_indices(nv, l + 1U < niveles_.size());
                nv.malla.actualizar_indices(nv.datos.indices);
            }
        }
        // 4) Modelo relativo al avión (origen flotante), altitud fija.
        const glm::vec2 esq = esquina(nv);
        nv.modelo = glm::translate(glm::mat4(1.0f),
                                   glm::vec3(esq.x - pos_avion.x,
                                             esq.y - pos_avion.y,
                                             altura_terreno_ - pos_avion.z));
    }
}

void Terrain::collect(std::vector<RenderItem>& items) const
{
    // Paleta de depuración (tecla L): un color por nivel, del fino al grueso.
    static const glm::vec3 paleta[] = {
        {0.90f, 0.30f, 0.25f}, {0.95f, 0.65f, 0.20f}, {0.85f, 0.85f, 0.30f},
        {0.35f, 0.75f, 0.35f}, {0.30f, 0.60f, 0.90f}, {0.60f, 0.40f, 0.85f},
        {0.85f, 0.45f, 0.75f}, {0.60f, 0.60f, 0.60f},
    };
    for (std::size_t l = 0; l < niveles_.size(); ++l) {
        RenderItem item;
        item.mesh  = &niveles_[l].malla;
        item.model = niveles_[l].modelo;
        item.color = colores_niveles_ ? paleta[l % 8U] : color_;
        // uMorph = (paso, N/2, w). El nivel más grueso no tiene a quién
        // parecerse afuera -> sin morph (paso 0). Con M apagado, todos en 0.
        const bool con_morph = morph_ && (l + 1U < niveles_.size());
        item.morph = con_morph
            ? glm::vec3(niveles_[l].paso, 0.5f * static_cast<float>(N_), ancho_transicion_)
            : glm::vec3(0.0f);
        items.push_back(item);
    }
}
