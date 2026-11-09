// -----------------------------------------------------------------------------
// Skybox.cpp — el FONDO de la escena: cielo + suelo continuado (Práctico 08)
// -----------------------------------------------------------------------------

#include "core/Skybox.h"      // la declaración de la clase que se implementa acá

#include <algorithm>          // std::max: el brillo máximo del cielo (para buscar el sol)
#include <cmath>              // std::sqrt y std::asin: dirección y elevación del sol pintado
#include <stdexcept>          // std::runtime_error: si falla el shader del fondo
#include <utility>            // std::pair: cada candidato a sol (dirección, brillo)
#include <vector>             // std::vector: la lista de candidatos y los texels del degradé

#include <glad/gl.h>          // funciones de OpenGL (glDepthFunc, glDisable(GL_CULL_FACE), ...)

#include "core/Primitives.h"  // primitives::cube: el cubo donde se pinta el cielo

namespace {

// Promedio RGB (en [0, 1]) de un rectángulo de píxeles de una imagen RGBA.
// El rectángulo va de la columna x0 a x1 y de la fila y0 a y1 (x1 e y1 NO
// incluidos). Es el COLOR MEDIO de esa zona: se suman los R, G y B de todos
// sus píxeles por separado y se divide por la cantidad de píxeles.
// Se usa dos veces en init():
//   - color del HORIZONTE: las 3 filas del medio de las 4 caras laterales
//     (es el color de la niebla);
//   - color del CENIT: el cuadrado central de la cara de arriba (informativo).
glm::vec3 promedio(const ImageData& img, int x0, int y0, int x1, int y1)
{
    // Acumuladores en double (dvec3 = vec3 de double): con cientos de miles de
    // píxeles la suma es grande y un float perdería decimales.
    glm::dvec3 suma(0.0);
    long cuenta = 0;
    for (int y = y0; y < y1; ++y) {          // cada fila del rectángulo
        for (int x = x0; x < x1; ++x) {      // cada columna de esa fila
            // p = PUNTERO al primer byte del píxel (x, y) dentro de `pixels`.
            // La imagen está guardada fila por fila, 4 bytes por píxel (RGBA,
            // ver ImageData en ResourceManager.h), así que el píxel (x, y)
            // empieza en el byte (y * ancho + x) * 4:
            //   y * ancho -> cuántos píxeles hay en las filas de antes
            //   + x       -> cuántos más hasta llegar a esta columna
            //   * 4       -> cada píxel ocupa 4 bytes
            // Desde ahí: p[0] = R, p[1] = G, p[2] = B, p[3] = alfa (no se usa).
            // (static_cast<std::size_t>: la cuenta se hace en un entero grande
            // sin signo, el tipo de los índices de un vector.)
            const unsigned char* p =
                &img.pixels[(static_cast<std::size_t>(y) * img.width + x) * 4U];
            // Suma el color de este píxel (cada canal de 0 a 255).
            suma += glm::dvec3(p[0], p[1], p[2]);
            ++cuenta;
        }
    }
    // Promedio = suma / cantidad de píxeles, y / 255 para pasar de 0..255 a
    // 0..1 (la escala de los colores en los shaders). Ejemplo: si todos los
    // píxeles fueran (196, 209, 229), da (0.769, 0.820, 0.898).
    return glm::vec3(suma / (static_cast<double>(cuenta) * 255.0));
}

// Dirección (convención del CUBEMAP de OpenGL, Y arriba) que corresponde al
// píxel (col, fila) de la cara `cara` (0..5 = +X -X +Y -Y +Z -Z). Es la tabla
// de la especificación de OpenGL "al revés": sc y tc son las coordenadas del
// píxel llevadas a [-1, 1]; la fila 0 es la de ARRIBA de la imagen (las caras
// no se invierten al cargarlas) y corresponde a t = 0.
glm::vec3 direccion_cubemap(int cara, int col, int fila, int lado)
{
    // Del número de píxel a una coordenada en [-1, 1], en tres pasos:
    //   col + 0.5      -> el CENTRO del píxel (no su borde izquierdo)
    //   / lado         -> en [0, 1]  (0 = un borde de la cara, 1 = el otro)
    //   * 2 - 1        -> en [-1, 1] (0 = el centro de la cara)
    // Ejemplo con lado = 900: col 0 -> -0.999, col 449 -> -0.001, col 899 -> 0.999.
    // [-1, 1] porque la cara es un lado del cubo de "radio" 1: el switch arma
    // la dirección poniendo ±1 en el eje de la cara y sc, tc en los otros dos.
    const float sc = 2.0f * (static_cast<float>(col)  + 0.5f) / static_cast<float>(lado) - 1.0f;
    const float tc = 2.0f * (static_cast<float>(fila) + 0.5f) / static_cast<float>(lado) - 1.0f;
    switch (cara) {
        case 0:  return glm::vec3( 1.0f, -tc,  -sc);    // +X (right)
        case 1:  return glm::vec3(-1.0f, -tc,   sc);    // -X (left)
        case 2:  return glm::vec3(  sc,  1.0f,  tc);    // +Y (top)
        case 3:  return glm::vec3(  sc, -1.0f, -tc);    // -Y (bottom)
        case 4:  return glm::vec3(  sc,  -tc,  1.0f);   // +Z (front)
        default: return glm::vec3( -sc,  -tc, -1.0f);   // -Z (back)
    }
}

// Inversa de a_cubemap() de fondo.fs: del cubemap (Y arriba) a la escena (Z arriba).
//   escena -> cubemap: (x, y, z) -> (x, z, -y)
//   cubemap -> escena: (x, y, z) -> (x, -z, y)
glm::vec3 cubemap_a_escena(const glm::vec3& g)
{
    return glm::vec3(g.x, -g.z, g.y);
}

}  

void Skybox::init(const ShaderSource& fuente, const std::array<const ImageData*, 6>& caras)
{
    // --- Programa de shaders del fondo ---------------------------------------
    if (!shader_.compile_from_source(fuente.vs, fuente.fs)) {
        throw std::runtime_error("Skybox: el programa del fondo no compiló");
    }
    loc_proyeccion_       = shader_.loc("uProjection");
    loc_vista_            = shader_.loc("uView");
    loc_modo_             = shader_.loc("uModoCielo");
    loc_suelo_continuado_ = shader_.loc("uSueloContinuado");
    loc_ojo_uv_           = shader_.loc("uOjoUV");
    loc_altura_ojo_       = shader_.loc("uAlturaOjo");
    loc_tam_tesela_       = shader_.loc("uTamTesela");
    loc_niebla_color_     = shader_.loc("uNieblaColor");
    loc_niebla_inicio_    = shader_.loc("uNieblaInicio");
    loc_niebla_fin_       = shader_.loc("uNieblaFin");

    // --- Geometría: el cubo de las primitivas (Práctico 03) ------------------
    // Ya está centrado en su origen, que es lo que hace falta: la posición de
    // cada vértice ES la dirección para muestrear. El tamaño no importa (la
    // dirección no necesita ser unitaria y la profundidad se fuerza a 1).
    cubo_.load(primitives::cube());

    // --- Cubemap (Parte 4) ---------------------------------------------------
    cubemap_.load_cubemap(caras);
    // Filtrado "sin costuras" entre caras: sin esto, el filtrado lineal no mira
    // la cara vecina y en las aristas del cubo puede verse una línea.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // --- Colores de referencia (CPU, con las imágenes todavía cargadas) ------
    // Horizonte: fila central (elevación 0) de las 4 caras LATERALES
    // (+X, -X, +Z, -Z = índices 0, 1, 4, 5). Se promedian 3 filas por si la
    // central cae justo sobre una línea muy marcada.
    const int lado = caras[0]->width;
    const int medio = lado / 2;
    glm::vec3 suma_h(0.0f);
    for (int k : {0, 1, 4, 5}) {
        // Rectángulo: columnas 0..lado (TODO el ancho) y filas medio-1 ..
        // medio+1 (la de en medio y sus dos vecinas; promedio() no incluye la
        // última, medio+2). Con lado = 900: filas 449, 450 y 451 -> 2700
        // píxeles por cara. Se suman los 4 colores medios...
        suma_h += promedio(*caras[static_cast<std::size_t>(k)], 0, medio - 1, lado, medio + 2);
    }
    // ...y se dividen por 4: el promedio de las 4 caras (pesan igual porque
    // todas aportan la misma cantidad de píxeles).
    horizonte_cubo_ = suma_h / 4.0f;
    // Cenit: el cuarto central de la cara de ARRIBA (+Y, índice 2).
    // Rectángulo de lado/4 a 3*lado/4 en columnas y en filas: el cuadrado
    // central, de la mitad del lado (con lado = 900: de 225 a 674, 450 x 450
    // píxeles). Se deja afuera el borde, que ya mira hacia las caras laterales.
    cenit_cubo_ = promedio(*caras[2], lado / 4, lado / 4, 3 * lado / 4, 3 * lado / 4);

    // Sol pintado: el centro de los píxeles MÁS BRILLANTES por encima del
    // horizonte. (Se excluye la mitad de abajo: el cubemap tiene un reflejo del
    // sol, como si fuera agua.) Se recorre cada 2 píxeles: alcanza y es la mitad
    // de trabajo.
    //
    // CÓMO, en dos pasadas:
    //   1. Recorrer los píxeles de las 6 caras. Para cada uno: calcular hacia
    //      qué DIRECCIÓN de la escena mira (direccion_cubemap + el cambio a
    //      Z arriba); descartar los de abajo del horizonte; calcular su brillo
    //      (luminancia, mismos pesos que Texture). Guardar como "candidato" todo
    //      píxel que esté a menos de 2 (en la escala 0..255) del máximo visto
    //      HASTA ESE MOMENTO. Como el máximo va subiendo, algunos candidatos
    //      viejos quedan por debajo: por eso hace falta la segunda pasada.
    //   2. Quedarse solo con los candidatos a menos de 2 del máximo FINAL y
    //      SUMAR sus direcciones (vectores unitarios). Normalizar la suma da la
    //      dirección MEDIA de la mancha más brillante = el centro del sol.
    // Con este cubemap da (-0.757, -0.425, 0.497): elevación ~30°.
    float lum_max = 0.0f;
    std::vector<std::pair<glm::vec3, float>> candidatos;
    for (int cara = 0; cara < 6; ++cara) {
        const ImageData& img = *caras[static_cast<std::size_t>(cara)];
        for (int fila = 0; fila < lado; fila += 2) {
            for (int col = 0; col < lado; col += 2) {
                // Dirección del píxel, ya en la escena (Z arriba).
                const glm::vec3 dir = cubemap_a_escena(direccion_cubemap(cara, col, fila, lado));
                if (dir.z <= 0.0f) {
                    continue;                       // por debajo del horizonte
                }
                const unsigned char* p =
                    &img.pixels[(static_cast<std::size_t>(fila) * lado + col) * 4U];
                // Brillo del píxel (0..255), pesos de Rec. 709.
                const float lum = 0.2126f * p[0] + 0.7152f * p[1] + 0.0722f * p[2];
                if (lum >= lum_max - 2.0f) {        // posible "parte del sol"
                    lum_max = std::max(lum_max, lum);
                    candidatos.emplace_back(glm::normalize(dir), lum);
                }
            }
        }
    }
    // Pasada 2: sumar las direcciones de los que quedaron cerca del máximo.
    // (c.first = dirección, c.second = brillo.)
    glm::vec3 acumulado(0.0f);
    for (const auto& c : candidatos) {
        if (c.second >= lum_max - 2.0f) {            // solo los del máximo final
            acumulado += c.first;
        }
    }
    // Normalizar = quedarse con la dirección media (largo 1). Si no hubo
    // candidatos (imagen negra), dir_sol_ queda con su valor inicial.
    if (glm::length(acumulado) > 0.0f) {
        dir_sol_ = glm::normalize(acumulado);
    }

    // --- Degradé 1D (opcional de la guía) ------------------------------------
    // 64 texels RGBA del horizonte (s = 0) al cenit (s = 1), generados por
    // código (no hay archivo).
    //   - Horizonte: el MISMO color del horizonte del cubemap, así la niebla
    //     (que usa ese color) coincide en los dos modos de cielo.
    //   - Cenit: un azul de cielo DESPEJADO elegido a mano. El promedio de la
    //     cara de arriba del cubemap da un gris (ese cenit está cubierto de
    //     nubes) y el degradé quedaba "lavado"; un cielo sin nubes es más
    //     oscuro y saturado arriba.
    //   - La mezcla usa sqrt(s): en un cielo real casi todo el cambio de color
    //     ocurre cerca del horizonte (la curva sube rápido al principio).
    const glm::vec3 kCenitDegrade(0.22f, 0.42f, 0.78f);
    // Una "imagen" de 64 x 1 píxeles armada a mano (misma estructura que una
    // cargada de archivo: RGBA, 4 bytes por texel), para subirla con load_1d.
    ImageData degr;
    degr.width = 64;
    degr.height = 1;
    degr.channels = 4;
    degr.pixels.resize(64U * 4U);
    for (int i = 0; i < 64; ++i) {
        // s = posición del texel en [0, 1]: texel 0 -> 0 (horizonte),
        // texel 63 -> 1 (cenit). Se divide por 63 (no 64) para llegar a 1.
        const float s = static_cast<float>(i) / 63.0f;
        // mix(a, b, t) = a*(1-t) + b*t: mezcla lineal entre los dos colores.
        // Con t = sqrt(s) en vez de s, la mezcla avanza RÁPIDO al principio:
        // a mitad de camino (s = 0.5) ya va el 71 % hacia el azul del cenit.
        const glm::vec3 c = glm::mix(horizonte_cubo_, kCenitDegrade, std::sqrt(s));
        // De [0, 1] a un byte 0..255 (+0.5 = redondear al más cercano en vez
        // de truncar). Alfa 255 (opaco; no se usa).
        degr.pixels[static_cast<std::size_t>(i) * 4U + 0U] = static_cast<unsigned char>(c.r * 255.0f + 0.5f);
        degr.pixels[static_cast<std::size_t>(i) * 4U + 1U] = static_cast<unsigned char>(c.g * 255.0f + 0.5f);
        degr.pixels[static_cast<std::size_t>(i) * 4U + 2U] = static_cast<unsigned char>(c.b * 255.0f + 0.5f);
        degr.pixels[static_cast<std::size_t>(i) * 4U + 3U] = 255U;
    }
    // Se sube como textura 1D. En fondo.fs se lee con la ELEVACIÓN de cada
    // dirección: s = asin(d.z) / 90° (0 en el horizonte, 1 mirando arriba).
    degrade_.load_1d(degr);
}

glm::vec3 Skybox::color_horizonte() const
{
    // Con el degradé, el horizonte es su texel 0, que se armó con el mismo
    // promedio: en los dos modos la niebla coincide con el cielo.
    return horizonte_cubo_;
}

float Skybox::elevacion_sol_grados() const
{
    // Elevación = ángulo de la dirección sobre el horizonte. Como dir_sol_ es
    // UNITARIA, su componente z (la altura, escena Z arriba) es el SENO de ese
    // ángulo: z = sin(elevación). asin (arcoseno) hace la cuenta inversa: dado
    // el seno, devuelve el ángulo, en radianes, entre -90° y +90°.
    //   z = 0 -> 0° (horizonte), z = 0.5 -> 30°, z = 1 -> 90° (cenit).
    // El clamp asegura que z esté en [-1, 1]: por redondeo podría dar 1.0000001
    // y asin de algo fuera de [-1, 1] da NaN ("no es un número").
    // glm::degrees pasa de radianes a grados. Con el sol pintado:
    // asin(0.497) = 0.520 rad = 29.8°.
    return glm::degrees(std::asin(glm::clamp(dir_sol_.z, -1.0f, 1.0f)));
}

void Skybox::draw(const ParametrosFondo& p) const
{
    // 1) La vista SIN TRASLACIÓN: mat3 se queda con la rotación (la 3x3 de
    //    arriba a la izquierda) y mat4 la vuelve a agrandar con identidad
    //    (traslación = 0). Equivale a mover el cubo junto con el ojo.
    const glm::mat4 vista_sin_traslacion = glm::mat4(glm::mat3(p.vista));

    shader_.use();
    shader_.set_uniform(loc_proyeccion_, p.proyeccion);
    shader_.set_uniform(loc_vista_, vista_sin_traslacion);
    shader_.set_uniform(loc_modo_, modo_ == ModoCielo::Degrade ? 1 : 0);
    shader_.set_uniform(loc_suelo_continuado_, p.suelo_continuado ? 1 : 0);
    shader_.set_uniform(loc_ojo_uv_, p.ojo_uv);
    shader_.set_uniform(loc_altura_ojo_, p.altura_ojo);
    shader_.set_uniform(loc_tam_tesela_, p.tam_tesela);
    shader_.set_uniform(loc_niebla_color_, p.niebla_color);
    shader_.set_uniform(loc_niebla_inicio_, p.niebla_inicio);
    shader_.set_uniform(loc_niebla_fin_, p.niebla_fin);

    // 2) Texturas en sus unidades (las mismas del layout(binding) de fondo.fs).
    //    La GPU tiene varias UNIDADES DE TEXTURA numeradas (0, 1, 2, ...), como
    //    "enchufes". bind(n) (= glBindTextureUnit(n, id)) enchufa la textura en
    //    la unidad n. El shader no conoce las texturas: sus samplers dicen de
    //    qué UNIDAD leer, con layout(binding = n) en fondo.fs:
    //      binding = 0 -> uCielo        (samplerCube) <- el cubemap
    //      binding = 1 -> uTexturaSuelo (sampler2D)   <- el pasto del terreno
    //      binding = 2 -> uDegrade      (sampler1D)   <- el degradé 1D
    //    Por eso los números: tienen que COINCIDIR con los del shader. Si se
    //    enchufara el degradé en la 0, uCielo leería una textura de otro tipo.
    //    Se usan las tres siempre: el shader lee las tres en cada fragmento
    //    (y elige después), así que todas tienen que estar enchufadas.
    cubemap_.bind(0);
    if (p.suelo != nullptr) {
        p.suelo->bind(1);
    }
    degrade_.bind(2);

    // 3) Estado para el fondo:
    //    - GL_LEQUAL: el fondo (profundidad 1) pasa donde el z-buffer quedó en 1.
    //    - sin recorte de caras: el cubo se ve desde adentro (giro invertido).
    //    - relleno: aunque la escena esté en modo alambre (F), el cielo no.
    //
    //    glDepthFunc: la PRUEBA DE PROFUNDIDAD. Cada fragmento compara su
    //      profundidad con la del z-buffer; con GL_LESS (la normal) solo se
    //      dibuja si está MÁS CERCA. El fondo se dibuja con profundidad
    //      exactamente 1 (lo más lejos posible, ver fondo.vs: pos.xyww), y
    //      donde no hay nada dibujado el z-buffer también vale 1: con GL_LESS,
    //      1 < 1 es falso y no se vería. Con GL_LEQUAL (menor O IGUAL) pasa.
    //      Donde ya hay terreno o avión (profundidad < 1), el fondo no pasa.
    //    glDisable(GL_CULL_FACE): apaga el RECORTE DE CARAS TRASERAS. La cámara
    //      está ADENTRO del cubo, así que ve el lado de atrás de sus caras, que
    //      el recorte descartaría (y no se vería cielo).
    //    glPolygonMode(..., GL_FILL): triángulos RELLENOS. La tecla F pone la
    //      escena en modo alambre (GL_LINE); el cielo se dibuja relleno igual.
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Dibujar el cubo:
    //    glBindVertexArray: activa el VAO del cubo (dónde están sus vértices
    //      y cómo leerlos: posición, normal, uv).
    //    glDrawElements: dibuja cubo_.count() índices como TRIÁNGULOS (12
    //      triángulos = 36 índices), enteros sin signo, desde el principio del
    //      buffer de índices del VAO (nullptr = desplazamiento 0).
    //    glBindVertexArray(0): desactiva el VAO (para no modificarlo por error).
    glBindVertexArray(cubo_.vao());
    glDrawElements(GL_TRIANGLES, cubo_.count(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // 4) Volver al estado por omisión para el próximo cuadro (el que esperan el
    //    terreno y el avión). El modo de polígono no se restaura acá: main lo
    //    fija al principio de cada cuadro según la tecla F.
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
}
