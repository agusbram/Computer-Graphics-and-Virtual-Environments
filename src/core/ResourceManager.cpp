// -----------------------------------------------------------------------------
// ResourceManager.cpp — implementacion (C++ puro, sin OpenGL)
// -----------------------------------------------------------------------------

#include "core/ResourceManager.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

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

void ResourceManager::clear(void)
{
    shaders_sources_.clear();
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
