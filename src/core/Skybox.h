// -----------------------------------------------------------------------------
// Skybox.h — el FONDO de la escena: cielo + suelo continuado (Práctico 08)
//
// Es la parte "skybox" de Scene en Arquitectura-Proyecto.md. Dibuja un CUBO que
// rodea al observador y que queda DETRÁS DE TODO; sobre sus caras se pinta:
//   - el cielo: un CUBEMAP (6 imágenes, Parte 4) o un DEGRADÉ 1D por elevación
//     (opcional de la guía, tecla K);
//   - por debajo del horizonte, el SUELO CONTINUADO (Parte 5): un plano infinito
//     con la textura del terreno, que tapa la "banda" que deja el terreno finito
//     cuando el avión sube.
//
// Decisiones (ver Practico08-Decisiones.md):
//   1. EL MÓDULO ES DUEÑO de todo lo suyo: el cubo (Mesh), el cubemap y el
//      degradé (Texture) y su programa de shaders (Shader). Lo único que NO es
//      suyo es la textura del suelo: es la del terreno (la posee main) y llega
//      como puntero en ParametrosFondo, así el fondo usa SIEMPRE la misma que la
//      malla (también con la imagen de prueba o la trampa de los mipmaps).
//   2. VISTA SIN TRASLACIÓN: mat4(mat3(vista)). El cubo queda centrado en el ojo
//      y nunca se lo alcanza (sin paralaje = "infinitamente lejos").
//   3. PROFUNDIDAD, MÉTODO (b): se dibuja AL ÚLTIMO, con z = w en el vertex
//      shader (profundidad 1 = plano lejano) y glDepthFunc(GL_LEQUAL). Solo se
//      pinta donde la escena no dibujó nada y no hay problema con el plano near.
//   4. RECORTE DE CARAS: el cubo se mira desde ADENTRO, sus triángulos se ven
//      con el giro invertido y el recorte de caras traseras los descartaría
//      todos. Se desactiva durante el dibujo del fondo y se vuelve a activar.
//   5. ESCENA Z-ARRIBA vs cubemap Y-arriba: la dirección se reacomoda en
//      fondo.fs, (x, y, z) -> (x, z, -y) (una rotación, no un espejado).
//
// Al cargar el cubemap también calcula (en la CPU, ANTES de que main libere las
// imágenes con unload_image):
//   - el COLOR DEL HORIZONTE: promedio de la fila central de las 4 caras
//     laterales (es el color de la niebla; lo pide así el Práctico 09, Parte 4);
//   - el COLOR DEL CENIT del cubemap: promedio del centro de la cara de
//     arriba (solo informativo: en este cubemap el cenit está NUBLADO y da un
//     gris; el degradé usa un azul de cielo despejado, ver kCenitDegrade);
//   - la DIRECCIÓN DEL SOL pintado en el cubemap (los píxeles más brillantes por
//     encima del horizonte): el Práctico 09 pide que la luz coincida con él.
// -----------------------------------------------------------------------------

#pragma once

#include <array>           // std::array: las 6 caras del cubemap que recibe init()

#include <glm/glm.hpp>     // glm::mat4, vec3, vec2: vista, proyección, colores y dirección del sol

#include "core/Mesh.h"     // Mesh: el cubo del cielo (es dueño)
#include "core/ResourceManager.h"   // ImageData, ShaderSource
#include "core/Shader.h"   // Shader: el programa "fondo" (es dueño)
#include "core/Texture.h"  // Texture: el cubemap y el degradé (es dueño)

// Cómo se pinta el cielo (tecla K).
enum class ModoCielo { Cubemap, Degrade };

// Todo lo que el fondo necesita en cada cuadro (lo arma main).
struct ParametrosFondo {
    glm::mat4 vista {1.0f};            // vista COMPLETA de la cámara (acá se le quita la traslación)
    glm::mat4 proyeccion {1.0f};
    glm::vec2 ojo_uv {0.0f};           // (x, y) del ojo en el mundo, módulo Terrain::kPeriodoUV
    float     altura_ojo {0.0f};       // altura del ojo SOBRE el plano del suelo
    float     tam_tesela {5.0f};       // el mismo del terreno (unidades de escena)
    const Texture* suelo {nullptr};    // textura del terreno (no propia)
    bool      suelo_continuado {true}; // tecla G
    glm::vec3 niebla_color {0.5f};
    float     niebla_inicio {0.0f};
    float     niebla_fin {0.0f};       // <= 0: sin niebla
};

class Skybox {
public:
    Skybox() = default;
    Skybox(const Skybox&) = delete;              // posee Mesh/Texture/Shader (dueños únicos)
    Skybox& operator=(const Skybox&) = delete;

    // Compila el programa del fondo, arma el cubo, sube el cubemap (caras en el
    // orden +X -X +Y -Y +Z -Z, SIN invertir) y genera el degradé. Calcula el
    // color del horizonte, el del cenit y la dirección del sol. Lanza
    // std::runtime_error si el shader no compila o las caras no son válidas.
    void init(const ShaderSource& fuente, const std::array<const ImageData*, 6>& caras);

    // Dibuja el fondo. Llamarlo DESPUÉS de la escena (método b). Deja el estado
    // de OpenGL como lo encontró salvo el modo de polígonos (queda en relleno:
    // main lo vuelve a fijar al empezar cada cuadro).
    void draw(const ParametrosFondo& p) const;

    void      set_modo(ModoCielo modo) { modo_ = modo; }
    ModoCielo modo() const { return modo_; }

    // Color del horizonte del cielo ACTIVO (sRGB en [0, 1]): es el color de la
    // niebla, para que la lejanía se funda con el cielo que se está mostrando.
    glm::vec3 color_horizonte() const;
    // Color medio del cenit del cubemap (informativo, sRGB en [0, 1]).
    glm::vec3 color_cenit_cubemap() const { return cenit_cubo_; }
    // Dirección HACIA el sol pintado en el cubemap (mundo, Z arriba, unitaria) y
    // su elevación en grados. Para el Práctico 09 (uSolDir).
    glm::vec3 direccion_sol() const { return dir_sol_; }
    float     elevacion_sol_grados() const;

private:
    Mesh      cubo_;
    Texture   cubemap_;
    Texture   degrade_;
    Shader    shader_;
    ModoCielo modo_ {ModoCielo::Cubemap};

    glm::vec3 horizonte_cubo_ {0.5f};   // promedio de la fila central de las 4 caras laterales
    glm::vec3 cenit_cubo_ {0.3f};       // promedio del centro de la cara de arriba
    glm::vec3 dir_sol_ {0.0f, 0.0f, 1.0f};

    // Ubicaciones de los uniforms (se buscan una vez, en init).
    // Por qué empiezan en -1: -1 es el valor que OpenGL usa para "este uniform
    // NO EXISTE" (lo devuelve glGetUniformLocation si el nombre está mal o si
    // el compilador de GLSL lo eliminó por no usarse). Y glProgramUniform* con
    // ubicación -1 se IGNORA en silencio, sin error. Entonces, si por algún
    // motivo se dibujara antes de init(), los set_uniform no hacen nada en vez
    // de escribir en un uniform equivocado. No sirve 0 como valor inicial:
    // 0 es una ubicación VÁLIDA (la del primer uniform del programa).
    int loc_proyeccion_ {-1}, loc_vista_ {-1};
    int loc_modo_ {-1}, loc_suelo_continuado_ {-1};
    int loc_ojo_uv_ {-1}, loc_altura_ojo_ {-1}, loc_tam_tesela_ {-1};
    int loc_niebla_color_ {-1}, loc_niebla_inicio_ {-1}, loc_niebla_fin_ {-1};
};
