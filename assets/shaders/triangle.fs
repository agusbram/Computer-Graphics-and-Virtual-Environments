#version 460 core

// -----------------------------------------------------------------------------
// triangle.fs — fragment shader del primer triángulo (Práctico 01)
//
// Ya no lo usa el programa. Lo cargan los tests del ResourceManager
// (tests/main_test_rm.cpp) como archivo de ejemplo.
// -----------------------------------------------------------------------------

// SALIDA (out): el color de cada píxel del triángulo.
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);   // naranja fijo (R, G, B, A)
}
