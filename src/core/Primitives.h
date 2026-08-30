// -----------------------------------------------------------------------------
// Primitives.h
//
// Generador de mallas de figuras primitivas del lado del CPU: devuelve un
// MeshData listo para subir a la GPU con Mesh. NO usa la API de OpenGL, es
// un generador de datos.
//
// Decisiones de diseño (Practico 02, Parte 3):
//   1. FUNCION LIBRE, NO CLASE. La pregunta "¿qué posee este objeto?" se
//      responde sola: ESTE modulo no posee nada (ni estado ni recursos que
//      liberar). La forma que sigue a la propiedad es un conjunto de
//      funciones libres en un namespace. Cuando aparezcan cylinder(radio,
//      altura, gajos) y cone(...) —misma forma: devolver MeshData con
//      parametros de tamaño— se agregan aca sin reescribir el modulo.
//   2. COLORES FIJOS DENTRO DEL GENERADOR: elegir el color al crear la malla
//      mantiene simple la tupla Vertex. Si algun dia se necesitan colores
//      propios se agrega un overload, sin cambiar la firma actual.
//   3. CUBO CENTRADO EN EL ORIGEN (de -0.5 a +0.5 por defecto en cada eje):
//      con la escala por eje se estiran otras figuras de caras rectas
//      paralelas (ej. una placa). Centrado conviene más para transformar
//      despues que apoyado sobre y = 0.
//   4. LAS CABEZAS RECORREN EN ANTIHORARIO VISTO DESDE AFUERA: ese sentido
//      decide qué cara se dibuja cuando se active el descarte de caras
//      traseras del teórico; se fija ahora y no se cambia.
// -----------------------------------------------------------------------------

#pragma once

#include "core/MeshData.h"

namespace primitives {

// Malla indexada de un cubo: 24 vértices únicos y 36 índices, un color
// plano distinto por cara. scale_x/y/z estiran el cubo en cada dirección;
// con los valores por defecto devuelve un cubo de lado unitario (l = 1),
// centrado en el origen.
MeshData cube(float scale_x = 1.0f, float scale_y = 1.0f, float scale_z = 1.0f);

// Segunda primitiva (ejemplo de portabilidad): un triángulo en el plano xy,
// 3 vértices únicos con un color distinto por vértice y 3 índices. Todas las
// figuras nuevas se agregan igual: una función que devuelve MeshData con la
// tupla Vertex y las esquinas en sentido antihorario; Mesh y Shader no cambian.
MeshData triangle(void);

}   // namespace primitives