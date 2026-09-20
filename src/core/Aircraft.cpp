// -----------------------------------------------------------------------------
// Aircraft.cpp — armado de la aeronave (Práctico 04)
//
// Despiece del SIAI Marchetti S-211 (medidas relativas al largo del fuselaje
// L = 1; sistema del modelo: origen en la nariz, X+ hacia la cola, Y+ hacia
// el ala derecha, Z+ hacia arriba):
//
//   pieza          primitiva   malla   medidas                  matriz local
//   ---------------------------------------------------------------------------
//   nariz          cone        A       r=0.09  h=0.20           T(0.10,0,0)  Rz(+90)
//   fuselaje       cylinder    B       r=0.09  l=0.50           T(0.45,0,0)  Rz(-90)
//   cola           cone        C       r=0.09  h=0.30           T(0.85,0,0)  Rz(-90)
//   ala izquierda  cube        D       S(0.16, 0.39, 0.03)      T(0.42,+0.285,-0.005) S(...)
//   ala derecha    cube        D       S(0.16, 0.39, 0.03)      T(0.42,-0.285,-0.005) S(...)
//   estab. horiz.  cube        D       S(0.09, 0.30, 0.02)      T(0.90,0,0)      S(...)
//   deriva         cube        D       S(0.10, 0.02, 0.12)      T(0.89,0,0.07)  S(...)
//
//   - cilindro y conos nacen con el eje en Y: Rz(-90) lo acuesta hacia +X
//     (cola); Rz(+90) lo apunta hacia -X (nariz).
//   - las placas nacen como cubos de lado 1 centrados en el origen y se
//     deforman con escalado NO uniforme (sus caras siguen siendo planos
//     axiales: la normal no se rompe, a diferencia de lo que pasaría con
//     un cilindro).
//   - el ala derecha NO usa escalado negativo (rompería el winding): es la
//     MISMA malla con la traslación al lado opuesto.
//
// 4 mallas distintas, 7 piezas (y 7 matrices locales): no coinciden porque
// las cuatro placas comparten la malla del cubo.
// -----------------------------------------------------------------------------

#include "core/Aircraft.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "core/Primitives.h"

void Aircraft::init()
{
    // ------------------------------------------------------------------
    // 1) Mallas DISTINTAS (4): se generan y se suben a la GPU una sola vez.
    //    La conicidad sale de la relación radio/altura (la altura del cono
    //    es radio/tan(alpha/2), ver Práctico 03).
    // ------------------------------------------------------------------
    std::vector<MeshData> datos;

    datos.push_back(primitives::cone(
        0.09f, 2.0f * std::atan(0.09f / 0.20f), 20U));        // A: nariz
    datos.push_back(primitives::cylinder(0.09f, 0.50f, 20U)); // B: fuselaje
    datos.push_back(primitives::cone(
        0.09f, 2.0f * std::atan(0.09f / 0.30f), 20U));        // C: cola
    datos.push_back(primitives::cube());                      // D: placas

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

    auto agregar = [&](const glm::mat4& local, const glm::vec3& color,
                       std::size_t malla) {
        locales_.push_back(local);
        colores_.push_back(color);
        malla_de_.push_back(malla);
    };

    // Nariz: cono con el ápice hacia la nariz (x=0) y la base en x=0.20.
    // En glm lo escrito más a la derecha se aplica primero: rotate()/scale()
    // post-multiplican, así que rotate(translate(I, t), ...) = T · R y el
    // cono se rota ANTES de trasladarlo.
    const glm::mat4 m_nariz = glm::rotate(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.10f, 0.0f, 0.0f)),
        glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    agregar(m_nariz, glm::vec3(0.75f, 0.75f, 0.80f), 0U);

    // Fuselaje: cilindro acostado sobre X, de x=0.20 a x=0.70.
    const glm::mat4 m_fuselaje = glm::rotate(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.45f, 0.0f, 0.0f)),
        glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    agregar(m_fuselaje, glm::vec3(0.85f, 0.25f, 0.20f), 1U);

    // Cola: cono afinándose hacia la cola, de x=0.70 a x=1.00.
    const glm::mat4 m_cola = glm::rotate(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.85f, 0.0f, 0.0f)),
        glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    agregar(m_cola, glm::vec3(0.95f, 0.80f, 0.25f), 2U);

    // Alas: dos placas, la misma malla trasladada a cada lado del fuselaje.
    const glm::vec3 escala_ala(0.16f, 0.39f, 0.03f);
    const glm::mat4 m_ala_izq = glm::scale(
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(0.42f, 0.285f, -0.005f)),
        escala_ala);
    agregar(m_ala_izq, glm::vec3(0.30f, 0.50f, 0.90f), 3U);

    const glm::mat4 m_ala_der = glm::scale(
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(0.42f, -0.285f, -0.005f)),
        escala_ala);
    agregar(m_ala_der, glm::vec3(0.30f, 0.50f, 0.90f), 3U);

    // Estabilizador horizontal (placa trasera, a la altura del fuselaje).
    const glm::mat4 m_estab = glm::scale(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.90f, 0.0f, 0.0f)),
        glm::vec3(0.09f, 0.30f, 0.02f));
    agregar(m_estab, glm::vec3(0.30f, 0.80f, 0.45f), 3U);

    // Deriva (empenaje vertical, placa parada sobre la cola).
    const glm::mat4 m_deriva = glm::scale(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.89f, 0.0f, 0.07f)),
        glm::vec3(0.10f, 0.02f, 0.12f));
    agregar(m_deriva, glm::vec3(0.55f, 0.40f, 0.85f), 3U);

    // ------------------------------------------------------------------
    // 3) Punto de referencia de la pose: a 0.438·Lf desde la nariz, sobre
    //    el eje (Lf = largo total del avión = 1). Es la posición que indica
    //    la imagen de la guía para aplicar las transformaciones de pose.
    // ------------------------------------------------------------------
    punto_ref_ = glm::vec3(0.438f, 0.0f, 0.0f);
}

void Aircraft::update(const glm::vec3& pos, const glm::vec3& angulos)
{
    // Mpose = T(pos) * Rx(rolido) * Ry(cabeceo) * Rz(guiñada) * T(-ref)
    // Los tres ángulos de ACTITUD (rolido/cabeceo/guiñada) definen, junto con
    // la posición, la orientación del avión en el mundo (su "pose"):
    //   - cabeceo (pitch): sube/baja la NARIZ, alrededor del eje Y (lateral)
    //   - rolido  (roll):  inclina el avión de lado, alrededor del eje X
    //     (longitudinal, nariz->cola)
    //   - guiñada (yaw):   gira la proa a izq/der, alrededor del eje Z (vertical)
    // Hoy solo se usa el cabeceo (verificación); roll/yaw quedan para el FDM.
    // Lo escrito más a la derecha se aplica primero: se lleva el punto de
    // referencia al origen, se rota y por último se traslada a la posición
    // del avión en el mundo. Sin el T(-ref) el avión giraría "en arco"
    // alrededor del origen del modelo (la nariz).
    glm::mat4 pose = glm::mat4(1.0f);
    pose = glm::translate(pose, pos);
    pose = glm::rotate(pose, angulos.z, glm::vec3(1.0f, 0.0f, 0.0f));  // rolido
    pose = glm::rotate(pose, angulos.x, glm::vec3(0.0f, 1.0f, 0.0f));  // cabeceo
    pose = glm::rotate(pose, angulos.y, glm::vec3(0.0f, 0.0f, 1.0f));  // guiñada
    pose = glm::translate(pose, -punto_ref_);
    pose_ = pose;
}

void Aircraft::collect(std::vector<RenderItem>& items) const
{
    // Un RenderItem por pieza, con la transformación ya compuesta.
    // El resto del programa no sabe cuántas piezas hay ni cómo están
    // armadas: solo recorre esta lista y dibuja. (collect NO dibuja: solo
    // describe QUÉ dibujar; el draw call vive en main.)
    //
    // Hay MENOS mallas que piezas (4 vs 7): las cuatro placas comparten la
    // malla del cubo. malla_de_[i] guarda, para cada pieza, QUÉ malla le
    // toca (índice dentro de piezas_). Por eso va &piezas_[malla_de_[i]] y
    // no &piezas_[i].
    for (std::size_t i = 0; i < locales_.size(); ++i) {
        RenderItem item;
        item.mesh  = &piezas_[malla_de_[i]];
        item.model = pose_ * locales_[i];
        item.color = colores_[i];
        items.push_back(item);
    }
}
