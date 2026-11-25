// Sin #version: desde el Práctico 09 la agrega la clase Shader a todas las etapas (ver Shader.cpp).

// -----------------------------------------------------------------------------
// aeronave.vs — vertex shader de las PIEZAS DEL AVIÓN (Prácticos 08 y 09)
//
// Es el antiguo solid.vs (Prácticos 03-07) sin el morph del terreno (que pasó a
// terreno.vs) y con las coordenadas de textura que, por primera vez, se usan:
// la Parte 3 de la guía pide VERIFICARLAS aplicando una imagen asimétrica.
//
// Práctico 09 (Parte 1): manda la POSICIÓN EN MUNDO (vPosMundo) en lugar de la
// distancia a la cámara, para la niebla del código común (y la luz, Parte 3).
//
// Práctico 09 (Parte 3): manda también la NORMAL EN MUNDO, transformada con la
// MATRIZ NORMAL de la pieza (uNormalMatrix), para iluminar en aeronave.fs.
// -----------------------------------------------------------------------------

// ENTRADAS (in): un valor POR VÉRTICE, leído del VBO de la malla de la pieza.
// Las location coinciden con Mesh::load (la aeronave no usa la 3 ni la 4, que
// son del geomorphing del terreno).
layout (location = 0) in vec3 aPos;         // posición del vértice en la PRIMITIVA (antes de ubicarla)
layout (location = 1) in vec3 aNormal;      // normal de la primitiva (unitaria, en SUS ejes)
layout (location = 2) in vec2 aTexCoords;   // (u, v) de la primitiva en [0, 1] (Práctico 03)

// SALIDAS (out): van, INTERPOLADAS, a aeronave.fs (mismo nombre).
out vec3  vNormalMundo; // P09: la normal en MUNDO (sin normalizar: lo hace el fragment shader)
out vec2  vTexCoord;    // (u, v) ya multiplicadas por la escala de la pieza
out vec3  vPosMundo;    // P09: posición en MUNDO (origen flotante: relativa al avión)

// UNIFORMS (iguales para todos los vértices de la pieza).
uniform mat4 uModel;        // matriz de modelo de la pieza (pose * local)
uniform mat3 uNormalMatrix; // P09: matriz NORMAL de la pieza, G = (M^-1)^T (la calcula la CPU)
uniform mat4 uView;         // vista de la cámara (glm::lookAt)
uniform mat4 uProjection;   // proyección en perspectiva
uniform vec2 uEscalaUV;     // P08: repeticiones de la textura sobre la pieza (1,1 = una copia)

void main()
{
    // LA NORMAL A MUNDO (Parte 3). No se puede usar la misma matriz que la
    // posición: con un escalado NO uniforme (los conos de la nariz y la cola
    // son un cono unitario estirado), M·n deja de ser perpendicular a la
    // superficie. La que conserva la perpendicularidad es la matriz normal
    // G = (M^-1)^T, de la parte 3x3 de M (la traslación no afecta a una
    // dirección). Se calcula UNA vez por pieza en la CPU (main.cpp,
    // dibujar_items) y no acá: invertir una matriz en CADA vértice sería
    // repetir la misma cuenta cientos de veces.
    // Con la tecla X la CPU manda mat3(M) en lugar de G, para ver el error.
    // No se normaliza acá: la interpolación la acorta igual y la normaliza
    // aeronave.fs.
    vNormalMundo = uNormalMatrix * aNormal;

    // Repetir la textura: con escala 2, las (u, v) van de 0 a 2 y GL_REPEAT
    // dibuja dos copias sobre la pieza.
    vTexCoord = aTexCoords * uEscalaUV;
    // posMundo = el vértice después de la matriz de modelo de la pieza (pose
    // del avión * ubicación de la pieza). Mismo sistema que uOjo y el sol.
    vec4 posMundo = uModel * vec4(aPos, 1.0);
    vPosMundo     = posMundo.xyz;
    gl_Position   = uProjection * uView * posMundo;   // salida obligatoria: posición en recorte
}
