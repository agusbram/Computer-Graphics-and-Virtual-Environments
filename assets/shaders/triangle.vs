#version 460 core

// -----------------------------------------------------------------------------
// triangle.vs — vertex shader del primer triángulo (Práctico 01)
//
// Ya no lo usa el programa. Lo cargan los tests del ResourceManager
// (tests/main_test_rm.cpp) como archivo de ejemplo.
// -----------------------------------------------------------------------------

// ENTRADA (in): la posición de cada vértice, ya en coordenadas de recorte
// (-1..1): sin matrices, el triángulo se dibuja tal cual en la pantalla.
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);   // salida obligatoria: posición (w = 1)
}
