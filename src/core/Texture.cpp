// -----------------------------------------------------------------------------
// Texture.cpp — dueño único de una textura de OpenGL (Práctico 08, Parte 1)
// -----------------------------------------------------------------------------

#include "core/Texture.h"  // la declaración de la clase que se implementa acá

#include <algorithm>       // std::max: el lado mayor de la imagen (niveles de mipmap)
#include <cmath>           // std::floor y std::log2: cantidad de niveles de mipmap
#include <stdexcept>       // std::runtime_error: imagen vacía, caras del cubemap distintas
#include <string>          // std::string (hoy no se usa directamente; se puede quitar)
#include <utility>         // std::move y similares (hoy no se usa directamente; se puede quitar)

#include <glad/gl.h>       // funciones de OpenGL (glCreateTextures, glTextureStorage2D, ...)

#include "core/ResourceManager.h"   // ImageData

namespace {

// Traducción de los enums propios a las constantes de OpenGL (el encabezado no
// expone GL_*: así quien usa Texture no necesita incluir glad).
GLint a_gl(Filtro f)
{
    switch (f) {
        case Filtro::Cercano:            return GL_NEAREST;
        case Filtro::Lineal:             return GL_LINEAR;
        case Filtro::LinealMipmapLineal: return GL_LINEAR_MIPMAP_LINEAR;
    }
    return GL_LINEAR;
}

GLint a_gl(Repeticion r)
{
    switch (r) {
        case Repeticion::Repetir:         return GL_REPEAT;
        case Repeticion::RepetirEspejado: return GL_MIRRORED_REPEAT;
        case Repeticion::BordeFijo:       return GL_CLAMP_TO_EDGE;
    }
    return GL_REPEAT;
}

// Cantidad de niveles de la pirámide de mipmaps: floor(log2(max(w, h))) + 1.
// Cada nivel tiene la mitad del lado del anterior, hasta llegar a 1x1:
// 256 -> 128 -> ... -> 1 son 9 niveles; 1024 -> ... -> 1 son 11.
int niveles_mipmap(int w, int h)
{
    const int lado = std::max(w, h);
    return static_cast<int>(std::floor(std::log2(static_cast<double>(lado)))) + 1;
}

// Luminancia media de una imagen RGBA (pesos de Rec. 709), en [0, 1].
//
// Qué es: el BRILLO PROMEDIO de la imagen, de 0 (toda negra) a 1 (toda
// blanca). Para la chapa da 0.573 (se imprime al arrancar).
//
// Para qué: la apariencia "metal gris" del avión (tecla T). El shader hace
// color x textura; sin compensar, gris 0.58 x chapa (promedio 0.573) ~ 0.33:
// el avión quedaría un 43 % más oscuro. Aircraft divide antes el color por
// este valor (0.58 / 0.573 ~ 1.01) y así, en promedio, el gris queda como se
// eligió y la chapa solo suma variación (rayones claros, juntas oscuras).
// Es la misma idea que `lum / lum.mean()` en tools/generar_camuflaje.py.
float luminancia_media(const ImageData& image)
{
    // Imagen vacía: 1 es el valor NEUTRO (quien divida por el brillo no divide
    // por cero ni cambia nada).
    if (image.pixels.empty()) {
        return 1.0f;
    }
    // double y no float: la chapa tiene ~1 millón de texels y la suma llega a
    // ~150 millones; un float (unas 7 cifras) perdería decimales al sumarle
    // números chicos a uno tan grande.
    double suma = 0.0;
    const std::size_t texels = image.pixels.size() / 4U;   // 4 bytes (RGBA) por texel
    for (std::size_t i = 0; i < texels; ++i) {
        // p apunta a los 4 bytes del texel i: p[0] = R, p[1] = G, p[2] = B
        // (0..255); p[3] = alfa, que no se usa.
        const unsigned char* p = &image.pixels[i * 4U];
        // LUMINANCIA: convierte el color en UN número de brillo (como pasar
        // una foto a blanco y negro). No es (R+G+B)/3 porque el ojo no es igual
        // de sensible a los tres: un verde puro se ve mucho más claro que un
        // azul puro. Pesos del estándar Rec. 709 (monitores y TV HD):
        //   verde 0.7152 (mucho)   rojo 0.2126 (medio)   azul 0.0722 (poco)
        // Suman 1: el blanco (255, 255, 255) da exactamente 255.
        // Detalle: se aplican a los bytes sRGB tal cual (en rigor la luminancia
        // es sobre valores lineales), pero acá es lo correcto: el shader del
        // Práctico 08 también multiplica los valores sRGB tal cual, y lo que se
        // quiere compensar es justamente ese promedio.
        suma += 0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2];
    }
    // Promedio (suma / texels) llevado de 0..255 a 0..1 (/ 255).
    return static_cast<float>(suma / (static_cast<double>(texels) * 255.0));
}

} 

Texture::~Texture()
{
    clear();
}

Texture::Texture(Texture&& other) noexcept
    : id_(other.id_), tipo_(other.tipo_), niveles_(other.niveles_),
      ancho_(other.ancho_), alto_(other.alto_), brillo_medio_(other.brillo_medio_)
{
    other.id_ = 0U;                       // el movido queda vacío (decisión de Mesh)
    other.tipo_ = TipoTextura::Ninguna;
    other.niveles_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        clear();                          // libera lo propio antes de adueñarse
        id_ = other.id_;
        tipo_ = other.tipo_;
        niveles_ = other.niveles_;
        ancho_ = other.ancho_;
        alto_ = other.alto_;
        brillo_medio_ = other.brillo_medio_;
        other.id_ = 0U;
        other.tipo_ = TipoTextura::Ninguna;
        other.niveles_ = 0;
    }
    return *this;
}

void Texture::load_2d(const ImageData& image, const Muestreo& muestreo,
                      bool generar_mipmaps, bool srgb)
{
    if (image.pixels.empty() || image.width <= 0 || image.height <= 0) {
        throw std::runtime_error("Texture::load_2d: imagen vacía");
    }
    clear();   // el almacenamiento es INMUTABLE: para recargar hay que crear otra

    const int w = image.width, h = image.height;
    niveles_ = niveles_mipmap(w, h);

    // 1. CREAR: devuelve el identificador (igual que un buffer o un VAO).
    glCreateTextures(GL_TEXTURE_2D, 1, &id_);

    // 2. RESERVAR: tamaño, formato INTERNO (cómo se guarda en la GPU) y niveles.
    //    GL_RGBA8 = 4 canales de 8 bits, leídos tal cual (byte / 255). Con srgb,
    //    GL_SRGB8_ALPHA8: mismos 4 canales, pero R, G y B se pasan de sRGB a
    //    lineal al leer (Práctico 09). Explicación completa en Texture.h.
    glTextureStorage2D(id_, niveles_, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, w, h);

    // 3. LLENAR el nivel 0 con los bytes de la imagen. GL_RGBA + GL_UNSIGNED_BYTE
    //    describen los datos de la CPU (no cómo se guardan): OpenGL convierte.
    glTextureSubImage2D(id_, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
                        image.pixels.data());
    //    ...y la pirámide: la GPU calcula los niveles 1..n-1 promediando el
    //    anterior. Si NO se llama (generar_mipmaps = false), los niveles existen
    //    pero su contenido es INDEFINIDO y OpenGL no avisa: "la trampa".
    if (generar_mipmaps) {
        glGenerateTextureMipmap(id_);
    }

    tipo_ = TipoTextura::T2D;
    ancho_ = w;
    alto_ = h;
    brillo_medio_ = luminancia_media(image);

    // 4. PARAMETRIZAR el muestreo.
    set_muestreo(muestreo);
}

void Texture::load_cubemap(const std::array<const ImageData*, 6>& faces, bool srgb)
{
    // Validación: seis caras cuadradas e iguales (si no, el cubo tendría
    // costuras o directamente OpenGL rechazaría la carga).
    for (const ImageData* f : faces) {
        if (f == nullptr || f->pixels.empty()) {
            throw std::runtime_error("Texture::load_cubemap: falta una cara");
        }
        if (f->width != faces[0]->width || f->height != faces[0]->height ||
            f->width != f->height) {
            throw std::runtime_error("Texture::load_cubemap: las caras tienen que "
                                     "ser cuadradas y del mismo tamaño");
        }
    }
    clear();

    const int w = faces[0]->width, h = faces[0]->height;
    niveles_ = 1;   // sin mipmaps: el cielo casi siempre se AMPLÍA (un texel ocupa
                    // más de un píxel), así que no hay reducción que filtrar

    // 1. crear (tipo cubemap)
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &id_);
    // 2. reservar COMO 2D: OpenGL ya sabe que son 6 caras de w x h.
    //    GL_RGB8 (3 bytes/texel): el cielo no necesita transparencia (las
    //    caras traen alfa, pero vale 255 en todos los píxeles).
    glTextureStorage2D(id_, niveles_, srgb ? GL_SRGB8 : GL_RGB8, w, h);
    // 3. llenar COMO 3D: cada cara es una CAPA (la coordenada z = índice de cara,
    //    en el orden +X -X +Y -Y +Z -Z). Los datos llegan en RGBA y se guardan en
    //    RGB: OpenGL descarta el alfa al convertir.
    for (int cara = 0; cara < 6; ++cara) {
        glTextureSubImage3D(id_, 0, 0, 0, cara, w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                            faces[static_cast<std::size_t>(cara)]->pixels.data());
    }

    tipo_ = TipoTextura::Cubemap;
    ancho_ = w;
    alto_ = h;
    brillo_medio_ = 1.0f;

    // 4. parametrizar: lineal y BORDE FIJO en las tres direcciones (S, T, R).
    //    Con GL_REPEAT el filtrado tomaría texels del borde OPUESTO de la cara y
    //    se verían líneas en las aristas del cubo.
    Muestreo m;
    m.reduccion = Filtro::Lineal;
    m.ampliacion = Filtro::Lineal;
    m.s = m.t = m.r = Repeticion::BordeFijo;
    set_muestreo(m);
}

void Texture::load_1d(const ImageData& image)
{
    if (image.pixels.empty() || image.width <= 0) {
        throw std::runtime_error("Texture::load_1d: imagen vacía");
    }
    clear();

    const int n = image.width;
    niveles_ = 1;
    glCreateTextures(GL_TEXTURE_1D, 1, &id_);                        // 1. crear
    glTextureStorage1D(id_, niveles_, GL_RGBA8, n);                  // 2. reservar (n texels)
    glTextureSubImage1D(id_, 0, 0, n, GL_RGBA, GL_UNSIGNED_BYTE,     // 3. llenar con la
                        image.pixels.data());                        //    primera fila
    tipo_ = TipoTextura::T1D;
    ancho_ = n;
    alto_ = 1;
    brillo_medio_ = luminancia_media(image);

    Muestreo m;                                                      // 4. parametrizar
    m.reduccion = Filtro::Lineal;
    m.ampliacion = Filtro::Lineal;
    m.s = Repeticion::BordeFijo;   // fuera de [0,1] se usa el texel del extremo
    set_muestreo(m);
}

// Etapa 4 (PARAMETRIZAR): le dice a la GPU CÓMO LEER la textura. No toca los
// datos (el almacenamiento es inmutable), solo el modo de lectura.
//
// Por qué fija SIEMPRE los cinco parámetros, aunque algunos no cambien o no se
// usen en ese tipo de textura:
//   1. Muestreo describe el estado COMPLETO de lectura, y set_muestreo lo
//      aplica entero. Así, después de llamarla, la textura queda EXACTAMENTE
//      como dice el Muestreo, sin depender de lo que tenía antes. Fijar solo
//      "lo que cambió" obligaría a guardar el estado anterior y compararlo
//      (más código y más lugares para equivocarse).
//   2. En la primera llamada (al cargar) hay que fijarlos todos igual: los
//      valores por omisión de OpenGL no son los que queremos (por ejemplo,
//      el MIN_FILTER por omisión es GL_NEAREST_MIPMAP_LINEAR).
//   3. Los que no aplican son INOFENSIVOS: WRAP_R en una textura 2D o WRAP_T
//      en una 1D son parámetros válidos que OpenGL guarda e ignora al leer.
//   4. Es barato y se llama POCO: cada glTextureParameteri solo anota un valor
//      en el objeto textura. Se llama una vez al cargar cada textura (desde
//      load_2d, load_cubemap y load_1d) y, en el bucle, SOLO cuando cambia la
//      tecla I (main guarda el último modo aplicado y no re-parametriza en
//      cada cuadro). No hay costo por cuadro.
void Texture::set_muestreo(const Muestreo& muestreo)
{
    // Textura vacía (nunca se cargó o ya se liberó): no hay nada que
    // parametrizar. glTextureParameteri sobre el id 0 daría error de OpenGL.
    if (id_ == 0U) {
        return;
    }

    // --- Filtro de REDUCCIÓN (MIN): cuando un píxel de pantalla cubre MUCHOS
    // texels (la textura se ve lejos/chica). Es el único que usa mipmaps.
    // Una textura sin mipmaps (1 nivel) no puede usar un filtro *_MIPMAP_*: la
    // textura quedaría "incompleta" y se leería NEGRO. Se degrada a lineal.
    // (Le pasa al cubemap y al degradé 1D, que se crean con un solo nivel.
    // Ojo: la "trampa" de la tecla I SÍ tiene 9 niveles reservados, por eso no
    // se degrada y se ve negra de lejos: ese es el error que se quiere mostrar.)
    Filtro reduccion = muestreo.reduccion;
    if (niveles_ <= 1 && reduccion == Filtro::LinealMipmapLineal) {
        reduccion = Filtro::Lineal;
    }
    glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, a_gl(reduccion));

    // --- Filtro de AMPLIACIÓN (MAG): cuando un texel cubre VARIOS píxeles (la
    // textura se ve cerca/grande). Solo admite NEAREST o LINEAR: al ampliar
    // siempre se lee el nivel 0, no hay nivel "más grande" que elegir, así que
    // los mipmaps no intervienen. Si el Muestreo pidiera LinealMipmapLineal
    // acá, se usa GL_LINEAR (pasarle un *_MIPMAP_* daría error de OpenGL).
    glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER,
                        muestreo.ampliacion == Filtro::Cercano ? GL_NEAREST : GL_LINEAR);

    // --- REPETICIÓN en cada eje (qué leer si la coordenada sale de [0, 1]):
    // S = u, T = v, R = w (ver Repeticion y Muestreo en Texture.h). Se fijan
    // los tres siempre (punto 3 de arriba): en una 2D, R se ignora; en la 1D,
    // T y R se ignoran; el cubemap usa los tres.
    glTextureParameteri(id_, GL_TEXTURE_WRAP_S, a_gl(muestreo.s));
    glTextureParameteri(id_, GL_TEXTURE_WRAP_T, a_gl(muestreo.t));
    glTextureParameteri(id_, GL_TEXTURE_WRAP_R, a_gl(muestreo.r));
}

void Texture::bind(unsigned int unit) const
{
    // DSA: conecta la textura a la unidad `unit` sin glActiveTexture/glBindTexture.
    glBindTextureUnit(unit, id_);
}

void Texture::clear(void)
{
    glDeleteTextures(1, &id_);   // glDeleteTextures(0) no hace nada: sin banderas
    id_ = 0U;
    tipo_ = TipoTextura::Ninguna;
    niveles_ = 0;
}
