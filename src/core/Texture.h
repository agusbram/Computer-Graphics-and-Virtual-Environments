// -----------------------------------------------------------------------------
// Texture.h — dueño único de una textura de OpenGL (Práctico 08, Parte 1)
//
// Una textura es un ARREGLO DE DATOS en la memoria de la GPU que se indexa con
// coordenadas normalizadas y del que se obtiene un valor por fragmento
// aplicando un filtrado. Igual que un buffer: hay que reservarla y, al terminar,
// liberarla. Por eso recibe EL MISMO TRATAMIENTO QUE Mesh (Práctico 02):
//   1. DUEÑO ÚNICO del identificador: el destructor lo libera, la copia está
//      prohibida y mover traspasa la propiedad (el objeto movido queda en 0).
//   2. clear() sin banderas: glDeleteTextures(0) no hace nada.
//
// Crear una textura = CUATRO ETAPAS, en estilo DSA (sin glGenTextures ni
// glBindTexture: se opera directo sobre el identificador):
//   1. crear       glCreateTextures            -> devuelve el identificador
//   2. reservar    glTextureStorage1D/2D       -> tamaño, formato y niveles de
//                                                  mipmap; INMUTABLE después
//   3. llenar      glTextureSubImage1D/2D/3D   -> los datos (+ glGenerateTextureMipmap)
//   4. parametrizar glTextureParameteri        -> filtros y repetición
//
// NO lee archivos: recibe la imagen ya decodificada (ImageData) del
// ResourceManager. Es la misma separación que MeshData (CPU) -> Mesh (GPU).
//
// Decisiones de diseño (las cuatro preguntas de la guía):
//   - ¿Se libera la imagen de la CPU después de subirla? -> sí, lo hace quien
//     la cargó, con ResourceManager::unload_image (Texture no la retiene).
//   - ¿Cuándo se invierte la imagen? -> AL CARGARLA (ResourceManager), no acá
//     ni en el shader: Texture sube las filas en el orden en que llegan.
//   - ¿Una clase o dos para 2D y cubemap? -> UNA SOLA (como propone la guía),
//     que recuerda su tipo. También sirve para la textura 1D del cielo degradé.
//   - ¿Muestreo fijo o cambiable? -> CAMBIABLE (set_muestreo): permite mostrar
//     EN VIVO la diferencia entre filtrar "con" y "sin" mipmaps (tecla I).
//
// Para no exponer constantes de OpenGL en el encabezado (igual que Mesh.h,
// que no incluye glad), los filtros y modos de repetición son enums propios y
// se traducen a GL_* en Texture.cpp.
// -----------------------------------------------------------------------------

#pragma once

#include <array>  // std::array: las 6 caras de load_cubemap

struct ImageData;   // definida en ResourceManager.h (C++ puro); acá alcanza con declararla

// Qué hacer cuando un fragmento no cae justo sobre un texel (FILTRADO).
enum class Filtro {
    Cercano,               // GL_NEAREST: el texel más cercano (bloques; para datos)
    Lineal,                // GL_LINEAR: promedio de los 4 vecinos (sin mipmaps)
    LinealMipmapLineal,    // GL_LINEAR_MIPMAP_LINEAR: trilineal (solo reducción)
};

// Qué hacer cuando la coordenada sale de [0, 1] (REPETICIÓN, en inglés "wrap").
// La imagen ocupa de 0 a 1 en cada eje; si el shader pide, por ejemplo,
// u = 3.7 o u = -0.2, el modo decide qué texel se lee:
//   Repetir          u = 3.7 -> lee 0.7 (descarta la parte entera): baldosas.
//   RepetirEspejado  u = 3.7 -> lee 0.3: se repite, pero una copia sí y una no
//                    aparece reflejada. (No se usa; ver Consultas, pregunta 17.)
//   BordeFijo        u = 3.7 -> lee 1.0: no repite, estira el texel del borde.
enum class Repeticion {
    Repetir,               // GL_REPEAT: la textura se repite (terreno, avión)
    RepetirEspejado,       // GL_MIRRORED_REPEAT: se repite reflejada
    BordeFijo,             // GL_CLAMP_TO_EDGE: se estira el texel del borde (cielo)
};

// Parámetros de muestreo (etapa 4). Los valores por omisión son los que la guía
// recomienda para el TERRENO: trilineal al reducir, lineal al ampliar y
// repetición en S y T (R solo la usa el cubemap).
//
// S, T y R NO SON TECLAS: son los nombres que OpenGL les da a los EJES de las
// coordenadas de textura (por tradición, en vez de u, v, w):
//   S = u (horizontal de la imagen)   -> todas las texturas
//   T = v (vertical de la imagen)     -> 2D, 3D y cubemap
//   R = w (tercera coordenada)        -> solo 3D y cubemap (una 2D no la tiene)
// Cada eje tiene su propio modo de repetición. En el proyecto:
//   - terreno y avión: Repetir en S y T (los valores por omisión). En el
//     terreno (u, v) = posición en el mundo / 50 m, valores como 37.4 o 512.8:
//     sin repetir no cubriría los 51 km.
//   - cubemap: BordeFijo en S, T y R (con Repetir se verían líneas en las
//     aristas del cubo, ver load_cubemap).
//   - degradé 1D: BordeFijo en S (si s se pasa apenas de 1, sigue el cenit).
// Ninguna tecla cambia la repetición; la tecla I cambia solo el FILTRO del pasto.
struct Muestreo {
    Filtro     reduccion  = Filtro::LinealMipmapLineal;   // GL_TEXTURE_MIN_FILTER
    Filtro     ampliacion = Filtro::Lineal;               // GL_TEXTURE_MAG_FILTER
    Repeticion s = Repeticion::Repetir;                   // GL_TEXTURE_WRAP_S (u)
    Repeticion t = Repeticion::Repetir;                   // GL_TEXTURE_WRAP_T (v)
    Repeticion r = Repeticion::Repetir;                   // GL_TEXTURE_WRAP_R (cubemap)
};

// Tipo de textura que guarda el objeto (lo decide el load_* que se llamó).
enum class TipoTextura { Ninguna, T1D, T2D, Cubemap };

class Texture {
public:
    Texture() = default;
    ~Texture();                                   // libera lo que posee

    Texture(const Texture&) = delete;             // las MISMAS tres reglas que Mesh
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Textura 2D: las cuatro etapas a partir de una imagen ya decodificada.
    // Niveles de mipmap = floor(log2(max(w, h))) + 1 (9 para 256 px, 10 para 512).
    //   generar_mipmaps = false reserva los niveles pero NO los llena: es "la
    //     trampa de los mipmaps" de la guía, a propósito, para poder verla
    //     (OpenGL no da error; los niveles quedan con contenido indefinido).
    //   srgb = true usa el formato GL_SRGB8_ALPHA8 (la GPU convierte a lineal al
    //     leer). Queda preparado para la corrección gamma del Práctico 09; en el
    //     Práctico 08 todas las texturas usan GL_RGBA8 (srgb = false).
    //
    // GL_RGBA8 vs GL_SRGB8_ALPHA8 (el "formato interno": cómo se guarda en la
    // GPU). Los dos guardan 4 canales de 8 bits y ocupan lo mismo; cambia cómo
    // se INTERPRETAN R, G y B al leerlos con texture() (el alfa, nunca):
    //   GL_RGBA8         byte 128 -> 0.502 (byte / 255, tal cual)
    //   GL_SRGB8_ALPHA8  byte 128 -> 0.216 (lo pasa de sRGB a lineal)
    //
    // RGBA y sRGB NO son opciones de lo mismo:
    //   - RGBA dice QUÉ CANALES hay (rojo, verde, azul, alfa): la "forma".
    //   - sRGB dice QUÉ SIGNIFICA cada número (cómo se traduce en luz): la
    //     "codificación". Se combinan:   lineal     sRGB
    //                       3 canales:   GL_RGB8    GL_SRGB8
    //                       4 canales:   GL_RGBA8   GL_SRGB8_ALPHA8
    //
    // Por qué existe sRGB: los PNG/JPG no guardan la luz real sino un valor
    // "comprimido con gamma" (~ luz^(1/2.2)), porque el ojo distingue mejor los
    // oscuros y así los 256 valores se reparten como los percibimos. El gris
    // 128 es, en luz real, solo el 21.6 % del blanco. Las cuentas de luz del
    // Práctico 09 (kd * luz * cos θ, sumar luces) son física y necesitan
    // valores LINEALES; con sRGB tal cual, las sombras salen muy oscuras y la
    // transición luz/sombra, dura. GL_SRGB8_ALPHA8 hace la conversión gratis,
    // por hardware y ANTES de filtrar (promedia bien entre texels vecinos).
    //
    // Por qué en el Práctico 08 va GL_RGBA8: no hay cuentas de luz, el shader
    // copia el color a la pantalla. La pantalla espera sRGB y el archivo ya es
    // sRGB: el byte que entra es el que sale. Con GL_SRGB8_ALPHA8 y nada más,
    // todo se vería mucho más oscuro (el gris 128 saldría como 55).
    //
    // En el Práctico 09 hay que cerrar el circuito: entrar en lineal (srgb =
    // true), calcular la luz en lineal y SALIR en sRGB (glEnable(
    // GL_FRAMEBUFFER_SRGB) o pow(color, 1/2.2) al final del shader). También
    // el kd de las piezas y el color de la niebla se pasan a lineal.
    // OJO: solo las texturas de COLOR van en sRGB. Las de DATOS (el normal map
    // chapa_normal_gl.jpg) van en GL_RGBA8: cada byte es una dirección, no un
    // color, y la conversión gamma la deformaría.
    void load_2d(const ImageData& image, const Muestreo& muestreo = Muestreo{},
                 bool generar_mipmaps = true, bool srgb = false);

    // Cubemap: seis caras en el orden +X -X +Y -Y +Z -Z (right, left, top,
    // bottom, front, back). Se reserva como 2D y se llena como 3D (cada cara es
    // una CAPA). Las caras llegan SIN invertir. Todas tienen que ser cuadradas
    // y del mismo tamaño (si no, lanza std::runtime_error).
    void load_cubemap(const std::array<const ImageData*, 6>& faces, bool srgb = false);

    // Textura 1D (opcional de la guía: cielo degradé). Usa la PRIMERA FILA de la
    // imagen (una imagen de ancho N y alto 1). Sin mipmaps, borde fijo.
    void load_1d(const ImageData& image);

    // Cambia los parámetros de muestreo de una textura ya creada (etapa 4 otra
    // vez). El almacenamiento no cambia: solo cómo se lee.
    void set_muestreo(const Muestreo& muestreo);

    // Conecta la textura a una UNIDAD de textura para los próximos dibujos
    // (glBindTextureUnit). En el shader, el sampler con layout(binding = unit)
    // lee esta textura. OJO: el sampler guarda el NÚMERO de unidad, no el id.
    void bind(unsigned int unit) const;

    void clear(void);

    unsigned int id(void) const       { return id_; }
    TipoTextura  tipo(void) const     { return tipo_; }
    int          niveles(void) const  { return niveles_; }   // niveles de mipmap reservados
    int          ancho(void) const    { return ancho_; }
    int          alto(void) const     { return alto_; }
    // Brillo medio de la imagen (luminancia promedio en [0, 1], calculada al
    // cargar con luminancia_media, ver Texture.cpp). Lo usa el avión para que
    // la chapa agregue DETALLE sin oscurecer el color de la pieza (ver
    // Aircraft::collect). Chapa: 0.573. Cubemap: 1 (no se calcula).
    float        brillo_medio(void) const { return brillo_medio_; }

private:
    unsigned int id_ {0U};
    TipoTextura  tipo_ {TipoTextura::Ninguna};
    int          niveles_ {0};
    int          ancho_ {0};
    int          alto_ {0};
    float        brillo_medio_ {1.0f};
};
