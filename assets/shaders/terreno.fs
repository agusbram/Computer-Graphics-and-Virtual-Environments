#version 460 core

// -----------------------------------------------------------------------------
// terreno.fs — fragment shader del TERRENO (Práctico 08)
//
// Con una TEXTURA el color deja de ser uno por objeto (o uno por vértice
// interpolado) y pasa a decidirse PUNTO POR PUNTO: cada fragmento lee la imagen
// en sus coordenadas (u, v) interpoladas. En este práctico la textura
// REEMPLAZA al color; en el Práctico 09 pasa a ser el coeficiente kd de la luz.
// -----------------------------------------------------------------------------

// ENTRADAS (in): las salidas de terreno.vs, INTERPOLADAS por la GPU para este
// fragmento (mismo nombre y tipo que los out del vertex shader).
in vec3  vNormal;      // normal interpolada (sin usar hasta la luz del P09)
in vec2  vTexCoord;    // coordenadas de textura, en teselas
in float vDistancia;   // distancia a la cámara (niebla)

// SALIDA (out): el color final de este píxel (R, G, B, A en [0, 1]).
out vec4 FragColor;

// El sampler lee la UNIDAD de textura 0 (layout binding): del lado de C++
// alcanza con textura.bind(0) antes de dibujar, sin setear este uniform. Su
// valor es el NÚMERO DE UNIDAD, no el identificador de la textura.
layout (binding = 0) uniform sampler2D uTextura;

uniform vec3 uColor;         // color plano (depuración: colores por nivel, tecla L)
uniform int  uUsarTextura;   // 1 = textura (pasto), 0 = color plano

// Niebla LINEAL (mejora leve del Práctico 07). El color de la niebla es ahora el
// PROMEDIO DEL HORIZONTE del cielo (lo calcula Skybox al cargar el cubemap).
// TEMPORAL: la misma función está repetida en fondo.fs (el suelo continuado
// necesita EXACTAMENTE la misma niebla). En el Práctico 09 (Parte 1) pasa a un
// archivo GLSL común (iluminacion.glsl) y se vuelve exponencial.
uniform vec3  uNieblaColor;  // color de la niebla (= horizonte del cielo)
uniform float uNieblaInicio; // distancia donde EMPIEZA (antes: sin niebla), unidades de escena
uniform float uNieblaFin;    // distancia donde es TOTAL; <= 0 -> niebla apagada (tecla N)

vec3 aplicar_niebla(vec3 color, float distancia)
{
    if (uNieblaFin <= 0.0) {
        return color;
    }
    // f = cuánta niebla: 0 antes de uNieblaInicio, 1 desde uNieblaFin, lineal
    // en el medio. mix(a, b, f) = a·(1-f) + b·f.
    float f = clamp((distancia - uNieblaInicio) / (uNieblaFin - uNieblaInicio), 0.0, 1.0);
    return mix(color, uNieblaColor, f);
}

void main()
{
    // La textura se lee SIEMPRE (también con color plano) y recién después se
    // elige: el filtrado con mipmaps necesita comparar las coordenadas de
    // fragmentos vecinos, y eso no está definido si la lectura queda adentro de
    // un if que algunos fragmentos toman y otros no.
    vec3 texel = texture(uTextura, vTexCoord).rgb;              // color del pasto en este punto
    vec3 color = (uUsarTextura == 1) ? texel : uColor;          // el color elegido, sin niebla

    FragColor = vec4(aplicar_niebla(color, vDistancia), 1.0);
}
