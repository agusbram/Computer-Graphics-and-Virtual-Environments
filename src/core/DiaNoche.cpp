// -----------------------------------------------------------------------------
// DiaNoche.cpp — el reloj del ciclo día/noche y el color del sol
//                (Práctico 09, opcional 2; ver DiaNoche.h)
// -----------------------------------------------------------------------------

#include "core/DiaNoche.h"   // la declaración que se implementa acá

#include <cmath>             // std::fmod, std::floor, std::sqrt, std::sin, std::asin
#include <cstdio>            // std::snprintf: la hora como texto

#include <glm/gtc/matrix_transform.hpp>   // glm::rotate: el giro del sol alrededor del eje

namespace {

// --- Decisión 25: las velocidades del reloj -----------------------------------
// Un día completo (360°) en 2 min (normal) o en 30 s (rápido): 3°/s y 12°/s.
// Con la normal, el sol tarda ~10 s en ir desde la hora del pintado (29,8°)
// hasta el horizonte, y el día (de las 6:00 a las 18:00) dura 1 minuto.
constexpr float kSegundosPorDiaNormal = 120.0f;
constexpr float kSegundosPorDiaRapido = 30.0f;

// --- Decisión 27: el aire -----------------------------------------------------
// τ = ESPESOR ÓPTICO del aire mirando al cenit desde el nivel del mar: de la luz
// del sol llega la fracción e^(-τ). Es la misma ley de la niebla de la Parte 4,
// 1 - f = e^(-d/V): lo que atraviesa un medio cae exponencialmente con lo que
// atraviesa (ley de Beer-Lambert). La diferencia: la niebla trata igual a los
// tres canales y el aire no.
// Las moléculas del aire son mucho más chicas que la longitud de onda λ de la
// luz, y entonces la desvían como 1/λ⁴ (dispersión de RAYLEIGH): el azul
// (450 nm) se desvía ~5 veces más que el rojo (680 nm). La luz azul desviada es
// la que pinta el cielo; la que sigue derecho, la del sol, va quedando roja
// cuanto más aire cruza. Valores aproximados para λ = 680 nm (R), 550 nm (G) y
// 450 nm (B); no incluyen el polvo ni la humedad, que la atenúan más.
const glm::vec3 kTauRayleigh(0.0411f, 0.0974f, 0.2213f);

// Para el CAMINO por el aire: la Tierra es una esfera de radio R = 6371 km y el
// aire, una capa de espesor H = 8 km. (Es la "atmósfera homogénea": todo el
// aire, apretado a la densidad que tiene en el suelo, ocuparía unos 8 km.)
// En la cuenta solo importa el cociente R/H.
constexpr float kRsobreH = 6371.0f / 8.0f;

} // namespace

// =============================================================================
// El color del sol (decisión 27)
// =============================================================================

float masa_de_aire(float elevacion_grados)
{
    // Un rayo sale del suelo, a distancia R del centro de la Tierra, hacia el
    // sol (dirección d unitaria, con elevación e), y se busca dónde cruza el
    // borde de la capa de aire (radio R + H). Es una intersección rayo-esfera:
    //   |p + t·d|² = (R + H)²,  con |p| = R y p·d = R·sen e
    //   t² + 2·t·R·sen e - (2·R·H + H²) = 0
    //   t = -R·sen e + √(R²·sen²e + 2·R·H + H²)      (la raíz positiva)
    // Dividido por H (el camino con el sol en el cenit) y con k = R/H:
    //   masa = √(k²·sen²e + 2k + 1) - k·sen e
    // En el cenit (sen e = 1) da √((k + 1)²) - k = 1; en el horizonte,
    // √(2k + 1) ≈ 40. Con una Tierra PLANA sería 1/sen e, que en el horizonte
    // se va a infinito: la curvatura es lo que la deja en ~40.
    const float k = kRsobreH;
    const float s = std::sin(glm::radians(elevacion_grados));
    return std::sqrt(k * k * s * s + 2.0f * k + 1.0f) - k * s;
}

glm::vec3 transmitancia_aire(float elevacion_grados)
{
    // Llega e^(-τ·masa). Dividido por lo que llega al mediodía, e^(-τ), queda
    // e^(-τ·(masa - 1)): 1 en el cenit. Así el sol de mediodía es justo el de
    // la decisión 18 (0,9) y el aire solo lo cambia cuando el sol baja.
    // Ejemplos con 0,9 al mediodía (R, G, B):
    //   60° -> (0,89 0,89 0,87)    29,8° (pintado) -> (0,86 0,82 0,72)
    //   10° -> (0,74 0,57 0,32)     5° -> (0,61 0,35 0,11)
    //    2° -> (0,40 0,13 0,01)     0° -> (0,19 0,02 0,00)
    // glm::exp de un vec3 calcula la exponencial de cada canal por separado.
    const float m = masa_de_aire(elevacion_grados);
    return glm::exp(-kTauRayleigh * (m - 1.0f));
}

float encendido_sol(float elevacion_grados)
{
    // El disco del sol (el del halo) mide 2°, radio 1°: con el centro a +1°
    // toca el horizonte con el borde de abajo; con el centro a -1°, se hundió
    // entero. En ese tramo la luz baja con smoothstep (la misma curva en S de
    // la niebla del cielo, Parte 4), sin quiebres. Sin esto se apagaría de
    // golpe al cruzar 0°: un salto de luz en toda la escena.
    return glm::smoothstep(-1.0f, 1.0f, elevacion_grados);
}

std::string hora_texto(float hora)
{
    // Minutos enteros desde la medianoche. Con floor y no redondeo: 16:00,8 se
    // muestra "16:00" y 23:59,9 nunca llega a mostrarse "24:00".
    const int minutos = static_cast<int>(std::floor(hora * 60.0f)) % (24 * 60);
    char texto[32];   // sobra lugar: "hh:mm" son 5 caracteres + el '\0'
    std::snprintf(texto, sizeof texto, "%02d:%02d", minutos / 60, minutos % 60);
    return texto;
}

// =============================================================================
// El recorrido y el reloj (decisiones 25 y 26)
// =============================================================================

DiaNoche::DiaNoche(const glm::vec3& dir_sol_pintado)
{
    // La parte HORIZONTAL de la dirección del pintado (su "sombra" sobre el
    // plano X-Y), normalizada: hacia ese lado se pone el sol. Si el pintado
    // estuviera justo en el cenit no tendría sombra; entonces sirve cualquier
    // lado (+X).
    glm::vec3 hacia_pintado(dir_sol_pintado.x, dir_sol_pintado.y, 0.0f);
    if (glm::length(hacia_pintado) < 1.0e-4f) {
        hacia_pintado = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    hacia_pintado = glm::normalize(hacia_pintado);

    // A las 6:00 el sol está en el horizonte, del lado OPUESTO al pintado.
    salida_ = -hacia_pintado;

    // El EJE de giro: horizontal y perpendicular al plano vertical en el que
    // está el pintado (el mismo plano que recorrían O/P en la Parte 2). Con
    // eje = salida × Z, la regla de la mano derecha lleva la salida hacia
    // arriba: eje × salida = Z (es la propiedad (a × b) × a = b para a y b
    // unitarios y perpendiculares). Así, girando 90°, el sol llega al cenit.
    eje_ = glm::cross(salida_, glm::vec3(0.0f, 0.0f, 1.0f));

    // A qué hora pasa por el pintado: de 0° a 90° el sol sube por el lado de
    // la salida; de 90° a 180° baja por el lado del pintado, y una elevación e
    // de ese lado se alcanza a 180° - e. Con el pintado a 29,8°: 150,2° ->
    // 6 h + 150,2° / 15°/h = 16:00 (y 48 s).
    const float e_pintado = glm::degrees(std::asin(glm::clamp(dir_sol_pintado.z, -1.0f, 1.0f)));
    angulo_pintado_ = 180.0f - e_pintado;
    angulo_         = angulo_pintado_;
}

void DiaNoche::avanzar(float dt)
{
    // Grados por segundo según la velocidad (360° = un día). Detenido: 0.
    float grados_por_segundo = 0.0f;
    if (reloj_ == Reloj::Normal) {
        grados_por_segundo = 360.0f / kSegundosPorDiaNormal;
    } else if (reloj_ == Reloj::Rapido) {
        grados_por_segundo = 360.0f / kSegundosPorDiaRapido;
    }
    mover(grados_por_segundo * dt);
}

DiaNoche::Reloj DiaNoche::cambiar_reloj()
{
    switch (reloj_) {
        case Reloj::Detenido: reloj_ = Reloj::Normal;   break;
        case Reloj::Normal:   reloj_ = Reloj::Rapido;   break;
        case Reloj::Rapido:   reloj_ = Reloj::Detenido; break;
    }
    return reloj_;
}

void DiaNoche::mover(float grados)
{
    // fmod deja el resto con el signo del dividendo (fmod(-5, 360) = -5): si
    // quedó negativo se le suma una vuelta. El segundo if cubre un caso de
    // redondeo: -0,00001 + 360 en float da exactamente 360.
    angulo_ = std::fmod(angulo_ + grados, 360.0f);
    if (angulo_ < 0.0f) {
        angulo_ += 360.0f;
    }
    if (angulo_ >= 360.0f) {
        angulo_ -= 360.0f;
    }
}

void DiaNoche::volver_al_pintado()
{
    angulo_ = angulo_pintado_;
}

float DiaNoche::hora() const
{
    // 0° son las 6:00 y van 15° por hora; fmod da la vuelta a las 24 h (270°
    // son 6 + 18 = 24 -> las 0:00).
    return std::fmod(6.0f + angulo_ / 15.0f, 24.0f);
}

float DiaNoche::hora_pintado() const
{
    return std::fmod(6.0f + angulo_pintado_ / 15.0f, 24.0f);
}

glm::mat3 DiaNoche::rotacion() const
{
    // La guía: el sol GIRA alrededor de un eje horizontal. glm::rotate arma la
    // matriz de ese giro (fórmula de Rodrigues): angulo_ grados alrededor de
    // eje_, con la regla de la mano derecha. Devuelve una mat4 (sirve para
    // puntos); para una DIRECCIÓN alcanza la parte 3x3, sin traslación.
    return glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(angulo_), eje_));
}

glm::vec3 DiaNoche::direccion_sol() const
{
    // La dirección de la salida, girada lo que avanzó la hora. Queda unitaria
    // (un giro no cambia largos) y siempre perpendicular al eje: el sol recorre
    // un círculo máximo que pasa por la salida, el cenit y el sol pintado.
    // Equivale a -cos(a)·hacia_pintado + sen(a)·Z, con a = angulo_.
    return rotacion() * salida_;
}

float DiaNoche::elevacion_sol() const
{
    // Con Z arriba, la componente z de una dirección unitaria es el SENO de su
    // elevación (como en Skybox::elevacion_sol_grados).
    return glm::degrees(std::asin(glm::clamp(direccion_sol().z, -1.0f, 1.0f)));
}

glm::vec3 DiaNoche::color_sol(const glm::vec3& color_mediodia) const
{
    // El de mediodía, por lo que deja pasar el aire (por canal) y por cuánto
    // del disco queda sobre el horizonte. De noche, (0, 0, 0): el sol no
    // ilumina (ni la difusa ni la especular), y el halo y la bruma, que son
    // proporcionales a uSolColor, también desaparecen.
    const float e = elevacion_sol();
    return color_mediodia * transmitancia_aire(e) * encendido_sol(e);
}
