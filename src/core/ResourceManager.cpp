// -----------------------------------------------------------------------------
// ResourceManager.cpp — implementacion (C++ puro, sin OpenGL)
// -----------------------------------------------------------------------------

#include "core/ResourceManager.h"  // la declaración de la clase que se implementa acá

#include <fstream>                 // std::ifstream: abrir y leer los archivos de los shaders
#include <iostream>                // std::cerr: avisos (misma clave cargada con otro archivo o flip)
#include <sstream>                 // std::stringstream: leer un archivo entero a un std::string
#include <stdexcept>               // std::runtime_error: errores (archivo inexistente, imagen inválida)

// -----------------------------------------------------------------------------
// stb_image (Práctico 08): biblioteca de UN SOLO encabezado para decodificar
// imágenes. El #define STB_IMAGE_IMPLEMENTATION hace que el encabezado incluya
// también el CÓDIGO de las funciones (no solo sus declaraciones), por eso tiene
// que estar en UN SOLO .cpp de todo el proyecto: si estuviera en dos, el
// linkeador vería las funciones definidas dos veces (símbolos duplicados).
// ResourceManager es el lugar natural: es el único módulo que lee archivos.
//   - STBI_ONLY_PNG / STBI_ONLY_JPEG: solo se compilan los decodificadores de
//     los formatos que usamos (pasto/cielo en PNG, chapa del avión en JPG).
//   - STBI_FAILURE_USERMSG: stbi_failure_reason() da mensajes legibles.
//   - Los pragmas silencian las advertencias de -Wall -Wextra que genera el
//     código de la biblioteca (es código externo: no lo vamos a modificar).
// -----------------------------------------------------------------------------
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_FAILURE_USERMSG
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif
#include "stb_image.h"             // stb_image: decodifica PNG y JPG a bytes (stbi_load, stbi_image_free)
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

ResourceManager::ResourceManager(const std::filesystem::path& assets_root)
    : assets_root_(assets_root)
{
    if (!std::filesystem::exists(assets_root_)) {
        throw std::runtime_error("ResourceManager: no existe la carpeta de "
                                 "recursos '" + assets_root_.string() + "'");
    }
    if (!std::filesystem::is_directory(assets_root_)) {
        throw std::runtime_error("ResourceManager: '" + assets_root_.string() +
                                 "' no es una carpeta");
    }
}

const ShaderSource& ResourceManager::load_shader_source(const std::string& key,
                                                        const std::string& vs_file,
                                                        const std::string& fs_file,
                                                        const std::string& gs_file)
{
    const auto it = shaders_sources_.find(key);
    if (it != shaders_sources_.end()) {
        const CachedShader& cached = it->second;

        if (cached.vs_file == vs_file && cached.fs_file == fs_file &&
            cached.gs_file == gs_file) {
            return cached.source;   // pedido repetido: cache, sin leer disco
        }

        std::cerr << "[ResourceManager] aviso: la clave '" << key
                  << "' ya estaba cargada con otros archivos; se conserva "
                  << "la copia en cache y se ignora ('" << vs_file << "', '"
                  << fs_file << "')" << std::endl;
        return cached.source;
    }

    CachedShader entry;
    entry.vs_file = vs_file;
    entry.fs_file = fs_file;
    entry.gs_file = gs_file;
    entry.source.vs = read_shader_file(vs_file, "vertex shader");
    entry.source.fs = read_shader_file(fs_file, "fragment shader");
    if (!gs_file.empty()) {
        entry.source.gs = read_shader_file(gs_file, "geometry shader");
    }

    return shaders_sources_.emplace(key, std::move(entry)).first->second.source;
}

const ShaderSource& ResourceManager::get_shader_source(const std::string& key) const
{
    const auto it = shaders_sources_.find(key);
    if (it == shaders_sources_.end()) {
        throw std::runtime_error("ResourceManager: se pidió la clave '" + key +
                                 "' pero nunca fue cargada");
    }
    return it->second.source;
}

// -----------------------------------------------------------------------------
// Imágenes (Práctico 08)
// -----------------------------------------------------------------------------
const ImageData& ResourceManager::load_image(const std::string& key,
                                             const std::string& file,
                                             bool flip_vertically)
{
    // 1) ¿Ya está en el caché? Mismo archivo y mismo flip -> se devuelve sin
    //    leer el disco. Si difiere, se avisa y gana la primera carga (decisión 7).
    const auto it = images_.find(key);
    if (it != images_.end()) {
        const CachedImage& cached = it->second;
        if (cached.file != file || cached.flip_vertically != flip_vertically) {
            std::cerr << "[ResourceManager] aviso: la imagen '" << key
                      << "' ya estaba cargada desde '" << cached.file
                      << "' (flip=" << cached.flip_vertically << "); se conserva "
                      << "la copia en cache y se ignora ('" << file << "', flip="
                      << flip_vertically << ")" << std::endl;
        }
        return cached.image;
    }

    // 2) Validar el archivo (mensajes claros, igual que con los shaders).
    const std::filesystem::path full_path = assets_root_ / file;
    if (!std::filesystem::is_regular_file(full_path)) {
        throw std::runtime_error("ResourceManager: no existe la imagen '" +
                                 full_path.string() + "'");
    }

    // 3) Decodificar con stb_image. El flip es un ESTADO GLOBAL de stb: se fija
    //    en CADA carga (decisión 6), si no quedaría el de la carga anterior.
    //    El último argumento (4) fuerza RGBA, sin importar los canales del archivo
    //    (a un RGB le agrega alfa = 255). En `canales` devuelve los que tenía el
    //    ARCHIVO, que se guardan solo como dato (ver ImageData en el .h).
    stbi_set_flip_vertically_on_load(flip_vertically ? 1 : 0);
    int w = 0, h = 0, canales = 0;
    unsigned char* datos = stbi_load(full_path.string().c_str(), &w, &h, &canales, 4);
    if (datos == nullptr) {
        throw std::runtime_error("ResourceManager: no se pudo decodificar '" +
                                 full_path.string() + "': " + stbi_failure_reason());
    }

    // 4) Copiar a un vector (memoria administrada por C++) y liberar la de stb:
    //    stbi_load reserva con malloc y hay que devolverla con stbi_image_free.
    CachedImage entry;
    entry.file = file;
    entry.flip_vertically = flip_vertically;
    entry.image.width    = w;
    entry.image.height   = h;
    entry.image.channels = canales;
    entry.image.pixels.assign(datos, datos + static_cast<std::size_t>(w) * h * 4U);
    stbi_image_free(datos);

    return images_.emplace(key, std::move(entry)).first->second.image;
}

const ImageData& ResourceManager::get_image(const std::string& key) const
{
    const auto it = images_.find(key);
    if (it == images_.end()) {
        throw std::runtime_error("ResourceManager: se pidió la imagen '" + key +
                                 "' pero no está cargada (¿nunca se cargó o ya "
                                 "se liberó con unload_image?)");
    }
    return it->second.image;
}

bool ResourceManager::unload_image(const std::string& key)
{
    // erase devuelve cuántos elementos borró (0 o 1).
    return images_.erase(key) > 0U;
}

void ResourceManager::clear(void)
{
    shaders_sources_.clear();
    images_.clear();
}

std::string ResourceManager::read_shader_file(const std::string& shader_file,
                                              const std::string& shader_type)
{
    const std::filesystem::path full_path = assets_root_ / shader_file;

    if (!std::filesystem::exists(full_path)) {
        throw std::runtime_error("ResourceManager: no existe el archivo '" +
                                 full_path.string() + "' (" + shader_type + ")");
    }
    if (!std::filesystem::is_regular_file(full_path)) {
        throw std::runtime_error("ResourceManager: '" + full_path.string() +
                                 "' no es un archivo regular (" + shader_type + ")");
    }

    std::ifstream in(full_path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("ResourceManager: no se pudo abrir '" +
                                 full_path.string() + "' (" + shader_type + ")");
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    if (in.bad() || buffer.fail()) {
        throw std::runtime_error("ResourceManager: error de lectura de '" +
                                 full_path.string() + "' (" + shader_type + ")");
    }
    return buffer.str();
}
