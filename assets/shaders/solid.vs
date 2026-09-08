#version 460 core

// -----------------------------------------------------------------------------
// solid.vs — vertex shader (Práctico 03)
//
// El formato de vertice ahora tiene TRES atributos (armados por Mesh::load):
//   location 0 -> aPos       (vec3, posicion)
//   location 1 -> aNormal    (vec3, normal de la superficie)
//   location 2 -> aTexCoords (vec2, coordenadas de textura; hoy sin usar)
//
// La normal se guarda y se copia a la siguiente etapa, pero todavia no la
// consume nadie: la usa la iluminacion (Unidad IX). Las coordenadas de
// textura se guardan pero tampoco se usan aun.
// -----------------------------------------------------------------------------

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 vNormal;

uniform mat4 uModel;    // matriz de MODELO de la pieza actual (una por objeto)
uniform mat4 uAjuste;   // caja negra: ajuste de vista y proyeccion (Unidad VII)

void main()
{
    vNormal = aNormal;
    // En uModel * vec4(aPos, 1.0) se aplica PRIMERO uModel (el vertice esta
    // en coordenadas del modelo) y despues uAjuste (lo lleva a pantalla).
    gl_Position = uAjuste * uModel * vec4(aPos, 1.0);
}