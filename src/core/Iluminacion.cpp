// -----------------------------------------------------------------------------
// Iluminacion.cpp — buscar y enviar los uniforms de la escena (Práctico 09)
// -----------------------------------------------------------------------------

#include "core/Iluminacion.h"   // la declaración que se implementa acá

#include <cmath>                // std::atan2, std::cos, std::sin: la dirección del sol (O/P)
#include <iostream>             // std::cout: el informe de qué uniforms usa cada programa
#include <iterator>             // std::size: cuántos uniforms hay en la lista del informe
#include <utility>              // std::pair: (nombre del uniform, ubicación) para el informe

#include <glm/trigonometric.hpp>   // glm::radians: grados -> radianes

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
    u.modo_suelo    = programa.loc("uModoSuelo", false);

    // Informe: cuántos usa este programa y cuáles no. Reemplaza al aviso de
    // loc(): sirve para notar un nombre mal escrito sin repetir siete avisos.
    const std::pair<const char*, int> todos[] = {
        {"uSolDir", u.sol_dir}, {"uSolColor", u.sol_color}, {"uAmbiente", u.ambiente},
        {"uOjo", u.ojo}, {"uNieblaColor", u.niebla_color},
        {"uNieblaInicio", u.niebla_inicio}, {"uNieblaFin", u.niebla_fin},
        {"uModoSuelo", u.modo_suelo}};
    int usados = 0;
    std::string faltan;
    for (const auto& [n, ubicacion] : todos) {
        if (ubicacion != -1) {
            ++usados;
        } else {
            faltan += (faltan.empty() ? "" : ", ") + std::string(n);
        }
    }
    std::cout << "[luz]    " << nombre << ": usa " << usados << " de " << std::size(todos)
              << " uniforms de la escena";
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
    programa.set_uniform(u.modo_suelo, luz.modo_suelo);
}

glm::vec3 sol_con_elevacion(const glm::vec3& base, float elevacion_grados)
{
    // Acimut de la dirección base: el ángulo de su sombra sobre el plano
    // horizontal X-Y (atan2 da el ángulo correcto en los cuatro cuadrantes).
    const float acimut = std::atan2(base.y, base.x);
    const float e      = glm::radians(elevacion_grados);
    // Coordenadas esféricas con Z arriba: la parte horizontal mide cos(e) y se
    // reparte entre x e y según el acimut; la vertical es sin(e). El vector
    // queda unitario (cos²e·(cos²a + sin²a) + sin²e = 1).
    return glm::vec3(std::cos(e) * std::cos(acimut),
                     std::cos(e) * std::sin(acimut),
                     std::sin(e));
}
