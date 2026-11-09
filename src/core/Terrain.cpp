// -----------------------------------------------------------------------------
// Terrain.cpp — terreno procedural (Práctico 07)
// -----------------------------------------------------------------------------

#include "core/Terrain.h"                // la declaración de la clase que se implementa acá

#include <algorithm>                     // std::max: ancho mínimo de la franja de geomorphing
#include <cmath>                         // std::floor, std::round, std::lround: módulo de las (u, v) y centros de los niveles

#include <glm/gtc/matrix_transform.hpp>  // glm::translate: la matriz de modelo de cada nivel

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

    // Práctico 08 — COORDENADAS DE TEXTURA EN COORDENADAS DE MUNDO.
    // La guía propone (u, v) = (j/N, i/N): una copia de la textura sobre toda la
    // malla. Con el clipmap eso NO sirve: cada nivel tiene otro tamaño, así que
    // el pasto cambiaría de escala en cada anillo (y saltaría al moverse la
    // ventana). En cambio, cada vértice guarda su posición en el mundo (x, y) y
    // el shader la divide por el tamaño de la tesela (uTamTesela): la textura
    // queda "pegada al mundo", igual en todos los niveles, y es la MISMA cuenta
    // que hace el suelo continuado del fondo (Parte 5) -> sin costura en el borde.
    // Para no acumular números grandes, el origen se toma MÓDULO kPeriodoUV
    // (en double, una vez por nivel; ver la explicación en Terrain.h).
    auto modulo = [](double x, double periodo) { return x - periodo * std::floor(x / periodo); };
    // El origen de las (u, v) de este nivel: la esquina del nivel, módulo el
    // período. Después, cada vértice suma su posición dentro del nivel.
    const glm::vec2 origen_uv(
        static_cast<float>(modulo(static_cast<double>(esq.x), kPeriodoUV)),
        static_cast<float>(modulo(static_cast<double>(esq.y), kPeriodoUV)));

    // --- PASO 1: la GRILLA TEMPORAL DE ALTURAS --------------------------------
    // Un nivel tiene (N+1) x (N+1) nodos (con N = 128: 129 x 129 = 16.641).
    // `h` es un arreglo de ese tamaño con la ALTURA de cada nodo; se guarda
    // "aplanado" en una sola fila de memoria: la fila i de la grilla ocupa
    // h[i*fila] .. h[i*fila + N]. Eso es lo que calcula at(i, j) = i*fila + j.
    //   i = número de FILA    -> recorre el eje y del mundo
    //   j = número de COLUMNA -> recorre el eje x del mundo
    //
    // Posición en el MUNDO del nodo (i, j): la esquina del nivel más j pasos en
    // x e i pasos en y:  (esq.x + j*p, esq.y + i*p). (p = paso de ESTE nivel.)
    // alturas_(x, y) es la fuente de alturas del Práctico 07 (clase Alturas,
    // ruido de FastNoiseLite): dada una posición del mundo, devuelve la altura
    // ya en unidades de escena. Se le pasa la posición del MUNDO (y no la
    // relativa al nivel) para que un mismo punto tenga la misma altura en
    // todos los niveles y aunque la ventana se mueva.
    //
    // Por qué una grilla aparte y no pedir la altura directo al armar cada
    // vértice: cada altura se usa VARIAS veces (la del vértice, la de las
    // normales de sus 4 vecinos y la altura gruesa de los vecinos impares), y
    // evaluar el ruido es lo CARO (varias octavas por punto). Así se evalúa
    // UNA vez por nodo y después se lee del arreglo.
    std::vector<float> h(static_cast<std::size_t>(fila) * fila);
    for (unsigned i = 0; i <= N_; ++i) {          // filas: de abajo (y chico) hacia arriba
        for (unsigned j = 0; j <= N_; ++j) {      // columnas: de izquierda (x chico) a derecha
            h[at(i, j)] = alturas_(esq.x + static_cast<float>(j) * p,
                                   esq.y + static_cast<float>(i) * p);
        }
    }

    // --- PASO 2: los VÉRTICES, leyendo las alturas de la grilla ------------------
    // Para cada nodo: posición (RELATIVA a la esquina del nivel; el RenderItem
    // la traslada al mundo), coordenadas de textura, normal por diferencias
    // con los vecinos y altura en el nivel grueso (para el geomorphing).
    for (unsigned i = 0; i <= N_; ++i) {
        for (unsigned j = 0; j <= N_; ++j) {
            Vertex& v = nv.datos.vertices[at(i, j)];
            v.position = glm::vec3(static_cast<float>(j) * p, static_cast<float>(i) * p, h[at(i, j)]);
            // (u, v) = posición en el mundo (módulo el período), en UNIDADES DE
            // ESCENA; el shader las pasa a "teselas" dividiendo por uTamTesela.
            // position.xy es la posición DENTRO del nivel (desde su esquina) y
            // origen_uv es esa esquina en el mundo, módulo el período: la suma
            // es la posición en el mundo (módulo el período).
            v.tex_coords = origen_uv + glm::vec2(v.position.x, v.position.y);

            // --- NORMAL por DIFERENCIAS CENTRALES (como en el Práctico 07) -------
            // La normal es la dirección perpendicular a la superficie en ese
            // punto: en lo plano apunta derecho hacia arriba (0, 0, 1) y en una
            // ladera se inclina hacia el lado que BAJA.
            //
            // Se arma con dos vectores TANGENTES que salen de mirar a los vecinos:
            //   en x: del vecino izquierdo (j-1) al derecho (j+1):
            //         Tx = (2p, 0, h[j+1] - h[j-1])   (avanza 2p y sube/baja eso)
            //   en y: del vecino de abajo (i-1) al de arriba (i+1):
            //         Ty = (0, 2p, h[i+1] - h[i-1])
            // La normal es el producto vectorial Tx x Ty, que (dividido por 2p)
            // da exactamente:
            //         ( h[j-1] - h[j+1],  h[i-1] - h[i+1],  2p )
            //           \_____ dx ____/   \_____ dy ____/
            // y después se normaliza (largo 1).
            //
            // Ejemplos:
            //   - plano: dx = dy = 0 -> (0, 0, 2p) -> normal (0, 0, 1).
            //   - sube hacia la derecha 1 paso por paso (45°): h[j+1] - h[j-1]
            //     = 2p -> dx = -2p -> (-2p, 0, 2p) -> (-0.71, 0, 0.71): inclinada
            //     45° hacia la IZQUIERDA, el lado que baja.
            //
            // Los ? : (operador ternario, "si ... entonces ... si no ...")
            // resuelven los BORDES de la grilla, donde falta un vecino:
            //   j == 0  (borde izquierdo, no hay j-1): usa el propio punto y el
            //           de la derecha   -> h[j] - h[j+1]
            //   j == N  (borde derecho, no hay j+1):  usa el de la izquierda y
            //           el propio punto -> h[j-1] - h[j]
            //   si no   (interior): la diferencia central h[j-1] - h[j+1].
            // Igual para dy con i (borde de abajo i == 0, de arriba i == N).
            // No se puede poner 0 en el borde: la normal quedaría vertical, y si
            // las tres componentes fueran 0, normalizar sería una división por 0.
            //
            // DETALLE CONOCIDO (para el Práctico 09): en el borde la diferencia
            // abarca UN paso (p) y no dos (2p), pero la z sigue siendo 2p: la
            // pendiente del borde queda a la MITAD y esa normal sale un poco más
            // vertical. Hoy no se nota (el 08 no ilumina). Con luz, se corrige
            // multiplicando por 2 las dos diferencias de un solo lado.
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
    // Con la depuración "solo nivel fino" (tecla 1) se entrega únicamente el
    // nivel 0: una ventana de N·paso como el terreno del Práctico 07.
    const std::size_t a_dibujar = solo_nivel_fino_ ? 1U : niveles_.size();
    for (std::size_t l = 0; l < a_dibujar; ++l) {
        RenderItem item;
        item.mesh  = &niveles_[l].malla;
        item.model = niveles_[l].modelo;
        item.color = colores_niveles_ ? paleta[l % 8U] : color_;
        // Práctico 08: textura del suelo. Con los colores de depuración (L) se
        // dibuja con color plano para que se vean los anillos.
        item.textura = colores_niveles_ ? nullptr : textura_;
        // uMorph = (paso, N/2, w). El nivel más grueso no tiene a quién
        // parecerse afuera -> sin morph (paso 0). Con M apagado, todos en 0.
        const bool con_morph = morph_ && (l + 1U < niveles_.size());
        item.morph = con_morph
            ? glm::vec3(niveles_[l].paso, 0.5f * static_cast<float>(N_), ancho_transicion_)
            : glm::vec3(0.0f);
        items.push_back(item);
    }
}
