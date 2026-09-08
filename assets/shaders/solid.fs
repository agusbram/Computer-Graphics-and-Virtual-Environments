#version 460 core

// -----------------------------------------------------------------------------
// solid.fs — fragment shader (Práctico 03)
//
// El color NO viene por vertice (salió de la tupla): llega por uniform,
// uno por objeto. Lo setea el CPU antes de dibujar cada pieza.
// -----------------------------------------------------------------------------

in vec3 vNormal;
out vec4 FragColor;

uniform vec3 uColor;    // color de la pieza (un valor por objeto)

void main()
{
    FragColor = vec4(uColor, 1.0);

    // Modo de depuracion de normales: reemplazar la linea de arriba por:
    //   FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);   // la normal, como color
    // Probado (captura + verificacion visual): el cubo muestra caras planas
    // (3 colores) y cilindro/cono/esfera degradé suave. Con este modo activo,
    // la consola avisa "uColor ... ubicacion -1" (el compilador elimina el
    // uniform que no se usa) — falla esperada y advertida por Shader::loc().
}