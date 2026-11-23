// Sin #version: desde el Práctico 09 la agrega la clase Shader a todas las etapas (ver Shader.cpp).

// -----------------------------------------------------------------------------
// terreno.vs — vertex shader del TERRENO (Prácticos 08 y 09)
//
// Hasta el Práctico 07 el terreno y el avión compartían solid.vs. Desde el
// Práctico 08 cada tipo de objeto tiene su PROPIO programa (terreno, aeronave y
// fondo), como proponen las guías 08 y 09: el terreno necesita el morph del LOD
// y la escala de la textura; el avión no.
//
// Práctico 09 (Parte 1): en lugar de la distancia a la cámara (vDistancia), el
// vertex shader manda la POSICIÓN EN MUNDO del vértice (vPosMundo). Con ella el
// fragment shader calcula la niebla (aplicar_niebla, código común) y, desde la
// Parte 2, la iluminación.
//
// Práctico 09 (Parte 2): manda también la NORMAL EN MUNDO (vNormalMundo), y el
// geomorphing ahora mueve la normal además de la altura (ver main()).
//
// Atributos (armados por Mesh::load):
//   location 0 -> aPos          posición LOCAL del vértice en su nivel del clipmap
//   location 1 -> aNormal       normal de ESTE nivel (diferencias centrales, Terrain.cpp)
//   location 2 -> aTexCoords    posición en el MUNDO (x, y) módulo Terrain::kPeriodoUV,
//                               en unidades de escena (ver Terrain::recalcular_vertices)
//   location 3 -> aZc           altura del nivel GRUESO (geomorphing del LOD)
//   location 4 -> aNormalGruesa normal del nivel GRUESO (P09: geomorphing de la normal)
// -----------------------------------------------------------------------------

// ENTRADAS (in): un valor POR VÉRTICE, leído del VBO de la malla. El número de
// location tiene que coincidir con el que configuró Mesh::load
// (glVertexArrayAttribFormat).
layout (location = 0) in vec3 aPos;          // posición LOCAL del vértice (relativa a la esquina de su nivel)
layout (location = 1) in vec3 aNormal;       // normal de la superficie en ESTE nivel (unitaria)
layout (location = 2) in vec2 aTexCoords;    // posición (x, y) en el MUNDO módulo kPeriodoUV, unidades de escena
layout (location = 3) in float aZc;          // altura que tendría este punto en el nivel GRUESO (geomorphing)
layout (location = 4) in vec3 aNormalGruesa; // P09: normal que tendría este punto en el nivel GRUESO (unitaria)

// SALIDAS (out): van al fragment shader. La GPU las INTERPOLA entre los 3
// vértices de cada triángulo; terreno.fs las recibe con el MISMO nombre (in).
out vec3  vNormalMundo; // P09: normal en MUNDO (mezclada con la del nivel grueso), para la luz
out vec2  vTexCoord;    // P08: coordenadas de textura, en TESELAS (1 unidad = una copia del pasto)
out vec3  vPosMundo;    // P09: posición del vértice en MUNDO (origen flotante: relativa al avión)

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
    vec3  pos  = aPos;   // copia modificable de la posición (el morph le cambia la z)
    float alfa = 0.0;    // α del morph: 0 = forma de ESTE nivel, 1 = forma del nivel grueso

    // --- Geomorphing del LOD (Práctico 07; explicado en docs-clipmaps) -------
    if (uMorph.x > 0.0) {
        vec2 viewer = -uModel[3].xy;                       // el avión, en local
        vec2 d = abs(pos.xy - viewer) / uMorph.x;          // distancia en celdas
        vec2 a = clamp((d - (uMorph.y - uMorph.z - 1.0)) / uMorph.z, 0.0, 1.0);
        alfa  = max(a.x, a.y);
        pos.z = mix(aPos.z, aZc, alfa);                    // z' = (1-α)z + α·z_c
    }

    // --- Normal (Práctico 09, Parte 2) ----------------------------------------
    // 1) GEOMORPHING DE LA NORMAL. El morph de arriba lleva la FORMA de este
    //    nivel a la del nivel grueso cerca del borde del anillo (α = 1 en el
    //    borde). Si la normal no cambiara con ella, en el borde la malla fina
    //    tendría la forma del grueso pero las normales del fino, y el grueso, del
    //    otro lado del borde, las suyas: la LUZ saltaría ahí y se vería el borde
    //    de cada anillo. Por eso la normal se mezcla con el MISMO α:
    //        n' = (1-α)·n + α·n_c
    //    (n_c = la normal del nivel grueso en este punto, Terrain.cpp). La
    //    mezcla de dos vectores unitarios no es unitaria: la normaliza el
    //    fragment shader (igual tiene que hacerlo, por la interpolación).
    vec3 n = mix(aNormal, aNormalGruesa, alfa);

    // 2) DE LOCAL A MUNDO. Una normal NO se transforma con la matriz de modelo
    //    M sino con la MATRIZ NORMAL, transpose(inverse(mat3(M))) (Parte 3:
    //    con escalados distintos por eje, M torcería la normal). Pero el modelo
    //    del terreno es SOLO UNA TRASLACIÓN (la esquina del nivel menos el
    //    avión) y una normal es una DIRECCIÓN: las traslaciones no la afectan
    //    (la parte 3x3 de M es la identidad, y su matriz normal también). Por
    //    eso, en el terreno, la normal local YA ES la normal en mundo.
    //    (Decisión 8: el terreno no usa matriz normal.)
    vNormalMundo = n;

    // --- Coordenadas de textura (Práctico 08) --------------------------------
    // aTexCoords es la posición en el mundo (unidades de escena); dividida por
    // el tamaño de la tesela queda en TESELAS: 0..1 = una copia, 1..2 = la
    // siguiente... Las que pasan de 1 las resuelve GL_REPEAT (se usa la parte
    // fraccionaria). El morph no las cambia: solo mueve z, no (x, y).
    vTexCoord = aTexCoords / uTamTesela;

    // --- Posición: P · V · M (en dos pasos: el primero da la posición en mundo)
    // posMundo = el vértice después de la matriz de modelo. "Mundo" acá es el
    // sistema con ORIGEN FLOTANTE (el avión en el origen): uModel traslada el
    // nivel a (esquina - avión). Es el mismo sistema del ojo (uOjo) y del sol.
    vec4 posMundo = uModel * vec4(pos, 1.0);
    vPosMundo     = posMundo.xyz;
    gl_Position   = uProjection * uView * posMundo;   // salida obligatoria: posición en recorte
}
