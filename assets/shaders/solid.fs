#version 460 core

// =============================================================================
// OBSOLETO desde el Práctico 08: este archivo YA NO SE CARGA. Se conserva como
// referencia de la versión del Práctico 07 (terreno + avión con un solo
// programa). Su contenido pasó a:
//   - terreno.vs / terreno.fs   (morph del LOD + textura del pasto + niebla)
//   - aeronave.vs / aeronave.fs (piezas del avión: color, chapa o imagen de prueba)
// =============================================================================

// -----------------------------------------------------------------------------
// solid.fs — fragment shader (Práctico 03)
//
// El color NO viene por vertice (salió de la tupla): llega por uniform,
// uno por objeto. Lo setea el CPU antes de dibujar cada pieza.
// -----------------------------------------------------------------------------

in vec3 vNormal;        // normal interpolada (solo para el modo de depuración de abajo)
in float vDistancia;    // P07 (niebla, mejora leve): distancia a la cámara (viene de solid.vs)
out vec4 FragColor;     // color final del píxel

uniform vec3 uColor;    // color de la pieza (un valor por objeto)

// Práctico 07 (mejora leve) — NIEBLA LINEAL. Versión básica: se mejora en el
// Práctico 09 (Unidad IX, iluminación). Cuanto más lejos está el fragmento, más se
// mezcla su color con el color de la niebla (= color del cielo/fondo):
//   f = (d - inicio) / (fin - inicio), recortado a [0, 1]
//   color = mix(uColor, uNieblaColor, f)
// Antes de `inicio` no hay niebla; desde `fin` el objeto tiene EXACTAMENTE el
// color del fondo, así que el borde del terreno (y el plano far) no se ven.
// uNieblaFin <= 0 -> niebla apagada (tecla N).
uniform vec3  uNieblaColor;   // color de la niebla
uniform float uNieblaInicio;  // distancia donde empieza
uniform float uNieblaFin;     // distancia donde es total

void main()
{
    vec3 color = uColor;
    if (uNieblaFin > 0.0) {
        float f = clamp((vDistancia - uNieblaInicio) / (uNieblaFin - uNieblaInicio),
                        0.0, 1.0);
        color = mix(uColor, uNieblaColor, f);
    }
    FragColor = vec4(color, 1.0);

    // Modo de depuracion de normales: reemplazar la linea de arriba por:
    //   FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);   // la normal, como color
    // Probado (captura + verificacion visual): el cubo muestra caras planas
    // (3 colores) y cilindro/cono/esfera degradé suave. Con este modo activo,
    // la consola avisa "uColor ... ubicacion -1" (el compilador elimina el
    // uniform que no se usa) — falla esperada y advertida por Shader::loc().
}