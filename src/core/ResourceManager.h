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
// -----------------------------------------------------------------------------

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

// Codigo fuente de un programa de shader, tal como se leyo de disco.
struct ShaderSource {
    std::string vs;             // vertex shader
    std::string fs;             // fragment shader
    std::string gs;             // geometry shader (opcional, hoy no se usa)
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

    // Vacia el cache. Toda referencia devuelta antes por load_/get_ queda
    // invalida a partir de este llamado.
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

    std::filesystem::path assets_root_;
    std::unordered_map<std::string, CachedShader> shaders_sources_;
};
