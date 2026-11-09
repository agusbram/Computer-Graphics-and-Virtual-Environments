#version 460 core

// -----------------------------------------------------------------------------
// terreno.vs — vertex shader del TERRENO (Práctico 08)
//
// Hasta el Práctico 07 el terreno y el avión compartían solid.vs. Desde el
// Práctico 08 cada tipo de objeto tiene su PROPIO programa (terreno, aeronave y
// fondo), como proponen las guías 08 y 09: el terreno necesita el morph del LOD
// y la escala de la textura; el avión no.
//
// Atributos (armados por Mesh::load):
//   location 0 -> aPos       posición LOCAL del vértice en su nivel del clipmap
//   location 1 -> aNormal    normal (todavía sin usar: la usa la luz, Práctico 09)
//   location 2 -> aTexCoords posición en el MUNDO (x, y) módulo Terrain::kPeriodoUV,
//                            en unidades de escena (ver Terrain::recalcular_vertices)
//   location 3 -> aZc        altura del nivel GRUESO (geomorphing del LOD)
// -----------------------------------------------------------------------------

// ENTRADAS (in): un valor POR VÉRTICE, leído del VBO de la malla. El número de
// location tiene que coincidir con el que configuró Mesh::load
// (glVertexArrayAttribFormat).
layout (location = 0) in vec3 aPos;          // posición LOCAL del vértice (relativa a la esquina de su nivel)
layout (location = 1) in vec3 aNormal;       // normal de la superficie (sin usar hasta la luz del P09)
layout (location = 2) in vec2 aTexCoords;    // posición (x, y) en el MUNDO módulo kPeriodoUV, unidades de escena
layout (location = 3) in float aZc;          // altura que tendría este punto en el nivel GRUESO (geomorphing)

// SALIDAS (out): van al fragment shader. La GPU las INTERPOLA entre los 3
// vértices de cada triángulo; terreno.fs las recibe con el MISMO nombre (in).
out vec3  vNormal;      // la normal, sin cambios (la usará la luz del P09)
out vec2  vTexCoord;    // P08: coordenadas de textura, en TESELAS (1 unidad = una copia del pasto)
out float vDistancia;   // distancia del vértice a la cámara (para la niebla)

// UNIFORMS: un valor para TODO el dibujo (igual en todos los vértices). Los
// manda C++ con Shader::set_uniform antes de dibujar.

uniform mat4 uModel;       // matriz de modelo del nivel: translate(esquina - avión)
uniform mat4 uView;        // vista de la cámara (glm::lookAt)
uniform mat4 uProjection;  // proyección en perspectiva

// LOD (Práctico 07): uMorph = (paso del nivel, N/2 en celdas, ancho w en celdas).
// uMorph.x == 0 -> sin morph.
uniform vec3 uMorph;

// Práctico 08: tamaño de UNA tesela (una copia de la textura) en unidades de
// escena. Es el "factor de repetición" de la guía (uTeselas) expresado al revés:
// en lugar de "cuántas copias entran en la malla" se dice "cuánto mide una".
// Con 5 u = 50 m, el pasto se repite cada 50 m (la referencia de la guía).
uniform float uTamTesela;

void main()
{
    vNormal = aNormal;
    vec3 pos = aPos;    // copia modificable de la posición (el morph le cambia la z)

    // --- Geomorphing del LOD (Práctico 07; explicado en docs-clipmaps) -------
    if (uMorph.x > 0.0) {
        vec2 viewer = -uModel[3].xy;                       // el avión, en local
        vec2 d = abs(pos.xy - viewer) / uMorph.x;          // distancia en celdas
        vec2 a = clamp((d - (uMorph.y - uMorph.z - 1.0)) / uMorph.z, 0.0, 1.0);
        pos.z = mix(aPos.z, aZc, max(a.x, a.y));           // z' = (1-α)z + α·z_c
    }

    // --- Coordenadas de textura (Práctico 08) --------------------------------
    // aTexCoords es la posición en el mundo (unidades de escena); dividida por
    // el tamaño de la tesela queda en TESELAS: 0..1 = una copia, 1..2 = la
    // siguiente... Las que pasan de 1 las resuelve GL_REPEAT (se usa la parte
    // fraccionaria). El morph no las cambia: solo mueve z, no (x, y).
    vTexCoord = aTexCoords / uTamTesela;

    // --- Posición: P · V · M (en dos pasos para la niebla) -------------------
    // posVista = el vértice en el sistema de la CÁMARA (después de M y V).
    vec4 posVista = uView * uModel * vec4(pos, 1.0);
    vDistancia    = length(posVista.xyz);   // la cámara está en el origen de la vista
    gl_Position   = uProjection * posVista;   // salida obligatoria: posición en recorte
}
