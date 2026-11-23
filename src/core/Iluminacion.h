// -----------------------------------------------------------------------------
// Iluminacion.h — parámetros de la LUZ y de la NIEBLA de la escena (Práctico 09)
//
// Es la cara CPU de assets/shaders/iluminacion.glsl: los mismos datos, del lado
// de C++. La guía (Parte 1) pide que estos parámetros:
//   - se definan en UN ÚNICO LUGAR del lado de la CPU: una sola instancia de
//     Iluminacion, que vive en main;
//   - se envíen desde ahí a TODOS los programas que los usan: terreno,
//     aeronave y fondo. Si alguno usara otros valores, la diferencia se vería
//     como una costura (por ejemplo, entre la malla y el suelo continuado).
//
// Decisiones (Práctico 09, pregunta 3):
//   1. UN STRUCT PROPIO, de solo datos (como FlightData o CameraData). No
//      pertenece a ningún módulo de dibujo: el terreno, el avión y el fondo lo
//      LEEN, ninguno es su dueño.
//   2. SE ENVÍA EN CADA CUADRO. La posición del ojo (uOjo) cambia en todos los
//      cuadros de todos modos; el sol y la niebla cambian con teclas. Son unos
//      pocos uniforms por programa: no vale la pena llevar la cuenta de "qué
//      cambió".
//   3. LAS UBICACIONES SE BUSCAN UNA VEZ por programa (UniformsIluminacion),
//      igual que UniformsEscena/UniformsPieza en main. Con glProgramUniform*
//      cada programa tiene SUS PROPIOS uniforms aunque se llamen igual, así
//      que hay un UniformsIluminacion por programa.
//   4. BÚSQUEDA SIN AVISO: un programa puede no usar alguno de estos uniforms
//      (el compilador de GLSL elimina los que no se usan y su ubicación es -1).
//      Por ejemplo, en la Parte 1 nadie llama todavía a iluminar() y los del
//      sol no existen en ningún programa. glProgramUniform* con -1 no hace nada.
//      Para no perder de vista un nombre mal escrito, al buscarlos se informa
//      por consola cuántos usa cada programa y cuáles no.
//
// Coordenadas: "MUNDO" en este proyecto es el sistema con ORIGEN FLOTANTE (el
// avión en el origen, ver main.cpp). La dirección del sol no cambia con eso
// (una dirección no depende del origen); la posición del ojo (uOjo) tiene que
// estar en ESE mismo sistema: el ojo relativo al avión (ojo_rel en main).
// -----------------------------------------------------------------------------

#pragma once

#include <string>          // std::string: el nombre del programa en el informe de consola

#include <glm/glm.hpp>     // glm::vec3: direcciones y colores

#include "core/Shader.h"   // Shader: el programa al que se le buscan y envían los uniforms

// Los parámetros de la escena. Los valores por omisión son los de la guía; main
// los ajusta (el sol pintado y el horizonte del cubemap los calcula el Skybox).
struct Iluminacion {
    // --- Sol (luz direccional, Parte 2) ---
    glm::vec3 sol_dir {0.0f, 0.0f, 1.0f};   // dirección HACIA el sol (de la superficie a la luz), mundo, unitaria
    glm::vec3 sol_color {0.7f};             // color e intensidad del sol (lineal: proporción de la luz)
    glm::vec3 ambiente {0.3f};              // luz ambiente (0,3 + 0,7 = 1: sin zonas quemadas)

    // --- Niebla ---
    // Parte 1: sigue siendo la niebla LINEAL del Práctico 08, ahora en el código
    // común. En la Parte 4 pasa a la exponencial con una visibilidad V.
    glm::vec3 niebla_color {0.5f};          // color del horizonte del cielo (lo calcula el Skybox)
    float     niebla_inicio {300.0f};       // distancia donde EMPIEZA (u de escena; 300 u = 3 km)
    float     niebla_fin {1800.0f};         // distancia donde es TOTAL; <= 0 -> sin niebla (tecla N)
};

// Ubicaciones de los uniforms de iluminacion.glsl en UN programa. -1 = el
// programa no lo usa (o todavía no se buscó).
struct UniformsIluminacion {
    int sol_dir = -1, sol_color = -1, ambiente = -1, ojo = -1;
    int niebla_color = -1, niebla_inicio = -1, niebla_fin = -1;
};

// Busca (una vez) las ubicaciones en el programa e informa por consola cuántas
// usa: "[luz] terreno: usa 4 de 7 (no usa: uSolDir, ...)". `nombre` es solo
// para ese informe.
UniformsIluminacion buscar_uniforms_iluminacion(const Shader& programa,
                                                const std::string& nombre);

// Envía los parámetros de la escena y la posición del ojo (mundo, origen
// flotante) al programa. Se llama en cada cuadro, una vez por programa.
void enviar_iluminacion(const Shader& programa, const UniformsIluminacion& u,
                        const Iluminacion& luz, const glm::vec3& ojo);
