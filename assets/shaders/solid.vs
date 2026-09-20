#version 460 core

// -----------------------------------------------------------------------------
// solid.vs — vertex shader (Práctico 05)
//
// El formato de vertice tiene TRES atributos (armados por Mesh::load):
//   location 0 -> aPos       (vec3, posicion)
//   location 1 -> aNormal    (vec3, normal de la superficie)
//   location 2 -> aTexCoords (vec2, coordenadas de textura; hoy sin usar)
//
// La normal se guarda y se copia a la siguiente etapa, pero todavia no la
// consume nadie: la usa la iluminacion (Unidad IX).
//
// Práctico 05: la "caja negra" uAjuste se parte en DOS matrices, la de vista
// y la de proyeccion (Unidad VII). El vertice cambia de sistema de referencia
// en cadena: objeto (uModel) -> mundo -> cámara (uView) -> recorte/proyeccion
// (uProjection). La división por w y el viewport los hace la GPU (etapa fija).
// -----------------------------------------------------------------------------

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 vNormal;

uniform mat4 uModel;       // matriz de MODELO de la pieza actual (una por objeto)
uniform mat4 uView;        // matriz de VISTA de la cámara (glm::lookAt)
uniform mat4 uProjection;  // matriz de PROYECCIÓN en perspectiva (glm::perspective)

void main()
{
    vNormal = aNormal;
    // Orden P · V · M: se aplica primero uModel (más a la derecha), después
    // uView y por último uProjection.
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}