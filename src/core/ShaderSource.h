// -----------------------------------------------------------------------------
// ShaderSource.h — el CÓDIGO FUENTE de un programa de shaders, en datos planos
//
// Lo LLENA el ResourceManager (lee los archivos del disco) y lo CONSUME Shader
// (lo compila). Es la misma separación que MeshData entre Primitives y Mesh:
// un struct de solo datos en el medio, para que ninguno de los dos módulos
// dependa del otro. (Hasta el Práctico 08 este struct vivía dentro de
// ResourceManager.h y tenía un solo texto por etapa.)
//
// Práctico 09 (Parte 1) — VARIOS ARCHIVOS POR ETAPA.
// GLSL no tiene #include, pero glShaderSource recibe un ARREGLO de cadenas y
// las compila como si fueran un único texto, una a continuación de la otra.
// Eso permite escribir el código común (iluminacion.glsl) UNA sola vez y
// pegarlo delante del código propio de cada fragment shader. Por eso cada
// etapa guarda una LISTA de archivos, en el orden en que se pegan:
//
//   fragment del terreno:  files   = { "shaders/iluminacion.glsl", "shaders/terreno.fs" }
//                          sources = { <texto de iluminacion.glsl>, <texto de terreno.fs> }
//
// Decisión (Práctico 09, pregunta 1): una lista de archivos por etapa, escrita
// en main al cargar cada programa (y no un archivo común fijo que se agrega
// siempre, ni un #include casero). Así se ve, en un solo lugar, qué código
// arma cada programa.
//
// Los archivos NO tienen la línea #version: la agrega Shader al compilar,
// porque tiene que ser la PRIMERA línea del texto completo (pregunta 2).
// -----------------------------------------------------------------------------

#pragma once

#include <string>   // std::string: rutas y contenido de cada archivo
#include <vector>   // std::vector: la lista de archivos de una etapa

// Fuente de UNA etapa (vertex, fragment o geometry): uno o más archivos.
struct ShaderStageSource {
    // Rutas relativas a assets/ (por ejemplo "shaders/terreno.fs"). Shader las
    // usa en el mensaje de error, para decir QUÉ archivo falló.
    std::vector<std::string> files;
    // Contenido de cada archivo, en el MISMO orden que `files`.
    std::vector<std::string> sources;

    bool empty() const { return sources.empty(); }
};

// Fuente de un programa completo: una entrada por etapa.
struct ShaderSource {
    ShaderStageSource vs;   // vertex shader (obligatorio)
    ShaderStageSource fs;   // fragment shader (obligatorio)
    ShaderStageSource gs;   // geometry shader (opcional; ningún programa lo usa todavía)
};
