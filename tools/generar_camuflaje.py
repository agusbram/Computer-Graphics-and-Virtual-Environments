#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# generar_camuflaje.py — pintura de camuflaje para el avión (Práctico 08, extra)
#
# Genera assets/textures/avion/camuflaje.png: un camuflaje militar de cuatro
# tonos (verde oscuro, oliva, marrón y arena) con el DETALLE de la chapa
# metálica (rayones y remaches de chapa_color.jpg, CC0) ya incorporado.
# Declarado: este script lo escribió la IA (Claude) a pedido del alumno; es
# reproducible y cualquiera puede regenerar la imagen.
#
# Por qué así:
#   - TILEABLE (se repite sin costura): las manchas salen de ruido aleatorio
#     suavizado con un filtro aplicado en el dominio de la FRECUENCIA (FFT).
#     La FFT trata la imagen como PERIÓDICA (el borde derecho continúa en el
#     izquierdo), así que el resultado empalma solo al repetirse con GL_REPEAT.
#   - MANCHAS CON BORDE NETO: el ruido suave se corta por UMBRALES (percentiles)
#     en cuatro zonas, una por color, como un camuflaje pintado.
#   - DETALLE DE CHAPA: se multiplica cada píxel por la luminancia de la chapa
#     dividida por su media (en promedio vale 1: no oscurece los colores).
#     Así el avión usa UNA sola textura y no hace falta tocar el shader.
#
# LOS PASOS, EN RESUMEN (cada uno está explicado en el código):
#   1. "campo": una imagen en escala de grises SUAVE, con zonas altas y bajas
#      de unos 200 píxeles (como un mapa de lomas y valles), que se repite sin
#      costura.
#   2. Cada píxel del campo se reemplaza por uno de los 4 colores según qué
#      tan "alto" está: los más bajos verde oscuro, los más altos arena. Como
#      los cortes son escalones, las manchas quedan con bordes netos.
#   3. Se multiplica por el detalle de la chapa (rayones y remaches).
#
# Todo trabaja con ARREGLOS de numpy: una imagen es una matriz de LADO x LADO
# números (o LADO x LADO x 3 para RGB), y las operaciones (+, *, /, comparar)
# se aplican a TODOS los píxeles a la vez, sin escribir bucles.
#
# Uso (desde la raíz del proyecto; necesita numpy y Pillow):
#   python3 tools/generar_camuflaje.py
# -----------------------------------------------------------------------------

from pathlib import Path

import numpy as np
from PIL import Image

LADO = 1024                    # la imagen sale de 1024x1024 (como la chapa)
SEMILLA = 2026                 # semilla fija: el azar sale SIEMPRE igual (reproducible)
CHAPA = Path("assets/textures/avion/chapa_color.jpg")
SALIDA = Path("assets/textures/avion/camuflaje.png")

# Colores del camuflaje (sRGB 0-255), del más oscuro al más claro, y qué
# fracción de la superficie ocupa cada uno.
COLORES = np.array([
    [ 52,  62,  42],   # verde oscuro
    [ 92,  98,  60],   # oliva
    [110,  86,  58],   # marrón
    [170, 158, 120],   # arena
], dtype=np.float64)
FRACCIONES = [0.28, 0.30, 0.24, 0.18]          # suman 1 (100 % de la superficie)


def ruido_periodico(lado, tam_mancha, rng):
    """Ruido blanco suavizado con un filtro gaussiano en frecuencia (periódico).

    Devuelve una matriz lado x lado de valores "suaves": vecinos cercanos tienen
    valores parecidos y hay zonas altas y bajas de unos `tam_mancha` píxeles.
    Tiene media 0 y desviación estándar 1.

    tam_mancha: tamaño típico de las manchas, en píxeles.
    rng: el generador de números aleatorios (con la semilla fija).
    """
    # 1. RUIDO BLANCO: un número aleatorio independiente por píxel (distribución
    #    normal, media 0). Visto como imagen es "nieve" de televisor: cambia de
    #    golpe de un píxel al vecino, no tiene manchas.
    blanco = rng.standard_normal((lado, lado))

    # 2. LAS FRECUENCIAS DE LA IMAGEN. La FFT (transformada rápida de Fourier)
    #    escribe la imagen como una SUMA DE ONDAS (senos y cosenos) de distintas
    #    frecuencias. Frecuencia baja = onda larga = manchas grandes; frecuencia
    #    alta = onda corta = detalle fino (la "nieve").
    #    fftfreq(lado) da, para cada posición del resultado de la FFT, qué
    #    frecuencia representa, en ciclos por píxel (de -0.5 a 0.5).
    #    meshgrid arma con eso dos matrices: fx[i, j] es la frecuencia en x y
    #    fy[i, j] la frecuencia en y de la onda guardada en la posición (i, j).
    f = np.fft.fftfreq(lado)                       # frecuencias en ciclos/píxel
    fx, fy = np.meshgrid(f, f)

    # 3. EL FILTRO: una campana de Gauss en frecuencia. Vale 1 para la
    #    frecuencia 0 y baja a medida que la frecuencia crece. sigma = 1/tam_mancha:
    #    las ondas de período mayor que tam_mancha (manchas grandes) pasan casi
    #    enteras; las de período menor que tam_mancha/2 quedan prácticamente en 0
    #    (a frecuencia 2/tam_mancha el filtro vale e^-2 ≈ 0,14).
    sigma = 1.0 / tam_mancha
    filtro = np.exp(-(fx ** 2 + fy ** 2) / (2.0 * sigma ** 2))

    # 4. FILTRAR: fft2 pasa la imagen a frecuencias; se multiplica cada onda por
    #    el filtro (se apagan las cortas); ifft2 vuelve a píxeles. Multiplicar en
    #    frecuencia equivale a DESENFOCAR en el espacio: la "nieve" se convierte
    #    en manchas suaves.
    #    Por qué TILEABLE: la FFT supone que la imagen se repite infinitamente
    #    (el borde derecho sigue en el izquierdo). El desenfoque mezcla cada
    #    borde con el opuesto, así que al repetir la textura no hay costura.
    #    np.real: el resultado teórico es real; la parte imaginaria es solo
    #    error de redondeo (del orden de 1e-16) y se descarta.
    suave = np.real(np.fft.ifft2(np.fft.fft2(blanco) * filtro))

    # 5. NORMALIZAR: media 0 y desviación 1. Así dos ruidos de distinto tamaño
    #    de mancha quedan en la MISMA escala y se pueden sumar con un peso.
    return (suave - suave.mean()) / suave.std()


def main():
    # Generador aleatorio con semilla fija: cada vez que se corre el script
    # sale EXACTAMENTE la misma imagen.
    rng = np.random.default_rng(SEMILLA)

    # --- 1. El campo: dos escalas de manchas sumadas -------------------------
    # Grandes (unos 200 px, la forma general) + medianas (unos 60 px, con peso
    # 0.35) que deforman los bordes de las grandes. Con una sola escala las
    # manchas saldrían redondeadas, como círculos; con dos son irregulares.
    # (El orden de las dos llamadas importa: cada una consume números del rng.)
    campo = ruido_periodico(LADO, 200.0, rng) + 0.35 * ruido_periodico(LADO, 60.0, rng)

    # --- 2. De valores suaves a 4 colores con bordes netos --------------------
    # Umbrales por percentiles: cada color ocupa su fracción de la superficie.
    #   np.cumsum(FRACCIONES)      -> [0.28, 0.58, 0.82, 1.00] (acumuladas)
    #   [:-1]                      -> [0.28, 0.58, 0.82]       (se saca el 1.00)
    #   * 100                      -> [28, 58, 82]             (en porcentaje)
    #   np.percentile(campo, ...)  -> los 3 VALORES del campo que dejan por
    #                                 debajo el 28 %, el 58 % y el 82 % de los
    #                                 píxeles.
    cortes = np.percentile(campo, np.cumsum(FRACCIONES)[:-1] * 100.0)
    # np.digitize dice, para cada píxel, en qué escalón cae:
    #   campo < cortes[0]               -> 0 (verde oscuro, 28 % de los píxeles)
    #   cortes[0] <= campo < cortes[1]  -> 1 (oliva,        30 %)
    #   cortes[1] <= campo < cortes[2]  -> 2 (marrón,       24 %)
    #   campo >= cortes[2]              -> 3 (arena,        18 %)
    indice = np.digitize(campo, cortes)            # 0..3 = qué color va en cada píxel
    # Indexar COLORES con la matriz de índices reemplaza cada 0..3 por la fila
    # RGB de ese color: de una matriz LADO x LADO se pasa a LADO x LADO x 3.
    rgb = COLORES[indice]                          # (LADO, LADO, 3)

    # --- 3. El detalle de la chapa ---------------------------------------------
    # Se abre la chapa, se pasa a escala de grises ("L" = luminancia, un solo
    # número por píxel) y se lleva a LADO x LADO (LANCZOS es un filtro de buena
    # calidad para cambiar de tamaño; si ya mide LADO, queda igual).
    chapa = Image.open(CHAPA).convert("L").resize((LADO, LADO), Image.LANCZOS)
    lum = np.asarray(chapa, dtype=np.float64)      # imagen -> matriz de números
    # Dividir por la media: un píxel de brillo "promedio" vale 1, uno más claro
    # (un rayón) vale más de 1 y uno más oscuro (un remache, una junta) menos.
    # Multiplicar por esto no cambia el color promedio del camuflaje.
    detalle = lum / lum.mean()
    # Se suaviza el contraste del detalle (0.6): que se note pero sin "ensuciar".
    # Acerca cada valor a 1: por ejemplo 1.5 -> 1.3 y 0.5 -> 0.7.
    detalle = 1.0 + 0.6 * (detalle - 1.0)
    # detalle es LADO x LADO y rgb es LADO x LADO x 3: detalle[..., None] le
    # agrega un eje (LADO x LADO x 1) para que numpy lo aplique igual a los tres
    # canales R, G y B ("broadcasting").
    # np.clip limita a 0..255 ANTES de pasar a uint8 (enteros de 8 bits): si un
    # valor quedara en 260, sin el clip daría la vuelta y se volvería 4 (negro).
    rgb = np.clip(rgb * detalle[..., None], 0, 255).astype(np.uint8)

    # --- Guardar ---------------------------------------------------------------
    # Matriz -> imagen RGB -> PNG (sin pérdida). Crea la carpeta si no existe.
    SALIDA.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(rgb, "RGB").save(SALIDA)
    print(f"camuflaje escrito en {SALIDA} ({LADO}x{LADO}, tileable)")


if __name__ == "__main__":
    main()
