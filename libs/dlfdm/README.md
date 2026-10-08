# dlfdm — modelo de dinámica de vuelo

Modelo de dinámica de vuelo (*FDM*, *Flight Dynamics Model*) de cuerpo rígido con
seis grados de libertad, escrito en C++17. Resuelve las ecuaciones de movimiento de
una aeronave a partir de sus parámetros físicos y de las posiciones de los mandos, e
integra el estado paso a paso.

Es material de la cátedra **Computación gráfica y ambientes virtuales (0494)** del
Centro Regional Universitario Córdoba – IUA. Se usa como librería: la aplicación le
pide un paso de integración por vez y lee el estado resultante para dibujar.

**Qué no es:** no dibuja, no abre ventanas y no lee teclado. No depende de OpenGL ni
de GLFW.

## Requisitos

- Un compilador con C++17 (`g++` o `clang++`).
- GNU `make`.
- [**glm**](https://github.com/g-truc/glm), sólo cabeceras, en el *include path* del
  sistema. En Debian/Ubuntu: `sudo apt install libglm-dev`. Probado con glm 0.9.9.

Es la única dependencia externa.

## Compilar y correr el ejemplo

```sh
make                 # compila la librería y el ejecutable de prueba
./build/dlfdm-test   # 60 segundos de simulación, off-line
```

`make run` hace las dos cosas y `make clean` borra los objetos y el ejecutable.

El ejecutable escribe la traza completa de la corrida en **`salida.csv`** (una fila por
paso de integración, con posición, actitud, velocidades, fuerzas, momentos y estado
atmosférico). En `plot/` hay dos guiones de *gnuplot* para graficarla. `graficar-ventanas.gp`
abre las gráficas en ventanas interactivas y espera una tecla entre una y otra; hay
que correrlo desde `plot/`, porque busca la traza en `../salida.csv`:

```sh
cd plot && gnuplot graficar-ventanas.gp
```

`graficar.gp` es la variante que exporta `.eps` en lugar de abrir ventanas.

Por defecto la corrida arranca en una condición de equilibrio en vuelo recto y
nivelado a 5000 m y 150 m/s de velocidad verdadera. La altitud se mantiene dentro de
±20 m a lo largo de los 60 s; la oscilación que se ve es el fugoide residual del
modelo, no un error de integración.

## Integrarlo a otro proyecto

El módulo declara sus fuentes e *includes* en `dlfdm.mk`, que se incluye desde el
`Makefile` del proyecto. Si el repo se clonó en `libs/dlfdm`:

```make
DLFDM_DIR = ./libs/dlfdm
include $(DLFDM_DIR)/dlfdm.mk

INC_DIRS = \
    $(DLFDM_INC_DIRS) \
    ./src

LIB_DIRS = \
    $(DLFDM_LIB_DIRS)
```

`DLFDM_INC_DIRS` son los directorios de cabeceras y `DLFDM_LIB_DIRS` los directorios
cuyos `.cpp` hay que compilar.

## Uso mínimo

```cpp
#include <dlfdm/fdmsolver.h>
#include <dlfdm/models/aircraft/jettrainer.h>

// El solver se queda con su propia copia de los parámetros.
dlfdm::AircraftParameters avion = dlfdm::jettrainer::load_model();
dlfdm::FDMSolver fdm(avion);          // paso de integración por defecto: 1/120 s

// Condición inicial: estado y mandos van juntos, son un par.
dlfdm::TrimPoint trim = dlfdm::jettrainer::get_trim_condition(
        dlfdm::jettrainer::TrimCondition::kISA5000TAS150);
fdm.setState(trim.state);

dlfdm::ControlInputs mandos = trim.controls;

// Un paso de integración por llamada.
fdm.update(mandos);
const dlfdm::AircraftState& estado = fdm.getState();
```

Los tipos (`AircraftState`, `ControlInputs`, `AircraftParameters`, `TrimPoint`) están
en `include/dlfdm/defines.h`.

## Convenciones

- **Unidades SI** y **ángulos en radianes**, en toda la interfaz.
- **Posición en marco NED**: `x` al norte, `y` al este, `z` **hacia abajo**. Una
  altitud de 5000 m es `inertial_position.z = -5000`.
- **Velocidades y velocidades angulares en ejes cuerpo**: `(u, v, w)` y `(p, q, r)`.
- **Actitud en ángulos de Euler** `phi`, `theta`, `psi` (alabeo, cabeceo, guiñada).
- Los mandos (`ControlInputs`) son deflexiones en radianes, salvo `throttle`, que va
  normalizado en `[0, 1]`.

Una aplicación gráfica trabaja en otro marco, así que la conversión corre por cuenta
de quien lo integra. `FDMSolver::getModelMatrix()` **no** hace esa conversión: arma la
matriz directamente con los valores NED.

## El modelo de aeronave

`include/dlfdm/models/aircraft/jettrainer.h` trae un entrenador a reacción listo para
usar, con una condición de equilibrio calculada. Los parámetros aerodinámicos y de
masa salen del *Airplane 'C'* de:

> Roskam, J. *Airplane Flight Dynamics and Automatic Flight Controls, Part I*.
> DARcorporation.

Las páginas están citadas en los comentarios de `src/dlfdm/models/aircraft/jettrainer.cpp`.
El modelo aerodinámico es lineal y vale en el entorno de la condición de equilibrio: no
pretende ser válido en pérdida, en régimen transónico ni a grandes ángulos.

Para agregar otra aeronave alcanza con llenar un `AircraftParameters` propio; el
*solver* no cambia.

## Licencia

MIT. Ver [LICENSE](LICENSE).
