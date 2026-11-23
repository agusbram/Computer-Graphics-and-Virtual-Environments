// -----------------------------------------------------------------------------
// Iluminacion.cpp — buscar y enviar los uniforms de la escena (Práctico 09)
// -----------------------------------------------------------------------------

#include "core/Iluminacion.h"   // la declaración que se implementa acá

#include <iostream>             // std::cout: el informe de qué uniforms usa cada programa
#include <utility>              // std::pair: (nombre del uniform, ubicación) para el informe

UniformsIluminacion buscar_uniforms_iluminacion(const Shader& programa,
                                                const std::string& nombre)
{
    // Los MISMOS nombres que declara iluminacion.glsl. Se buscan sin aviso
    // (false): que un programa no use alguno es normal (ver Iluminacion.h).
    UniformsIluminacion u;
    u.sol_dir       = programa.loc("uSolDir", false);
    u.sol_color     = programa.loc("uSolColor", false);
    u.ambiente      = programa.loc("uAmbiente", false);
    u.ojo           = programa.loc("uOjo", false);
    u.niebla_color  = programa.loc("uNieblaColor", false);
    u.niebla_inicio = programa.loc("uNieblaInicio", false);
    u.niebla_fin    = programa.loc("uNieblaFin", false);

    // Informe: cuántos usa este programa y cuáles no. Reemplaza al aviso de
    // loc(): sirve para notar un nombre mal escrito sin repetir siete avisos.
    const std::pair<const char*, int> todos[] = {
        {"uSolDir", u.sol_dir}, {"uSolColor", u.sol_color}, {"uAmbiente", u.ambiente},
        {"uOjo", u.ojo}, {"uNieblaColor", u.niebla_color},
        {"uNieblaInicio", u.niebla_inicio}, {"uNieblaFin", u.niebla_fin}};
    int usados = 0;
    std::string faltan;
    for (const auto& [n, ubicacion] : todos) {
        if (ubicacion != -1) {
            ++usados;
        } else {
            faltan += (faltan.empty() ? "" : ", ") + std::string(n);
        }
    }
    std::cout << "[luz]    " << nombre << ": usa " << usados << " de 7 uniforms de la escena";
    if (!faltan.empty()) {
        std::cout << " (no usa: " << faltan << ")";
    }
    std::cout << std::endl;
    return u;
}

void enviar_iluminacion(const Shader& programa, const UniformsIluminacion& u,
                        const Iluminacion& luz, const glm::vec3& ojo)
{
    // glProgramUniform* (dentro de set_uniform) no necesita el programa activo.
    // Los que valen -1 se ignoran en silencio (el programa no los usa).
    programa.set_uniform(u.sol_dir, luz.sol_dir);
    programa.set_uniform(u.sol_color, luz.sol_color);
    programa.set_uniform(u.ambiente, luz.ambiente);
    programa.set_uniform(u.ojo, ojo);
    programa.set_uniform(u.niebla_color, luz.niebla_color);
    programa.set_uniform(u.niebla_inicio, luz.niebla_inicio);
    programa.set_uniform(u.niebla_fin, luz.niebla_fin);
}
