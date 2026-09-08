// -----------------------------------------------------------------------------
// Primitives.h
//
// Generador de mallas de figuras primitivas del lado del CPU: devuelve un
// MeshData listo para subir a la GPU con Mesh. NO usa la API de OpenGL, es
// un generador de datos.
//
// Decisiones de diseño (Practico 02, Parte 3; ampliado en el Practico 03):
//   1. FUNCION LIBRE, NO CLASE. La pregunta "¿qué posee este objeto?" se
//      responde sola: ESTE modulo no posee nada (ni estado ni recursos que
//      liberar). La forma que sigue a la propiedad es un conjunto de
//      funciones libres en un namespace. cylinder()/cone()/sphere() se
//      agregan aca con la misma forma sin reescribir el modulo.
//   2. GENERACION PARAMETRICA (Practico 03): las figuras curvas NO se
//      escriben a mano. Una funcion recibe radio/largo/gajos/anillos y
//      calcula todos los vertices e indices en bucles; ninguna coordenada
//      se teclea. Asi cambiar de 8 a 36 gajos no reescribe nada.
//   3. SISTEMA DE REFERENCIA: el eje de revolucion (eje "largo") es Y. Las
//      coordenadas de un anillo son (r*cos, y, r*sen) y la normal del
//      lateral del cilindro es (cos, 0, sen) — radial, con la componente
//      axial en cero. El cubo sigue centrado en el origen.
//   4. ANGULOS EN RADIANES. "conicidad" es el angulo de apertura COMPLETO
//      del cono (α); el semiangulo es α/2. Documentar las unidades evita el
//      clasico "resultado que no se parece a nada".
//   5. CANTIDAD DE VERTICES POR ANILLO = N+1 (no N). En la costura la
//      posicion y la normal coinciden, pero la coordenada de textura vale 0
//      y 1 a la vez: un atributo difiere, asi que el vertice se duplica.
//   6. ORIENTACION EN ANTIHORARIO VISTO DESDE AFUERA. En los bucles el
//      orden "natural" {b0, b1, t1} deja los triangulos invertidos; se usa
//      el orden {b0, t1, b1} y {b0, t0, t1}, verificado por producto
//      vectorial (ver Practico03-*.md).
//   7. SOLO SUPERFICIE LATERAL (sin tapas). Las tapas llevan normal axial y
//      nunca comparten vertices con el lateral; se dejan afuera por ahora
//      (decision tomada para este practico).
// -----------------------------------------------------------------------------

#pragma once

#include "core/MeshData.h"

namespace primitives {

// Malla indexada de un cubo: 24 vértices únicos y 36 índices, con la normal
// de cada cara y coordenadas de textura por esquina. scale_x/y/z estiran el
// cubo en cada dirección; con los valores por defecto devuelve un cubo de
// lado unitario (l = 1), centrado en el origen. Sigue teniendo 24 vértices
// porque la normal cambia de forma discreta en cada arista.
MeshData cube(float scale_x = 1.0f, float scale_y = 1.0f, float scale_z = 1.0f);

// Cilindro parametrico, eje de revolucion = Y, centrado en el origen.
//   radio  : radio de la base (>= 0)
//   largo  : altura total a lo largo de Y
//   gajos  : discretizacion angular (>= 3)
//   anillos: subdivisiones axiales (default 1 -> dos anillos de vertices,
//            uno en cada extremo). Un anillo lleva gajos+1 vertices.
// Superficie lateral SOLAMENTE (sin tapas). Normal radial (cos, 0, sen).
MeshData cylinder(float radio, float largo,
                  unsigned gajos, unsigned anillos = 1U);

// Cono parametrico, eje de revolucion = Y, base abajo y apice arriba.
//   radio    : radio de la base (>= 0)
//   conicidad: angulo de apertura COMPLETO, en RADIANES. El semiangulo es
//              conicidad/2 y fija la altura: largo = radio / tan(semiangulo).
//   gajos    : discretizacion angular de la base (>= 3)
//   anillos  : subdivisiones axiales del lateral (default 1 -> solo el anillo
//              de la base y el apice).
// La normal lateral esta inclinada por el semiangulo:
//   (cos(semi)*cos, sin(semi), cos(semi)*sen)
// El apice es UN solo vertice con normal axial (0, 1, 0).
MeshData cone(float radio, float conicidad,
              unsigned gajos, unsigned anillos = 1U);

// Triángulo plano en xy (ejemplo de portabilidad): 3 vértices, normal +z.
MeshData triangle(void);

// Esfera parametrica (OPCIONAL de la guia, no forma parte del trabajo): el
// mismo metodo que cilindro/cono con un bucle mas: anillos de LATITUD en vez
// de dos anillos. Puede servir para modelar el cielo.
//   radio  : radio de la esfera (>= 0)
//   gajos  : discretizacion angular alrededor del eje Y (>= 3)
//   anillos: bandas de latitud entre los polos (default 8)
// Los polos son UN solo vertice cada uno, con normal axial (0, ±1, 0) — el
// mismo criterio del apice del cono. La normal de los anillos es radial:
// (cos(lat)*cos(lon), sen(lat), cos(lat)*sen(lon)).
MeshData sphere(float radio, unsigned gajos, unsigned anillos = 8U);

}   // namespace primitives