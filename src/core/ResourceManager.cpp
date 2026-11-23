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

namespace {

// Une una lista de archivos en un solo texto legible, para los mensajes de
// aviso: {"a.glsl", "b.fs"} -> "a.glsl + b.fs".
std::string unir_nombres(const std::vector<std::string>& files)
{
    std::string texto;
    for (std::size_t i = 0; i < files.size(); ++i) {
        texto += (i == 0U ? "" : " + ") + files[i];
    }
    return texto;
}

}   // namespace (anónimo)

const ShaderSource& ResourceManager::load_shader_source(const std::string& key,
                                                        const std::vector<std::string>& vs_files,
                                                        const std::vector<std::string>& fs_files,
                                                        const std::vector<std::string>& gs_files)
{
    // 1) ¿Ya está en el caché? Se compara la LISTA completa de cada etapa
    //    (mismos archivos y en el mismo orden: el orden cambia el texto que se
    //    compila). Igual que antes: mismo pedido -> caché; otro -> aviso y gana
    //    la primera carga (decisión 2).
    const auto it = shaders_sources_.find(key);
    if (it != shaders_sources_.end()) {
        const ShaderSource& cached = it->second;

        if (cached.vs.files == vs_files && cached.fs.files == fs_files &&
            cached.gs.files == gs_files) {
            return cached;   // pedido repetido: cache, sin leer disco
        }

        std::cerr << "[ResourceManager] aviso: la clave '" << key
                  << "' ya estaba cargada con otros archivos; se conserva "
                  << "la copia en cache y se ignora ('" << unir_nombres(vs_files)
                  << "', '" << unir_nombres(fs_files) << "')" << std::endl;
        return cached;
    }

    // 2) El vertex y el fragment son obligatorios: una lista vacía es un error
    //    del llamador y se informa con una excepción (decisión 1).
    if (vs_files.empty() || fs_files.empty()) {
        throw std::runtime_error("ResourceManager: la clave '" + key +
                                 "' necesita al menos un archivo de vertex shader "
                                 "y uno de fragment shader");
    }

    // 3) Leer cada etapa (si un archivo falta, lanza y no se guarda nada).
    ShaderSource entry;
    entry.vs = read_shader_stage(vs_files, "vertex shader");
    entry.fs = read_shader_stage(fs_files, "fragment shader");
    entry.gs = read_shader_stage(gs_files, "geometry shader");   // vacía si no hay

    return shaders_sources_.emplace(key, std::move(entry)).first->second;
}

const ShaderSource& ResourceManager::get_shader_source(const std::string& key) const
{
    const auto it = shaders_sources_.find(key);
    if (it == shaders_sources_.end()) {
        throw std::runtime_error("ResourceManager: se pidió la clave '" + key +
                                 "' pero nunca fue cargada");
    }
    return it->second;
}

// Práctico 09: lee los archivos de UNA etapa, en el orden de la lista. Cada uno
// queda como un texto aparte (no se concatenan acá: lo hace la GPU al compilar).
ShaderStageSource ResourceManager::read_shader_stage(const std::vector<std::string>& files,
                                                     const std::string& shader_type)
{
    ShaderStageSource stage;
    for (const std::string& file : files) {
        std::string texto = read_shader_file(file, shader_type);
        // glShaderSource pega las cadenas TAL CUAL, sin agregar nada entre una
        // y otra. Si un archivo no termina en salto de línea, su última línea
        // quedaría pegada a la primera del archivo siguiente (por ejemplo, un
        // comentario // final se "comería" la primera línea del otro). Se
        // asegura un '\n' al final de cada uno.
        if (!texto.empty() && texto.back() != '\n') {
            texto += '\n';
        }
        stage.files.push_back(file);
        stage.sources.push_back(std::move(texto));
    }
    return stage;
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
