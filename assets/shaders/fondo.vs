#version 460 core

// -----------------------------------------------------------------------------
// fondo.vs — vertex shader del FONDO: cielo + suelo continuado (Práctico 08)
//
// La geometría es el cubo de las primitivas (Práctico 03), centrado en su
// origen. Cada vértice, medido desde el centro, ES UNA DIRECCIÓN: la que se usa
// para leer el cubemap (o el degradé, o para trazar el rayo del suelo).
// -----------------------------------------------------------------------------

// ENTRADA (in): la posición de cada vértice del cubo (de -0.5 a 0.5 en cada
// eje). Solo se usa la location 0: el fondo no necesita normal ni (u, v).
layout (location = 0) in vec3 aPos;

// SALIDA (out): va, interpolada, a fondo.fs. Interpolar las posiciones de las
// esquinas da, en cada píxel, la dirección que pasa por ese píxel.
out vec3 vDireccion;   // dirección desde el ojo hacia este punto del cielo (MUNDO)

uniform mat4 uProjection;  // proyección en perspectiva (la misma de la escena)
uniform mat4 uView;        // la vista SIN TRASLACIÓN: mat4(mat3(vista))

void main()
{
    // La dirección va en coordenadas de MUNDO (sin la rotación de la cámara):
    // así el cielo queda "pegado al mundo" y gira cuando gira la cámara.
    vDireccion = aPos;

    // Sin la traslación de la vista, el cubo queda SIEMPRE centrado en el ojo:
    // el observador nunca se acerca a sus caras (no hay paralaje) y el cielo se
    // percibe infinitamente lejos.
    vec4 pos = uProjection * uView * vec4(aPos, 1.0);

    // MÉTODO (b): profundidad al MÁXIMO. Con z = w, después de dividir por w la
    // profundidad vale 1 (plano lejano) para todo el cubo: queda detrás de todo
    // sin importar su tamaño. Del lado de C++ se dibuja AL ÚLTIMO y con
    // glDepthFunc(GL_LEQUAL) (1 <= 1 pasa solo donde la escena no dibujó nada).
    gl_Position = pos.xyww;
}
