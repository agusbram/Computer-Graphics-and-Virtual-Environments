#version 460 core

// -----------------------------------------------------------------------------
// fondo.fs — fragment shader del FONDO (Práctico 08, Partes 4 y 5 + opcional)
//
// Pinta cada píxel del fondo por CAPAS, según su dirección d:
//   1. CIELO (por encima del horizonte): el cubemap (Parte 4) o el degradé 1D
//      por elevación (opcional), según uModoCielo (tecla K).
//   2. SUELO CONTINUADO (por debajo del horizonte, Parte 5): el rayo que sale
//      del ojo en la dirección d choca contra un PLANO HORIZONTAL INFINITO a la
//      altura media del terreno; ahí se lee la textura del pasto con la MISMA
//      escala y el MISMO origen que la malla. Como la malla se dibuja antes y
//      más cerca, este suelo solo queda a la vista donde la malla no llega:
//      tapa la "banda" del problema de la altitud.
//
// ESCENA Z-ARRIBA: la guía usa y como altura; acá la altura es z. Entonces:
//   - "debajo del horizonte" es d.z < 0 (la guía: d.y < 0);
//   - el plano del suelo es z = altura del terreno (la guía: y = ys);
//   - el punto del suelo es (x, y) (la guía: (x, z)).
// El cubemap de OpenGL, en cambio, tiene "arriba" en +Y: para leerlo se
// reacomoda la dirección con a_cubemap().
// -----------------------------------------------------------------------------

// ENTRADA (in): la dirección de este píxel, interpolada desde fondo.vs (en el
// MUNDO, Z arriba). No es unitaria: se normaliza en main.
in vec3 vDireccion;

// SALIDA (out): el color final de este píxel (cielo o suelo continuado).
out vec4 FragColor;

layout (binding = 0) uniform samplerCube uCielo;          // unidad 0: el cubemap
layout (binding = 1) uniform sampler2D   uTexturaSuelo;   // unidad 1: el pasto (el mismo de la malla)
layout (binding = 2) uniform sampler1D   uDegrade;        // unidad 2: degradé (opcional)

uniform int   uModoCielo;        // 0 = cubemap, 1 = degradé 1D (tecla K)
uniform int   uSueloContinuado;  // 1 = suelo continuado, 0 = no (tecla G: se ve la banda)
uniform vec2  uOjoUV;            // posición (x, y) del ojo en el mundo, MÓDULO el período de
                                 // las (u, v) del terreno (Terrain::kPeriodoUV), unidades de escena
uniform float uAlturaOjo;        // altura del ojo SOBRE el plano del suelo (unidades de escena)
uniform float uTamTesela;        // tamaño de una tesela (el mismo uTamTesela del terreno)

// Niebla lineal: la MISMA de terreno.fs (repetida a propósito hasta el Práctico
// 09, que la pasa a código común). El suelo continuado tiene que recibir
// exactamente la misma niebla que la malla: si no, se vería el borde.
uniform vec3  uNieblaColor;   // color de la niebla (= horizonte del cielo)
uniform float uNieblaInicio;  // distancia donde empieza
uniform float uNieblaFin;     // distancia donde es total; <= 0 -> apagada (tecla N)

vec3 aplicar_niebla(vec3 color, float distancia)
{
    if (uNieblaFin <= 0.0) {
        return color;
    }
    float f = clamp((distancia - uNieblaInicio) / (uNieblaFin - uNieblaInicio), 0.0, 1.0);
    return mix(color, uNieblaColor, f);
}

// Dirección de NUESTRA escena (Z arriba) -> convención del cubemap (Y arriba).
// (x, y, z) -> (x, z, -y) es una ROTACIÓN de -90° alrededor del eje X: lleva
// nuestro "arriba" (+Z) al "arriba" del cubemap (+Y). Al ser una rotación (y no
// un simple intercambio de ejes) no espeja la imagen.
// Lo que se rota es el VECTOR DIRECCIÓN d (hacia dónde mira este píxel), no la
// imagen ni la geometría: se reescribe la MISMA dirección en los ejes del
// cubemap, para que texture() lea el texel correcto. Ejemplos:
//   arriba    (0, 0, 1) -> (0, 1, 0)  = cara +Y (top)
//   hacia +Y  (0, 1, 0) -> (0, 0, -1) = cara -Z (back)
//   hacia +X  (1, 0, 0) -> (1, 0, 0)  = cara +X (right; el eje de giro no cambia)
// (Rotación de -90° alrededor de X: y' = z, z' = -y. El cambio "a secas"
// (x, z, y) sería un ESPEJO, con determinante -1: el cielo se vería reflejado.)
vec3 a_cubemap(vec3 d)
{
    return vec3(d.x, d.z, -d.y);
}

void main()
{
    // La dirección interpolada no es unitaria: para la elevación y la
    // intersección hace falta normalizarla (el cubemap no lo necesita).
    vec3 d = normalize(vDireccion);

    // --- 1. Cielo: cubemap o degradé ---------------------------------------
    vec3 cielo_cubo = texture(uCielo, a_cubemap(d)).rgb;   // color del cubemap en esa dirección
    // Degradé por ELEVACIÓN: asin(d.z) va de -90° a +90°; s = 0 en el horizonte
    // y 1 en el cenit. Debajo del horizonte el clamp deja s = 0 (color del
    // horizonte), que es lo que se ve si el suelo continuado está apagado.
    float elevacion  = asin(clamp(d.z, -1.0, 1.0));
    float s          = clamp(elevacion / radians(90.0), 0.0, 1.0);
    vec3 cielo_degr  = texture(uDegrade, s).rgb;                        // color del degradé
    vec3 cielo       = (uModoCielo == 1) ? cielo_degr : cielo_cubo;     // el cielo elegido (tecla K)

    // --- 2. Suelo continuado: rayo P(t) = ojo + t d contra el plano ----------
    //   ojo.z + t d.z = z_suelo   ->   t = (ojo.z - z_suelo) / (-d.z) = uAlturaOjo / (-d.z)
    // t > 0 solo si d.z < 0 (el rayo baja). max(-d.z, 1e-4) evita dividir por 0
    // en el horizonte (ahí t crece sin límite: el suelo queda infinitamente lejos).
    float t   = uAlturaOjo / max(-d.z, 1.0e-4);    // distancia del ojo al suelo por este rayo
    vec2  xy  = uOjoUV + t * d.xy;                 // punto del suelo visto en este píxel
    // MISMA regla que la malla: posición en el mundo / tamaño de la tesela. Hacia
    // el horizonte las (u, v) cambian muy rápido entre píxeles vecinos y el
    // mipmap usa niveles cada vez más chicos: queda el color promedio del pasto.
    vec3 suelo = texture(uTexturaSuelo, xy / uTamTesela).rgb;   // color del pasto en ese punto
    suelo = aplicar_niebla(suelo, t);              // t = distancia al ojo (d es unitaria)

    // Las TRES texturas se leyeron siempre, ANTES de elegir: el filtrado con
    // mipmaps compara coordenadas de píxeles vecinos y dentro de un if eso no
    // está definido. Recién ahora se elige el color.
    // debajo = el rayo baja (mira por debajo del horizonte) y el ojo está por
    // encima del suelo (si no, el plano nunca se cruzaría hacia abajo).
    bool debajo = (d.z < 0.0) && (uAlturaOjo > 0.0);
    vec3 color  = (uSueloContinuado == 1 && debajo) ? suelo : cielo;   // color final

    FragColor = vec4(color, 1.0);
}
