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
//      Por ejemplo, en la Parte 1 nadie llamaba todavía a iluminar() y los del
//      sol no existían en ningún programa; desde la Parte 2 el suelo (ks = 0)
//      no usa uOjo ni uBlinnPhong, porque el compilador elimina la especular
//      que multiplica a cero. glProgramUniform* con -1 no hace nada.
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
    // Luz DIRECCIONAL: el sol está tan lejos que sus rayos llegan PARALELOS a
    // toda la escena. Por eso es una sola dirección para todos los puntos (la
    // w = 0 de las filminas) y no una posición: no se calcula L por fragmento.
    glm::vec3 sol_dir {0.0f, 0.0f, 1.0f};   // dirección HACIA el sol (de la superficie a la luz), mundo, unitaria
    glm::vec3 sol_color {0.7f};             // color e intensidad del sol (lineal: proporción de la luz)
    glm::vec3 ambiente {0.3f};              // luz ambiente (0,3 + 0,7 = 1: sin zonas quemadas)

    // --- Niebla ---
    // Parte 4: EXPONENCIAL, f = 1 - e^(-d/V) (aplicar_niebla, código común). Un
    // solo parámetro, la VISIBILIDAD V. (Hasta la Parte 3 era la lineal del
    // Práctico 08, con un inicio y un fin.) main elige V (kVisibilidad) y
    // la pone en 0 con la tecla N.
    glm::vec3 niebla_color {0.5f};          // color del horizonte del cielo (lo calcula el Skybox; J: gris)
    float     visibilidad {600.0f};         // V, u de escena (600 u = 6 km): a V la niebla es del 63 %;
                                            // <= 0 -> sin niebla (tecla N)

    // --- Verificación de la luz del suelo (Parte 2, tecla B) ---
    // 0 = con luz y las normales del terreno; 1 = sin luz (color = kd, como el
    // P08); 2 = con luz y la normal constante (0, 0, 1). Va acá y no en un
    // uniform propio del terreno porque lo usan DOS programas (la malla del
    // terreno y el suelo continuado del fondo) y los dos tienen que cambiar
    // juntos; así viaja por el mismo camino que el sol y la niebla.
    int       modo_suelo {0};

    // --- Modelo del brillo especular (Parte 3, tecla R) ---
    // 0 = Phong (vector reflejado R), 1 = Blinn-Phong (vector medio H, con el
    // exponente x 4). Es de la ESCENA y no del avión: el modelo es UNO para
    // toda la escena (en el suelo no se nota porque su ks = 0).
    int       blinn_phong {0};
};

// Ubicaciones de los uniforms de iluminacion.glsl en UN programa. -1 = el
// programa no lo usa (o todavía no se buscó).
struct UniformsIluminacion {
    int sol_dir = -1, sol_color = -1, ambiente = -1, ojo = -1;
    int niebla_color = -1, visibilidad = -1;   // P09, Parte 4: uVisibilidad reemplaza a inicio/fin
    int modo_suelo = -1;   // P09, Parte 2 (tecla B)
    int blinn_phong = -1;  // P09, Parte 3 (tecla R)
};

// Busca (una vez) las ubicaciones en el programa e informa por consola cuántas
// usa: "[luz] fondo: usa 6 de 8 (no usa: uOjo, ...)". `nombre` es solo para
// ese informe.
UniformsIluminacion buscar_uniforms_iluminacion(const Shader& programa,
                                                const std::string& nombre);

// Envía los parámetros de la escena y la posición del ojo (mundo, origen
// flotante) al programa. Se llama en cada cuadro, una vez por programa.
void enviar_iluminacion(const Shader& programa, const UniformsIluminacion& u,
                        const Iluminacion& luz, const glm::vec3& ojo);

// Práctico 09 (Parte 2), teclas O/P: la dirección HACIA el sol con la MISMA
// orientación horizontal (acimut) que `base` pero a otra ELEVACIÓN (grados
// sobre el horizonte). Escena Z-ARRIBA: la elevación es el ángulo con el plano
// X-Y y el acimut, el ángulo en ese plano. Se usa para subir y bajar el sol
// sin que cambie el lado del que viene (el del sol pintado del cubemap).
glm::vec3 sol_con_elevacion(const glm::vec3& base, float elevacion_grados);
