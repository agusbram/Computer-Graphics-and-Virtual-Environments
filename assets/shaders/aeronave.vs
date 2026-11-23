// Sin #version: desde el Práctico 09 la agrega la clase Shader a todas las etapas (ver Shader.cpp).

// -----------------------------------------------------------------------------
// aeronave.vs — vertex shader de las PIEZAS DEL AVIÓN (Práctico 08)
//
// Es el antiguo solid.vs (Prácticos 03-07) sin el morph del terreno (que pasó a
// terreno.vs) y con las coordenadas de textura que, por primera vez, se usan:
// la Parte 3 de la guía pide VERIFICARLAS aplicando una imagen asimétrica.
//
// Práctico 09 (Parte 1): manda la POSICIÓN EN MUNDO (vPosMundo) en lugar de la
// distancia a la cámara, para la niebla del código común (y la luz, Parte 3).
// -----------------------------------------------------------------------------

// ENTRADAS (in): un valor POR VÉRTICE, leído del VBO de la malla de la pieza.
// Las location coinciden con Mesh::load (la aeronave no usa la 3, aZc).
layout (location = 0) in vec3 aPos;         // posición del vértice en la PRIMITIVA (antes de ubicarla)
layout (location = 1) in vec3 aNormal;      // normal de la primitiva (sin usar hasta la luz del P09)
layout (location = 2) in vec2 aTexCoords;   // (u, v) de la primitiva en [0, 1] (Práctico 03)

// SALIDAS (out): van, INTERPOLADAS, a aeronave.fs (mismo nombre).
out vec3  vNormal;      // la normal, sin cambios (la luz la usa desde la Parte 3)
out vec2  vTexCoord;    // (u, v) ya multiplicadas por la escala de la pieza
out vec3  vPosMundo;    // P09: posición en MUNDO (origen flotante: relativa al avión)

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
    // posMundo = el vértice después de la matriz de modelo de la pieza (pose
    // del avión * ubicación de la pieza). Mismo sistema que uOjo y el sol.
    vec4 posMundo = uModel * vec4(aPos, 1.0);
    vPosMundo     = posMundo.xyz;
    gl_Position   = uProjection * uView * posMundo;   // salida obligatoria: posición en recorte
}
