// -----------------------------------------------------------------------------
// main_test_dia_noche.cpp — prueba del CICLO DÍA/NOCHE (Práctico 09, opcional 2)
//
// Verifica por cálculo, sin dibujar nada (DiaNoche no usa OpenGL), el paso 1:
//   1. El RECORRIDO (decisión 26): con un sol pintado de prueba (el del
//      cubemap del profe: 29,8° de elevación), el sol sale a las 6:00 en el
//      horizonte del lado opuesto, pasa por el cenit a las 12:00, por el
//      pintado a las 16:00 y se pone a las 18:00 de su lado. El eje es
//      horizontal y la dirección, siempre unitaria y perpendicular al eje.
//   2. El RELOJ (decisión 25): detenido no avanza; normal = 3°/s (un día en
//      2 min); rápido = 12°/s (un día en 30 s); D los recorre en orden; O/P dan
//      la vuelta sin salirse de [0°, 360°); H vuelve al pintado.
//   3. El COLOR (decisión 27): la masa de aire vale 1 en el cenit y ~40 en el
//      horizonte; al mediodía el color es el de la decisión 18 (0,9); cuando el
//      sol baja, el azul cae más que el verde y el verde más que el rojo; y
//      debajo del horizonte el sol se apaga (suave entre +1° y -1°).
//   4. La hora como texto: "16:00", "00:00", "23:59".
//
// Compilar y correr (desde la raíz del proyecto; es UNA sola línea):
//   g++ -std=c++17 -Wall -Wextra -I./src src/core/DiaNoche.cpp tests/main_test_dia_noche.cpp -o build/test_dia_noche && ./build/test_dia_noche
// -----------------------------------------------------------------------------

#include <cmath>       // std::fabs: diferencias con tolerancia
#include <cstdlib>     // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>    // std::cout

#include <glm/glm.hpp> // glm::vec3, length, dot, normalize, radians

#include "core/DiaNoche.h"   // DiaNoche, masa_de_aire, transmitancia_aire, encendido_sol, hora_texto

static int fallas = 0;

static void verificar(bool ok, const char* texto)
{
    std::cout << (ok ? "  [OK]   " : "  [FALLA] ") << texto << std::endl;
    if (!ok) {
        ++fallas;
    }
}

static bool cerca(float a, float b, float tol)
{
    return std::fabs(a - b) <= tol;
}

static bool cerca(const glm::vec3& a, const glm::vec3& b, float tol)
{
    return glm::length(a - b) <= tol;
}

int main()
{
    // El sol pintado del cubemap del profe (lo informa el programa al
    // arrancar): (-0,7567, -0,4247, 0,4970), elevación 29,8°.
    const glm::vec3 pintado = glm::normalize(glm::vec3(-0.756714f, -0.424654f, 0.497045f));
    const glm::vec3 hacia_pintado = glm::normalize(glm::vec3(pintado.x, pintado.y, 0.0f));
    const glm::vec3 Z(0.0f, 0.0f, 1.0f);

    // -------------------------------------------------------------------------
    std::cout << "1. Recorrido (decision 26)" << std::endl;
    DiaNoche dia(pintado);
    verificar(cerca(dia.direccion_sol(), pintado, 1.0e-4f),
              "arranca en el sol pintado (la luz viene de donde se lo ve)");
    verificar(cerca(dia.elevacion_sol(), 29.8f, 0.05f), "elevacion al arrancar: 29,8 grados");
    verificar(hora_texto(dia.hora()) == "16:00", "hora al arrancar: 16:00");
    verificar(dia.reloj() == DiaNoche::Reloj::Detenido, "el reloj arranca detenido");
    verificar(cerca(dia.eje().z, 0.0f, 1.0e-6f) && cerca(glm::length(dia.eje()), 1.0f, 1.0e-5f),
              "el eje es horizontal y unitario");

    dia.mover(-dia.angulo());   // angulo 0 = la salida
    verificar(cerca(dia.direccion_sol(), -hacia_pintado, 1.0e-4f),
              "a las 6:00 sale en el horizonte, del lado opuesto al pintado");
    verificar(hora_texto(dia.hora()) == "06:00", "angulo 0 = 06:00");
    dia.mover(90.0f);
    verificar(cerca(dia.direccion_sol(), Z, 1.0e-4f), "a las 12:00 pasa por el cenit");
    verificar(hora_texto(dia.hora()) == "12:00", "angulo 90 = 12:00");
    dia.mover(90.0f);
    verificar(cerca(dia.direccion_sol(), hacia_pintado, 1.0e-4f),
              "a las 18:00 se pone en el horizonte, del lado del pintado");
    dia.mover(90.0f);
    verificar(cerca(dia.direccion_sol(), -Z, 1.0e-4f), "a las 0:00 esta debajo, en el nadir");
    verificar(hora_texto(dia.hora()) == "00:00", "angulo 270 = 00:00");

    bool unitaria = true, perpendicular = true;
    for (int i = 0; i < 72; ++i) {   // una vuelta entera, de a 5 grados
        dia.mover(5.0f);
        unitaria      = unitaria && cerca(glm::length(dia.direccion_sol()), 1.0f, 1.0e-5f);
        perpendicular = perpendicular && cerca(glm::dot(dia.direccion_sol(), dia.eje()), 0.0f, 1.0e-5f);
    }
    verificar(unitaria, "la direccion es unitaria a cualquier hora");
    verificar(perpendicular, "la direccion es perpendicular al eje (gira alrededor de el)");

    // -------------------------------------------------------------------------
    std::cout << "2. Reloj (decision 25)" << std::endl;
    DiaNoche reloj(pintado);
    const float a0 = reloj.angulo();
    reloj.avanzar(10.0f);
    verificar(cerca(reloj.angulo(), a0, 1.0e-5f), "detenido: 10 s no mueven el sol");
    verificar(reloj.cambiar_reloj() == DiaNoche::Reloj::Normal, "D: detenido -> normal");
    reloj.avanzar(10.0f);
    verificar(cerca(reloj.angulo(), a0 + 30.0f, 1.0e-3f), "normal: 10 s = 30 grados (un dia en 2 min)");
    verificar(reloj.cambiar_reloj() == DiaNoche::Reloj::Rapido, "D: normal -> rapido");
    reloj.avanzar(1.0f);
    verificar(cerca(reloj.angulo(), a0 + 42.0f, 1.0e-3f), "rapido: 1 s = 12 grados (un dia en 30 s)");
    verificar(reloj.cambiar_reloj() == DiaNoche::Reloj::Detenido, "D: rapido -> detenido");
    reloj.mover(-400.0f);
    verificar(reloj.angulo() >= 0.0f && reloj.angulo() < 360.0f, "O: atrasar 400 grados da la vuelta");
    reloj.mover(1000.0f);
    verificar(reloj.angulo() >= 0.0f && reloj.angulo() < 360.0f, "P: adelantar 1000 grados da la vuelta");
    reloj.volver_al_pintado();
    verificar(cerca(reloj.direccion_sol(), pintado, 1.0e-4f), "H: vuelve al sol pintado");

    // -------------------------------------------------------------------------
    std::cout << "3. Color (decision 27)" << std::endl;
    verificar(cerca(masa_de_aire(90.0f), 1.0f, 1.0e-4f), "masa de aire en el cenit: 1");
    verificar(cerca(masa_de_aire(0.0f), 39.9f, 0.1f), "masa de aire en el horizonte: ~40 (no infinito)");
    verificar(masa_de_aire(60.0f) < masa_de_aire(30.0f) && masa_de_aire(30.0f) < masa_de_aire(10.0f)
                  && masa_de_aire(10.0f) < masa_de_aire(0.0f),
              "cuanto mas bajo el sol, mas aire");
    // Con una Tierra plana sería 1/sen(e): a 30° da 2. Lejos del horizonte la
    // curvatura casi no cambia nada.
    verificar(cerca(masa_de_aire(30.0f), 2.0f, 0.01f), "a 30 grados, casi 1/sen(e) = 2 (Tierra plana)");

    const glm::vec3 mediodia(0.9f);
    DiaNoche color(pintado);
    color.mover(90.0f - color.angulo());   // las 12:00
    verificar(cerca(color.color_sol(mediodia), mediodia, 1.0e-4f), "al mediodia, el color de la decision 18 (0,9)");
    const glm::vec3 t10 = transmitancia_aire(10.0f);
    verificar(cerca(t10, glm::vec3(0.826f, 0.637f, 0.358f), 0.01f), "a 10 grados llega (0,83 0,64 0,36)");
    verificar(t10.b < t10.g && t10.g < t10.r, "el azul cae mas que el verde y el verde mas que el rojo");
    const glm::vec3 t2 = transmitancia_aire(2.0f);
    verificar(t2.g / t2.r < t10.g / t10.r && t2.b / t2.r < t10.b / t10.r,
              "mas bajo, mas rojo (2 grados contra 10)");
    verificar(cerca(encendido_sol(1.0f), 1.0f, 1.0e-6f) && cerca(encendido_sol(-1.0f), 0.0f, 1.0e-6f)
                  && cerca(encendido_sol(0.0f), 0.5f, 1.0e-6f),
              "encendido: 1 a +1 grado, 0,5 en el horizonte, 0 a -1 grado");
    color.mover(95.0f);   // 12:00 + 95° = 18:20, sol a -5°
    verificar(cerca(color.elevacion_sol(), -5.0f, 0.01f) && color.color_sol(mediodia) == glm::vec3(0.0f),
              "a las 18:20 (sol a -5 grados) esta apagado");

    // -------------------------------------------------------------------------
    std::cout << "4. La hora como texto" << std::endl;
    verificar(hora_texto(16.0130f) == "16:00", "16,013 h -> 16:00 (no redondea para arriba)");
    verificar(hora_texto(23.9999f) == "23:59", "23,9999 h -> 23:59 (nunca 24:00)");
    verificar(hora_texto(0.5f) == "00:30", "0,5 h -> 00:30");

    std::cout << (fallas == 0 ? "TODO OK" : "HAY FALLAS") << " (" << fallas << " fallas)" << std::endl;
    return fallas == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
