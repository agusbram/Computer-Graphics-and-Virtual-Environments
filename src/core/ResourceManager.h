// -----------------------------------------------------------------------------
// ResourceManager.h
//
// Cache de recursos del proyecto en C++ puro (solo biblioteca estandar, sin
// OpenGL). Hoy administra fuentes de shaders; mas adelante se amplía con el
// mismo patron (struct por tipo + load_* + get_* + mapa) para texturas,
// modelos de mallas poligonales, etc.
//
// Decisiones de diseño (Practico 01, Parte 2):
//   1. ERRORES -> EXCEPCIONES (std::runtime_error).
//      El constructor valida que assets_root exista; load_shader_source
//      lanza si un archivo no existe o no se puede leer; get_shader_source
//      lanza si la clave nunca se cargo. Razon: un fallo nunca pasa
//      desapercibido (no se rompe en silencio) y el llamador no puede
//      olvidarse de chequear el resultado.
//   2. CLAVE REPETIDA -> warning por stderr y se devuelve la copia cacheada.
//      Si load_shader_source recibe una clave ya cargada con los MISMOS
//      archivos, devuelve el cache sin tocar disco (comportamiento normal).
//      Si los archivos son DISTINTOS, avisa por consola y gana el cache:
//      la primera carga es la fuente de verdad.
//   3. RETORNO POR REFERENCIA CONST.
//      Evita copiar strings grandes en cada pedido. ATENCION: la referencia
//      queda INVALIDA si alguien llama clear() o destruye el ResourceManager;
//      es responsabilidad del llamador no retenerla mas alla de eso.
//   4. CACHE INDEXADO POR CLAVE DEL USUARIO (no por nombre de archivo).
//      Permite referirse al recurso de forma semantica ("triangle") aunque
//      los archivos se renombren o reorganice la carpeta assets/.
//
// Práctico 08 — IMÁGENES (texturas). Mismo patrón: un struct por tipo de
// recurso (ImageData) + load_image / get_image + un mapa propio. Decodificar
// una imagen (PNG/JPG -> bytes) es C++ puro: se hace con stb_image y NO toca
// OpenGL. La subida a la GPU es responsabilidad de la clase Texture (es la
// misma separación que MeshData -> Mesh). Decisiones nuevas:
//   5. SIEMPRE RGBA (4 bytes por texel), aunque el archivo tenga 3 canales:
//      stbi_load(..., 4). Así el código que sube la imagen a la GPU no depende
//      de cuántos canales trae el archivo (channels queda solo informativo).
//   6. EL VOLTEO VERTICAL SE DECIDE AL CARGAR (parámetro flip_vertically).
//      Una imagen guarda primero la fila de ARRIBA; OpenGL pone el (0,0) de una
//      textura 2D ABAJO a la izquierda: sin voltear, la textura sale invertida.
//      Las caras de un cubemap NO se voltean (su convención tiene el origen
//      arriba). stbi_set_flip_vertically_on_load es un ESTADO GLOBAL de la
//      biblioteca, por eso se fija explícitamente en CADA carga (si no, el
//      valor de la carga anterior "contaminaría" a la siguiente).
//   7. MISMA CLAVE CON OTRO ARCHIVO U OTRO FLIP -> aviso + gana el caché (igual
//      que con los shaders: la primera carga es la fuente de verdad). Para
//      tener la misma imagen invertida y sin invertir hay que usar DOS claves.
//   8. unload_image(key): libera la copia de la CPU cuando la GPU ya tiene la
//      suya (un cubemap de 6 x 900 x 900 en RGBA ocupa ~19 MB que no se vuelven
//      a usar). Costo: si se la vuelve a pedir, hay que releer el disco.
// -----------------------------------------------------------------------------

#pragma once

#include <filesystem>     // std::filesystem::path: la carpeta assets y las rutas de los archivos
#include <string>         // std::string: claves del caché, rutas y código de los shaders
#include <unordered_map>  // std::unordered_map: los cachés (clave -> shader / imagen)
#include <vector>         // std::vector: los bytes de la imagen (ImageData::pixels)

// Codigo fuente de un programa de shader, tal como se leyo de disco.
struct ShaderSource {
    std::string vs;             // vertex shader
    std::string fs;             // fragment shader
    std::string gs;             // geometry shader (opcional, hoy no se usa)
};

// Imagen decodificada, lista para subir a la GPU (Práctico 08). C++ puro.
// Fila 0 de `pixels` = fila de ABAJO si se cargó con flip (convención OpenGL),
// o la de ARRIBA si no (convención del archivo y de las caras del cubemap).
//
// CANALES: cada píxel es un grupo de números de 1 byte (0..255), uno por
// CANAL. Según cuántos tenga, la imagen es:
//   1 = gris (solo brillo)            2 = gris + alfa
//   3 = RGB (rojo, verde, azul)       4 = RGBA (RGB + alfa)
// ALFA = opacidad: 255 opaco, 0 transparente, intermedio semitransparente.
//
// Hay DOS datos distintos:
//   - `channels`: cuántos canales tiene el ARCHIVO. Es solo INFORMATIVO.
//     Nuestras imágenes: grass, chapa, camuflaje y prueba_uv son RGB (3); las
//     caras del cubemap son RGBA (4), con alfa 255 en todos los píxeles.
//   - `pixels`: SIEMPRE RGBA (4 bytes por texel), cualquiera sea el archivo.
//     stbi_load(..., 4) lo convierte: a un RGB le agrega alfa = 255.
//
// Orden de los bytes: fila por fila y, dentro de la fila, píxel por píxel:
//   [R G B A][R G B A]...  <- fila 0
//   [R G B A][R G B A]...  <- fila 1
// Tamaño total = width * height * 4 bytes (pasto 256x256: 262.144 bytes).
// El canal c (0 = R, 1 = G, 2 = B, 3 = A) del píxel (x, y) está en
//   pixels[(y * width + x) * 4 + c]
//
// Por qué forzar SIEMPRE RGBA:
//   1. Un solo camino de subida: Texture sube siempre con GL_RGBA +
//      GL_UNSIGNED_BYTE, sin preguntar cuántos canales tenía el archivo.
//   2. Evita la TRAMPA DE ALINEACIÓN: OpenGL supone que cada fila empieza en
//      un múltiplo de 4 bytes (GL_UNPACK_ALIGNMENT = 4). En RGB una fila mide
//      ancho * 3 bytes y, si no es múltiplo de 4, la imagen se ve "torcida" en
//      diagonal. Con 4 bytes por píxel cualquier ancho funciona.
//   3. Cuesta poco: un 33 % más de memoria en la CPU, solo mientras se carga
//      (después se libera con unload_image).
//
// EL ALFA NO SE USA EN ESTE PROYECTO: ninguna imagen tiene transparencia, los
// shaders leen solo .rgb y escriben siempre alfa = 1.0, y nunca se activa
// GL_BLEND (sin blending OpenGL ignora el alfa). El byte está porque todo se
// maneja en RGBA. Serviría para vidrios, nubes semitransparentes o el HUD
// (blending), o para recortar formas con `discard` (alpha test).
//
// Lo que se elige acá (CPU) es independiente de cómo se GUARDA en la GPU (el
// "formato interno"): el cubemap llega en RGBA y se guarda como GL_RGB8, sin
// alfa (ver Texture::load_cubemap).
struct ImageData {
    int width    = 0;
    int height   = 0;
    int channels = 0;                    // canales del ARCHIVO (informativo: 3 = RGB, 4 = RGBA)
    std::vector<unsigned char> pixels;   // SIEMPRE RGBA: 4 bytes por texel (ver arriba)
};

class ResourceManager {
public:
    // assets_root: carpeta raiz de los recursos (ej.: "./assets").
    // Los nombres de archivo que se pasan a load_* son rutas relativas a ella.
    // Lanza std::runtime_error si la carpeta no existe o no es un directorio.
    explicit ResourceManager(const std::filesystem::path& assets_root);

    // Lee vs_file y fs_file (y gs_file si no es vacio) desde
    // assets_root / <archivo> y guarda el resultado bajo key.
    // - Primera vez: lee del disco.
    // - Clave ya cargada con mismos archivos: devuelve el cache (sin leer).
    // - Clave ya cargada con otros archivos: avisa por stderr y devuelve
    //   el cache existente.
    // Lanza std::runtime_error si algun archivo falta o no se puede leer.
    const ShaderSource& load_shader_source(const std::string& key,
                                           const std::string& vs_file,
                                           const std::string& fs_file,
                                           const std::string& gs_file = "");

    // Devuelve el recurso asociado a key SIN tocar disco.
    // Lanza std::runtime_error si la clave no fue cargada antes.
    const ShaderSource& get_shader_source(const std::string& key) const;

    // --- Imágenes (Práctico 08) ---------------------------------------------
    // Lee y decodifica assets_root / file con stb_image (siempre a RGBA) y la
    // guarda bajo key. flip_vertically: true para texturas 2D (el (0,0) de
    // OpenGL está abajo), false para las caras de un cubemap.
    // - Clave ya cargada con el mismo archivo y flip: devuelve el caché.
    // - Clave ya cargada con OTRO archivo u otro flip: avisa y devuelve el caché.
    // Lanza std::runtime_error si el archivo no existe o no se puede decodificar.
    const ImageData& load_image(const std::string& key, const std::string& file,
                                bool flip_vertically);

    // Devuelve la imagen de key SIN tocar disco. Lanza si no fue cargada (o si
    // ya se liberó con unload_image).
    const ImageData& get_image(const std::string& key) const;

    // Libera la copia de la CPU de la imagen (la GPU ya tiene la suya). Devuelve
    // false si la clave no estaba. Las referencias a esa imagen quedan inválidas.
    bool unload_image(const std::string& key);

    // Vacia el cache (shaders E imágenes). Toda referencia devuelta antes por
    // load_/get_ queda invalida a partir de este llamado.
    void clear(void);

private:
    // Lee un solo archivo desde assets_root e informa su tipo en caso de
    // error (ej.: "vertex shader") para que el mensaje sea claro.
    // Lanza std::runtime_error si falta o falla la lectura.
    std::string read_shader_file(const std::string& shader_file,
                                 const std::string& shader_type);

    // Entrada interna del cache: ademas del fuente, recuerda QUE archivos
    // originaron la carga para poder avisar cuando una clave repetida llega
    // con archivos distintos.
    struct CachedShader {
        ShaderSource source;
        std::string vs_file;
        std::string fs_file;
        std::string gs_file;
    };

    // Entrada del caché de imágenes: la imagen + con qué archivo y qué flip se
    // cargó (para avisar si la misma clave llega con otros datos).
    struct CachedImage {
        ImageData image;
        std::string file;
        bool flip_vertically = false;
    };

    std::filesystem::path assets_root_;
    std::unordered_map<std::string, CachedShader> shaders_sources_;
    std::unordered_map<std::string, CachedImage> images_;
};
