#version 460 core

// -----------------------------------------------------------------------------
// aeronave.vs — vertex shader de las PIEZAS DEL AVIÓN (Práctico 08)
//
// Es el antiguo solid.vs (Prácticos 03-07) sin el morph del terreno (que pasó a
// terreno.vs) y con las coordenadas de textura que, por primera vez, se usan:
// la Parte 3 de la guía pide VERIFICARLAS aplicando una imagen asimétrica.
// -----------------------------------------------------------------------------

// ENTRADAS (in): un valor POR VÉRTICE, leído del VBO de la malla de la pieza.
// Las location coinciden con Mesh::load (la aeronave no usa la 3, aZc).
layout (location = 0) in vec3 aPos;         // posición del vértice en la PRIMITIVA (antes de ubicarla)
layout (location = 1) in vec3 aNormal;      // normal de la primitiva (sin usar hasta la luz del P09)
layout (location = 2) in vec2 aTexCoords;   // (u, v) de la primitiva en [0, 1] (Práctico 03)

// SALIDAS (out): van, INTERPOLADAS, a aeronave.fs (mismo nombre).
out vec3  vNormal;      // la normal, sin cambios
out vec2  vTexCoord;    // (u, v) ya multiplicadas por la escala de la pieza
out float vDistancia;   // distancia a la cámara (niebla)

// UNIFORMS (iguales para todos los vértices de la pieza).
uniform mat4 uModel;       // matriz de modelo de la pieza (pose * local)
uniform mat4 uView;        // vista de la cámara (glm::lookAt)
uniform mat4 uProjection;  // proyección en perspectiva
uniform vec2 uEscalaUV;    // P08: repeticiones de la textura sobre la pieza (1,1 = una copia)

void main()
{
    vNormal   = aNormal;
    // Repetir la textura: con escala 2, las (u, v) van de 0 a 2 y GL_REPEAT
    // dibuja dos copias sobre la pieza.
    vTexCoord = aTexCoords * uEscalaUV;
    // posVista = el vértice en el sistema de la CÁMARA (después de M y V); la
    // cámara está en el origen, así que su largo es la distancia a ella.
    vec4 posVista = uView * uModel * vec4(aPos, 1.0);
    vDistancia    = length(posVista.xyz);
    gl_Position   = uProjection * posVista;   // salida obligatoria: posición en recorte
}
