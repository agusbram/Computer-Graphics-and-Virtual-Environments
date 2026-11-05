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
layout (location = 3) in float aZc;   // Práctico 07, LOD: altura en el nivel GRUESO

out vec3 vNormal;
out float vDistancia;   // Práctico 07, mejora leve (niebla; se mejora en P09): distancia del vértice a la CÁMARA

uniform mat4 uModel;       // matriz de MODELO de la pieza actual (una por objeto)
uniform mat4 uView;        // matriz de VISTA de la cámara (glm::lookAt)
uniform mat4 uProjection;  // matriz de PROYECCIÓN en perspectiva (glm::perspective)

// Práctico 07, LOD — GEOMORPHING de los geometry clipmaps (Losasso & Hoppe, §6.2).
// uMorph = (paso del nivel, media extensión N/2 en celdas, ancho w en celdas).
// uMorph.x == 0 -> no es un nivel de terreno (o el morph está apagado).
uniform vec3 uMorph;

void main()
{
    vNormal = aNormal;
    vec3 pos = aPos;

    if (uMorph.x > 0.0) {
        // Posición del OBSERVADOR (el avión) en coordenadas LOCALES del nivel.
        // La matriz de modelo del nivel es translate(esquina - avión), así que
        // el avión está en -(traslación): la columna 3 de uModel.
        vec2 viewer = -uModel[3].xy;

        // Distancia al avión EN CELDAS del nivel, por eje (el anillo es un
        // cuadrado: se usa cada eje por separado, como en el paper).
        vec2 d = abs(pos.xy - viewer) / uMorph.x;

        // α por eje: 0 en el interior y rampa lineal hasta 1 en el borde
        // exterior. Fórmula del paper:
        //   α = clamp((d - (N/2 - w - 1)) / w, 0, 1)
        // El "-1" asegura α = 1 en el borde aunque el avión no esté justo en
        // el centro (el centro va a saltos: el avión está a <= 1 celda).
        vec2 a = clamp((d - (uMorph.y - uMorph.z - 1.0)) / uMorph.z, 0.0, 1.0);
        float alpha = max(a.x, a.y);

        // z' = (1 - α)·z + α·z_c: en el borde el nivel fino toma EXACTAMENTE
        // la forma del grueso -> no hay grietas; y como α varía continuo con
        // la posición del avión, el cambio es suave (sin popping).
        pos.z = mix(aPos.z, aZc, alpha);
    }

    // Orden P · V · M: se aplica primero uModel (más a la derecha), después
    // uView y por último uProjection.
    //
    // Práctico 07, mejora leve (NIEBLA; se mejora en el Práctico 09): se separa el producto en dos pasos para quedarse
    // con la posición en el sistema de la CÁMARA (después de V·M). Ahí la
    // cámara está en el origen, así que la distancia a ella es simplemente
    // length(posVista). Se pasa al fragment shader, que la usa para la niebla
    // (la GPU la interpola entre los vértices de cada triángulo).
    vec4 posVista = uView * uModel * vec4(pos, 1.0);
    vDistancia    = length(posVista.xyz);
    gl_Position   = uProjection * posVista;
}