// -----------------------------------------------------------------------------
// Aircraft.cpp — armado de la aeronave (Práctico 04)
//
// Despiece del SIAI Marchetti S-211 (medidas relativas al largo del fuselaje
// L = 1; sistema del modelo: origen en la nariz, X+ hacia la cola, Y+ hacia
// el ala derecha, Z+ hacia arriba):
//
//   pieza          primitiva   malla   medidas                  matriz local
//   ---------------------------------------------------------------------------
//   nariz          cone (1,1)  A       r=0.09  h=0.20           T(0.10,0,0)  Rz(+90) S(0.09,0.20,0.09)
//   fuselaje       cylinder    B       r=0.09  l=0.50           T(0.45,0,0)  Rz(-90)
//   cola           cone (1,1)  A       r=0.09  h=0.30           T(0.85,0,0)  Rz(-90) S(0.09,0.30,0.09)
//   ala izquierda  cube        C       S(0.16, 0.39, 0.03)      T(0.42,+0.285,-0.005) S(...)
//   ala derecha    cube        C       S(0.16, 0.39, 0.03)      T(0.42,-0.285,-0.005) S(...)
//   estab. horiz.  cube        C       S(0.09, 0.30, 0.02)      T(0.90,0,0)      S(...)
//   deriva         cube        C       S(0.10, 0.02, 0.12)      T(0.89,0,0.07)  S(...)
//
//   - cilindro y conos nacen con el eje en Y: Rz(-90) lo acuesta hacia +X
//     (cola); Rz(+90) lo apunta hacia -X (nariz).
//   - las placas nacen como cubos de lado 1 centrados en el origen y se
//     deforman con escalado NO uniforme (sus caras siguen siendo planos
//     axiales: la normal no se rompe, a diferencia de lo que pasaría con
//     un cilindro).
//   - el ala derecha NO usa escalado negativo (rompería el winding): es la
//     MISMA malla con la traslación al lado opuesto.
//   - Práctico 09 (Parte 3, decisión 17): la nariz y la cola salen de UN
//     MISMO cono UNITARIO (radio 1, alto 1) estirado con un escalado NO
//     uniforme S(r, h, r) (r en los ejes del radio, h en el eje). Hasta el
//     P08 cada cono se generaba con su radio y su alto. La FORMA es la misma;
//     lo que cambia es que ahora hay piezas cuya normal está INCLINADA
//     respecto de los ejes del escalado (la del lateral del cono tiene una
//     parte radial y otra axial, que se escalan distinto). En esas piezas,
//     transformar la normal con M en lugar de la matriz normal da un error
//     visible: es la comparación que pide la guía (tecla X). En el avión del
//     P08 no había ninguna pieza así y la tecla X no habría cambiado nada.
//
// 3 mallas distintas, 7 piezas (y 7 matrices locales): no coinciden porque
// las cuatro placas comparten la malla del cubo, y la nariz y la cola, la del
// cono unitario (P09; hasta el P08 eran 4 mallas).
// -----------------------------------------------------------------------------

#include "core/Aircraft.h"               // la declaración de la clase que se implementa acá

#include <cmath>                         // std::atan: ángulo de los conos de la nariz y la cola

#include <glm/gtc/matrix_transform.hpp>  // glm::translate, rotate, scale: matrices de cada pieza

#include "core/Primitives.h"             // primitives::cube, cylinder, cone: las mallas de las piezas

void Aircraft::init()
{
    // ------------------------------------------------------------------
    // 1) Mallas DISTINTAS (4): se generan y se suben a la GPU una sola vez.
    //    La conicidad sale de la relación radio/altura (la altura del cono
    //    es radio/tan(alpha/2), ver Práctico 03).
    // ------------------------------------------------------------------
    std::vector<MeshData> datos;

    // A: el cono UNITARIO de la nariz y la cola (P09, decisión 17). Radio 1 y
    //    alto 1: semiángulo atan(radio/alto) = atan(1) = 45°, apertura 90°.
    //    Cada pieza lo estira con su matriz local (S(r, h, r), más abajo).
    datos.push_back(primitives::cone(1.0f, 2.0f * std::atan(1.0f), 20U));
    datos.push_back(primitives::cylinder(0.09f, 0.50f, 20U)); // B: fuselaje
    datos.push_back(primitives::cube());                      // C: placas

    piezas_.resize(datos.size());
    for (std::size_t i = 0; i < datos.size(); ++i) {
        piezas_[i].load(datos[i]);
    }

    // ------------------------------------------------------------------
    // 2) Matrices locales por PIEZA (7), en coordenadas del modelo, desde
    //    la identidad. Mlocal ubica la primitiva DENTRO del modelo: la
    //    orienta (rotación), la escala (placas) y la traslada a su lugar.
    //    Es FIJA: se arma una sola vez acá y no cambia por cuadro (solo
    //    Mpose cambia). En glm lo escrito más a la derecha se aplica
    //    primero: primero se rota/escala la primitiva y después se traslada.
    // ------------------------------------------------------------------
    locales_.clear();
    colores_.clear();
    malla_de_.clear();
    escalas_uv_.clear();
    escalas_camuflaje_.clear();

    // Práctico 08: cada pieza guarda también cuántas veces se repite cada
    // textura sobre ella en (u, v):
    //   - CHAPA: una copia cada ~0.3 u (~3 m), para que los remaches tengan un
    //     tamaño parecido en todas las piezas.
    //   - CAMUFLAJE: una copia cada ~0.8 u, para que las manchas sean grandes
    //     respecto del avión (como la pintura de un avión real). En los conos
    //     (proyección radial, u y v en [0,1]) se usa un recorte de 0.25 de la
    //     imagen: si no, las manchas quedarían chicas en la nariz y la cola.
    // En el CILINDRO la repetición en u (alrededor) tiene que ser ENTERA: la
    // costura (u = 0 y u = 1) cae en el mismo lugar y, con un valor no entero,
    // la textura no empalmaría ahí.
    //
    // agregar(...) REGISTRA UNA PIEZA del avión. Es una lambda (función local)
    // que no dibuja ni crea nada en la GPU: solo agrega un elemento a cada una
    // de las 5 listas paralelas de la clase (la pieza k ocupa la posición k en
    // todas). [&] = usa por referencia las listas del objeto (locales_, ...).
    // Sus 5 parámetros y DE DÓNDE SALEN los valores de cada llamada:
    //
    //   1. local (mat4): DÓNDE va la pieza dentro del avión (Mlocal). Sale del
    //      DESPIECE del Práctico 04 (tabla al principio de este archivo y en
    //      Practico04-Decisiones.md): proporciones parecidas al S-211, en
    //      unidades relativas al largo L = 1 (la guía pide no usar datos
    //      reales). Las traslaciones se deducen de las medidas:
    //        - nariz  0.10 = mitad de su tramo 0 .. 0.20
    //        - fuselaje 0.45 = mitad de 0.20 .. 0.70
    //        - cola   0.85 = mitad de 0.70 .. 1.00
    //        - alas  y = ±0.285 = radio del fuselaje 0.09 + medio ala 0.39/2
    //                (el ala empieza justo en el costado del fuselaje);
    //                z = -0.005: un poco por debajo del eje (ala baja)
    //        - deriva z = 0.07 = medio alto 0.12/2 + 0.01 (apoyada sobre la cola)
    //   2. color (vec3): color plano de la pieza (modo "colores", tecla T).
    //      Elegidos a mano en el Práctico 04 para DISTINGUIR las piezas (rojo
    //      el fuselaje, azul las alas, ...), no para que parezca real.
    //   3. malla (índice): qué malla de piezas_ usa: 0 = cono unitario (A,
    //      nariz y cola), 1 = cilindro (B), 2 = cubo (C, las 4 placas).
    //   4. escala_uv (vec2): cuántas veces se repite la CHAPA en (u, v).
    //      Regla: una copia cada ~0.3 u, o sea medida de la cara / 0.3:
    //        fuselaje: perímetro 2·π·0.09 = 0.57 -> 1.9 -> 2 (ENTERO, por la
    //                  costura); largo 0.50 -> 1.7 -> 2
    //        alas: 0.16 / 0.3 = 0.53 -> 0.6;  0.39 / 0.3 = 1.3
    //        estabilizador: 0.09 -> 0.3;  0.30 -> 1.0
    //        deriva: 0.10 -> 0.33 -> 0.4;  0.12 -> 0.4
    //        conos: 1 (proyección radial, una sola copia)
    //   5. escala_camuflaje (vec2): lo mismo para el CAMUFLAJE, con una copia
    //      cada ~0.8 u (manchas grandes):
    //        fuselaje: 0.57 / 0.8 = 0.7 -> 1 (ENTERO);  0.50 / 0.8 = 0.6
    //        alas: 0.16 / 0.8 = 0.2;  0.39 / 0.8 = 0.49 -> 0.5
    //        estabilizador: 0.09 -> 0.11 -> 0.12;  0.30 -> 0.375 -> 0.38
    //        deriva: 0.10 -> 0.125 -> 0.13;  0.12 -> 0.15
    //        conos: 0.25 (elegido mirando el resultado, ver arriba)
    auto agregar = [&](const glm::mat4& local, const glm::vec3& color,
                       std::size_t malla, const glm::vec2& escala_uv,
                       const glm::vec2& escala_camuflaje) {
        locales_.push_back(local);
        colores_.push_back(color);
        malla_de_.push_back(malla);
        escalas_uv_.push_back(escala_uv);
        escalas_camuflaje_.push_back(escala_camuflaje);
    };

    // Nariz: cono con el ápice hacia la nariz (x=0) y la base en x=0.20.
    // En glm lo escrito más a la derecha se aplica primero: rotate()/scale()
    // post-multiplican, así que scale(rotate(translate(I, t), ...), s) =
    // T · R · S: el cono se ESTIRA primero (en sus propios ejes: el eje es Y),
    // después se rota y al final se traslada.
    // P09: S(0.09, 0.20, 0.09) lleva el cono unitario a radio 0.09 y alto
    // 0.20, las medidas del P04 (el estirado es NO uniforme: 0.09 en el radio
    // y 0.20 en el eje).
    const glm::mat4 m_nariz = glm::scale(
        glm::rotate(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.10f, 0.0f, 0.0f)),
            glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::vec3(0.09f, 0.20f, 0.09f));
    agregar(m_nariz, glm::vec3(0.75f, 0.75f, 0.80f), 0U,
            glm::vec2(1.0f, 1.0f),      // cono: proyección radial, una copia
            glm::vec2(0.25f, 0.25f));

    // Fuselaje: cilindro acostado sobre X, de x=0.20 a x=0.70.
    const glm::mat4 m_fuselaje = glm::rotate(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.45f, 0.0f, 0.0f)),
        glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    agregar(m_fuselaje, glm::vec3(0.85f, 0.25f, 0.20f), 1U,
            glm::vec2(2.0f, 2.0f),      // perímetro 0.57 u y largo 0.5 u -> 2 x 2
            glm::vec2(1.0f, 0.6f));     // camuflaje: 1 vuelta (entera) x 0.6

    // Cola: cono afinándose hacia la cola, de x=0.70 a x=1.00. Mismo cono
    // unitario que la nariz (malla 0), estirado a radio 0.09 y alto 0.30.
    const glm::mat4 m_cola = glm::scale(
        glm::rotate(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.85f, 0.0f, 0.0f)),
            glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::vec3(0.09f, 0.30f, 0.09f));
    agregar(m_cola, glm::vec3(0.95f, 0.80f, 0.25f), 0U,
            glm::vec2(1.0f, 1.0f),      // cono: proyección radial, una copia
            glm::vec2(0.25f, 0.25f));

    // Alas: dos placas, la misma malla trasladada a cada lado del fuselaje.
    const glm::vec3 escala_ala(0.16f, 0.39f, 0.03f);
    const glm::mat4 m_ala_izq = glm::scale(
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(0.42f, 0.285f, -0.005f)),
        escala_ala);
    agregar(m_ala_izq, glm::vec3(0.30f, 0.50f, 0.90f), 2U,
            glm::vec2(0.6f, 1.3f),      // cara de arriba 0.16 x 0.39 u
            glm::vec2(0.2f, 0.5f));

    const glm::mat4 m_ala_der = glm::scale(
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(0.42f, -0.285f, -0.005f)),
        escala_ala);
    agregar(m_ala_der, glm::vec3(0.30f, 0.50f, 0.90f), 2U,
            glm::vec2(0.6f, 1.3f),
            glm::vec2(0.2f, 0.5f));

    // Estabilizador horizontal (placa trasera, a la altura del fuselaje).
    const glm::mat4 m_estab = glm::scale(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.90f, 0.0f, 0.0f)),
        glm::vec3(0.09f, 0.30f, 0.02f));
    agregar(m_estab, glm::vec3(0.30f, 0.80f, 0.45f), 2U,
            glm::vec2(0.3f, 1.0f),      // 0.09 x 0.30 u
            glm::vec2(0.12f, 0.38f));

    // Deriva (empenaje vertical, placa parada sobre la cola).
    const glm::mat4 m_deriva = glm::scale(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.89f, 0.0f, 0.07f)),
        glm::vec3(0.10f, 0.02f, 0.12f));
    agregar(m_deriva, glm::vec3(0.55f, 0.40f, 0.85f), 2U,
            glm::vec2(0.4f, 0.4f),      // 0.10 x 0.12 u
            glm::vec2(0.13f, 0.15f));

    // ------------------------------------------------------------------
    // 3) Punto de referencia de la pose: a 0.438·Lf desde la nariz, sobre
    //    el eje (Lf = largo total del avión = 1). Es la posición que indica
    //    la imagen de la guía para aplicar las transformaciones de pose.
    // ------------------------------------------------------------------
    punto_ref_ = glm::vec3(0.438f, 0.0f, 0.0f);

    // ------------------------------------------------------------------
    // 4) Práctico 08: colores del modo "metal gris". Todo el avión de un gris
    //    metálico claro y la NARIZ (la primera pieza agregada) más oscura,
    //    como el radomo de un jet. La chapa los tiñe con rayones y remaches.
    // ------------------------------------------------------------------
    //    (Gris medio y no claro: al compensar el brillo de la chapa los texels
    //    más claros se recortan a blanco; con un gris claro el avión se veía
    //    casi blanco y "lavado".)
    colores_metal_.assign(locales_.size(), glm::vec3(0.56f, 0.58f, 0.61f));
    colores_metal_[0] = glm::vec3(0.26f, 0.27f, 0.29f);   // nariz (radomo)
}

void Aircraft::update(const FlightData& flight)
{
    // Pose del avión a partir de los datos de vuelo (Práctico 06).
    // Los ejes y sentidos se adaptan al MODELO y a la ESCENA (Z arriba,
    // nariz -X, ala derecha +Y) — verificado por cálculo:
    //   - guiñada (psi, alrededor de la vertical Z): se niega psi para que
    //     psi>0 gire la nariz hacia la derecha (convención aeronáutica).
    //   - cabeceo (theta, alrededor del ala Y): theta>0 sube la nariz.
    //   - alabeo  (phi, alrededor del eje de la nariz -X): phi>0 baja el ala
    //     derecha.
    // El orden de composición hereda el 3-2-1 del FDM (psi -> theta -> phi):
    // en glm (post-multiplica) eso es Rz * Ry * Rx_nala, con la de más a la
    // derecha aplicada primero.
    glm::mat4 pose = glm::mat4(1.0f);
    pose = glm::translate(pose, flight.position);
    pose = glm::rotate(pose, -flight.psi,   glm::vec3(0.0f, 0.0f, 1.0f)); // guiñada
    pose = glm::rotate(pose,  flight.theta, glm::vec3(0.0f, 1.0f, 0.0f)); // cabeceo
    pose = glm::rotate(pose,  flight.phi,   glm::vec3(-1.0f, 0.0f, 0.0f)); // alabeo

    // El FDM da la posición del centro de gravedad: se lo hace coincidir con
    // el origen del modelo (punto_ref_) para que la rotación sea alrededor de
    // él (mismo T(-ref) del Práctico 04).
    pose = glm::translate(pose, -punto_ref_);
    pose_ = pose;
}

// Práctico 09 (Parte 3, decisión 6): BRILLO (ks, ns) POR APARIENCIA. El kd
// sale del color de cada pieza (como el color del P08); ks y ns describen de
// QUÉ está hecha la superficie, y eso lo decide la apariencia, no la pieza:
//
//   apariencia      ks     ns   por qué
//   -----------------------------------------------------------------------
//   camuflaje       0.10    8   pintura MATE (militar): un brillo débil y
//                               ancho, apenas un "lustre" del lado del sol.
//   metal gris      0.60   64   chapa de aluminio sin pintar: brillo fuerte
//                               y chico, el que se espera de un metal.
//   colores         0.40   32   plástico (los colores del P04 parecen un
//                               modelo de juguete): brillo blanco mediano.
//   prueba UV       0.00    1   verificación de las (u, v): sin brillo, que
//                               no tape la imagen de prueba.
//
// El brillo se suma al color con el color del SOL (iluminar(), código común):
// ks = 0,6 con el sol en 0,7 agrega hasta 0,42 en el centro del reflejo.
// La tecla R (Blinn-Phong) multiplica ns por 4 en el shader, para comparar
// un reflejo de tamaño parecido.
static Material material_apariencia(AparienciaAvion modo, const glm::vec3& kd)
{
    Material m;
    m.kd = kd;
    switch (modo) {
        case AparienciaAvion::Camuflaje:     m.ks = glm::vec3(0.10f); m.ns = 8.0f;  break;
        case AparienciaAvion::MetalGris:     m.ks = glm::vec3(0.60f); m.ns = 64.0f; break;
        case AparienciaAvion::ColoresPiezas: m.ks = glm::vec3(0.40f); m.ns = 32.0f; break;
        case AparienciaAvion::PruebaUV:      m.ks = glm::vec3(0.0f);  m.ns = 1.0f;  break;
    }
    return m;
}

void Aircraft::collect(std::vector<RenderItem>& items) const
{
    // Un RenderItem por pieza, con la transformación ya compuesta.
    // El resto del programa no sabe cuántas piezas hay ni cómo están
    // armadas: solo recorre esta lista y dibuja. (collect NO dibuja: solo
    // describe QUÉ dibujar; el draw call vive en main.)
    //
    // Hay MENOS mallas que piezas (3 vs 7): las cuatro placas comparten la
    // malla del cubo y la nariz y la cola, la del cono unitario (P09).
    // malla_de_[i] guarda, para cada pieza, QUÉ malla le toca (índice dentro
    // de piezas_). Por eso va &piezas_[malla_de_[i]] y no &piezas_[i].
    for (std::size_t i = 0; i < locales_.size(); ++i) {
        RenderItem item;
        item.mesh  = &piezas_[malla_de_[i]];
        item.model = pose_ * locales_[i];

        // Práctico 08: apariencia. Práctico 09: el color que elige cada caso
        // es el kd del material; el shader de la aeronave hace kd * textura
        // (o solo kd si textura == nullptr).
        glm::vec3 kd = colores_[i];
        switch (apariencia_) {
            case AparienciaAvion::ColoresPiezas:
                // Colores planos del Práctico 04, sin textura.
                item.textura = nullptr;
                break;
            case AparienciaAvion::Camuflaje:
                // La pintura está EN la textura: kd blanco (1,1,1) para que
                // la multiplicación no la cambie, y manchas grandes.
                item.textura   = textura_;
                kd             = glm::vec3(1.0f);
                item.escala_uv = escalas_camuflaje_[i];
                break;
            case AparienciaAvion::MetalGris:
                // La chapa TIÑE el gris de cada pieza. Como su brillo medio es
                // ~0.57, multiplicar oscurecería la pieza un 43 %; se compensa
                // dividiendo el color por ese brillo medio: en promedio la pieza
                // conserva su gris y la textura solo agrega variación (rayones,
                // remaches). Los texels más claros pueden dar un kd·texel mayor
                // que 1 (con luz, la cara de frente al sol se recorta a blanco).
                item.textura   = textura_;
                kd             = textura_ != nullptr
                                     ? colores_metal_[i] / textura_->brillo_medio()
                                     : colores_metal_[i];
                item.escala_uv = escalas_uv_[i];
                break;
            case AparienciaAvion::PruebaUV:
                // Verificación de (u, v) (P08, Parte 3): la imagen de prueba TAL
                // CUAL (kd blanco) y UNA copia por cara, para ver si sale derecha.
                item.textura   = textura_;
                kd             = glm::vec3(1.0f);
                item.escala_uv = glm::vec2(1.0f);
                break;
        }
        item.material = material_apariencia(apariencia_, kd);   // P09: kd + brillo de la apariencia
        items.push_back(item);
    }
}

void Aircraft::set_apariencia(AparienciaAvion modo, const Texture* textura)
{
    apariencia_ = modo;
    textura_ = textura;
}
