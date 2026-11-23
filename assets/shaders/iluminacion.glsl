// -----------------------------------------------------------------------------
// iluminacion.glsl — código GLSL COMÚN a los fragment shaders de la escena
// (Práctico 09, Parte 1)
//
// El terreno, el avión y el fondo (que pinta el suelo continuado) tienen que
// usar EL MISMO sol y LA MISMA niebla: si uno usara otro modelo u otros
// valores, se notaría una costura (por ejemplo, entre la malla del terreno y
// el suelo continuado). Por eso las funciones y sus parámetros se escriben UNA
// sola vez, acá, y cada fragment shader las usa en lugar de repetirlas. (En el
// Práctico 08 la niebla estaba repetida en terreno.fs, aeronave.fs y fondo.fs.)
//
// CÓMO SE USA. GLSL no tiene #include: este archivo se PEGA delante del código
// propio de cada fragment shader. main lo pide así al cargar cada programa:
//   load_shader_source("terreno", {"shaders/terreno.vs"},
//                      {"shaders/iluminacion.glsl", "shaders/terreno.fs"});
// y Shader compila las cadenas una detrás de la otra:
//   "#version 460 core" + iluminacion.glsl + terreno.fs
// Como este archivo va ANTES, todo lo que declara (uniforms, el struct
// Material y las funciones) ya existe cuando empieza el código propio.
//
// SIN #version: esa línea tiene que ser la primera del texto completo y la
// agrega Shader a todas las etapas. Si este archivo (o uno propio) la tuviera,
// quedaría en el medio del texto y no compilaría.
//
// OJO: el código propio NO puede volver a declarar un uniform de acá (por
// ejemplo uOjo o uNieblaColor): el compilador da un error de redefinición.
//
// COORDENADAS: "mundo" es el sistema con ORIGEN FLOTANTE (el avión en el
// origen; ver main.cpp). P, N, uSolDir y uOjo tienen que estar TODOS en ese
// mismo sistema. Mezclar sistemas no da error: el síntoma es que la luz se
// mueve con la cámara. La escena es Z-ARRIBA (la guía usa Y-arriba), pero
// estas funciones no dependen de cuál eje es "arriba".
// -----------------------------------------------------------------------------

// --- Parámetros de la ESCENA ---------------------------------------------------
// Los manda C++ en cada cuadro desde el struct Iluminacion (Iluminacion.h), el
// MISMO para los tres programas. Un programa que no use alguno no lo tiene (el
// compilador lo elimina) y su ubicación es -1: está previsto.

uniform vec3  uSolDir;        // dirección HACIA el sol (de la superficie a la luz), mundo, unitaria
uniform vec3  uSolColor;      // color e intensidad del sol
uniform vec3  uAmbiente;      // luz ambiente (la que llega a todos lados, aun en sombra)
uniform vec3  uOjo;           // posición de la cámara, en mundo (relativa al avión: origen flotante)

// Niebla. Parte 1: la niebla LINEAL del Práctico 08, sin cambios (solo se mudó
// a este archivo). En la Parte 4 pasa a la exponencial, con una visibilidad V.
uniform vec3  uNieblaColor;   // color de la niebla = color del horizonte del cielo
uniform float uNieblaInicio;  // distancia donde EMPIEZA (antes: sin niebla), unidades de escena
uniform float uNieblaFin;     // distancia donde es TOTAL; <= 0 -> niebla apagada (tecla N)

// --- Material (Partes 2 y 3) ----------------------------------------------------
// Coeficientes del modelo de Phong de UN objeto. Es un dato del OBJETO, no de
// la escena: el terreno lo arma con su textura y el avión lo recibe por pieza.
struct Material
{
    vec3  kd;    // coeficiente difuso (y ambiente: ka = kd, lo habitual)
    vec3  ks;    // coeficiente especular (cuánta luz se refleja como brillo)
    float ns;    // exponente especular: más alto = brillo más chico y concentrado
};

// Modelo de iluminación de PHONG con UNA luz direccional (el sol).
//   P: posición del punto, en mundo.   N: normal en mundo, YA normalizada.
// Devuelve el color iluminado: ambiente + difusa + especular.
// (Se usa desde la Parte 2. En la Parte 1 nadie la llama todavía: por eso los
// uniforms del sol no aparecen en ningún programa.)
vec3 iluminar(vec3 P, vec3 N, Material m)
{
    vec3  L     = uSolDir;                  // direccional: la MISMA en todos los puntos
    vec3  V     = normalize(uOjo - P);      // hacia el ojo
    float NdotL = max(dot(N, L), 0.0);      // coseno entre la normal y la luz (0 si la cara no la ve)

    vec3 ambiente  = uAmbiente * m.kd;
    vec3 difusa    = uSolColor * m.kd * NdotL;
    vec3 especular = vec3(0.0);
    if (NdotL > 0.0) {                      // sin brillo en la cara que no recibe luz
        vec3 R = reflect(-L, N);            // reflect espera la dirección QUE LLEGA: -L
        especular = uSolColor * m.ks * pow(max(dot(R, V), 0.0), m.ns);
    }
    return ambiente + difusa + especular;
}

// NIEBLA: mezcla el color con el de la niebla según la distancia del punto P
// (en mundo) al ojo. mix(a, b, f) = a·(1-f) + b·f: f = 0 sin niebla, 1 total.
// Se aplica DESPUÉS de la iluminación (la niebla no se ilumina).
// Parte 1: fórmula LINEAL (la del Práctico 08). Lo único que cambió respecto
// del 08 es de dónde sale la distancia: antes la calculaba el vertex shader en
// el sistema de la cámara; ahora es |P - uOjo| en mundo. Es la MISMA distancia
// (la vista no cambia distancias), así que la imagen no tiene que cambiar.
vec3 aplicar_niebla(vec3 color, vec3 P)
{
    if (uNieblaFin <= 0.0) {                // tecla N: niebla apagada
        return color;
    }
    float d = length(P - uOjo);             // distancia al ojo
    float f = clamp((d - uNieblaInicio) / (uNieblaFin - uNieblaInicio), 0.0, 1.0);
    return mix(color, uNieblaColor, f);
}
