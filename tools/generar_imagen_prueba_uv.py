#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# generar_imagen_prueba_uv.py — imagen de prueba ASIMÉTRICA (Práctico 08, Parte 3)
#
# Genera assets/textures/prueba/prueba_uv.png, la imagen que se aplica sobre las
# primitivas (cubo, cilindro, cono) y el terreno para VERIFICAR sus coordenadas
# de textura (u, v).
# Declarado: este script lo escribió la IA (Claude) a pedido del alumno; es
# reproducible y cualquiera puede regenerar la imagen.
#
# Por qué así (lo que pide la guía): la imagen tiene que ser asimétrica en los
# DOS ejes. Un damero no sirve: invertido o espejado se ve igual. Con esta:
#   - "F" grande arriba a la izquierda: la F no es simétrica ni vertical ni
#     horizontalmente -> si se ve espejada o patas arriba, las (u, v) están mal.
#   - "2" abajo a la derecha, de otro color: confirma la diagonal.
#   - flecha "ARRIBA": marca hacia dónde crece v.
#   - "(0,0)" abajo a la izquierda y "(1,1)" arriba a la derecha: en OpenGL el
#     origen (0,0) de una textura 2D es la esquina INFERIOR izquierda.
#   - barra ROJA en el borde izquierdo (u = 0) y VERDE en el de abajo (v = 0):
#     sirven para ver dónde está la costura del cilindro (u = 0 / u = 1).
#   - grilla de 8x8: deja ver estiramientos (el cono "radial", el terreno).
#
# CÓMO DIBUJA PILLOW (la biblioteca de imágenes de Python):
#   - Image.new crea una imagen vacía de un color; ImageDraw.Draw(img) devuelve
#     un "lápiz" (acá se llama d) que dibuja ENCIMA de esa imagen.
#   - Las coordenadas son en PÍXELES, con el origen (0, 0) en la esquina
#     SUPERIOR izquierda: x crece hacia la derecha e y crece hacia ABAJO.
#     (Es al revés que OpenGL, donde v crece hacia arriba: por eso la textura se
#     carga con flip = true, ver ResourceManager.)
#   - Las funciones que se usan:
#       d.line([(x0, y0), (x1, y1)], fill=color, width=w)
#           una línea recta de (x0, y0) a (x1, y1), de w píxeles de ancho.
#       d.rectangle([x0, y0, x1, y1], fill=color)
#           un rectángulo RELLENO con esquina superior izquierda (x0, y0) y
#           esquina inferior derecha (x1, y1), ambas incluidas.
#       d.polygon([(x, y), (x, y), ...], fill=color)
#           un polígono RELLENO: se unen los puntos en orden y se cierra solo
#           (el último se une con el primero). Con 3 puntos, un triángulo.
#       d.text((x, y), "texto", font=fuente, fill=color)
#           escribe el texto con (x, y) como esquina SUPERIOR izquierda de la
#           caja donde entra el texto. El tamaño lo da la fuente.
#   - Los colores son (R, G, B) de 0 a 255.
#
# Uso (desde la raíz del proyecto; solo hace falta Pillow):
#   python3 tools/generar_imagen_prueba_uv.py
# -----------------------------------------------------------------------------

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

LADO = 512                                   # 512x512: potencia de 2 (10 niveles de mipmap)
SALIDA = Path("assets/textures/prueba/prueba_uv.png")
FUENTE = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"


def fuente(tam):
    """Devuelve una fuente de letra de `tam` píxeles de alto.

    Usa DejaVu Sans Bold (viene instalada en Ubuntu). Si no está, usa la fuente
    de Pillow, que es fija y chica: la imagen sale igual, con letras más chicas.
    """
    try:
        return ImageFont.truetype(FUENTE, tam)
    except OSError:
        return ImageFont.load_default()


def main():
    # Imagen vacía de LADO x LADO píxeles, con un fondo gris claro (casi blanco:
    # así se ven bien los colores encima). d es el "lápiz" para dibujar en ella.
    img = Image.new("RGB", (LADO, LADO), (225, 225, 220))
    d = ImageDraw.Draw(img)

    # --- Grilla de 8x8 -------------------------------------------------------
    # paso = 512 / 8 = 64 píxeles entre líneas. Para k = 1..7 se dibuja una
    # línea VERTICAL en x = k*paso (de arriba, y = 0, hasta abajo, y = LADO) y
    # una HORIZONTAL en y = k*paso (de la izquierda a la derecha). No se dibujan
    # k = 0 ni k = 8 porque esos bordes los marcan las barras de color.
    # Si la textura se estira en una pieza, los cuadrados dejan de ser cuadrados.
    paso = LADO // 8
    for k in range(1, 8):
        d.line([(k * paso, 0), (k * paso, LADO)], fill=(170, 170, 170), width=2)
        d.line([(0, k * paso), (LADO, k * paso)], fill=(170, 170, 170), width=2)

    # --- Bordes de color: u = 0 (izquierda, rojo) y v = 0 (abajo, verde) -----
    # Barra roja: rectángulo de x = 0 a 14 (15 píxeles de ancho) y de y = 0 a
    # LADO (toda la altura): el borde IZQUIERDO, donde u = 0.
    # Barra verde: de x = 0 a LADO (todo el ancho) y de y = LADO-15 a LADO: el
    # borde de ABAJO de la imagen.
    # OJO: en la IMAGEN la fila de abajo es la última (y = LADO); al cargarla
    # invertida (flip) pasa a ser v = 0 en OpenGL.
    d.rectangle([0, 0, 14, LADO], fill=(210, 30, 30))            # u = 0
    d.rectangle([0, LADO - 15, LADO, LADO], fill=(30, 160, 60))   # v = 0

    # --- "F" y "2" ------------------------------------------------------------
    # "F" roja de 230 px, con su esquina superior izquierda en (40, 10): queda
    # ARRIBA a la izquierda. "2" azul de 220 px desde (330, 250): queda ABAJO a
    # la derecha. Son letras asimétricas: si la textura se aplica espejada, la
    # "F" se ve al revés (como en un espejo); si se aplica patas arriba, la "F"
    # queda abajo y el "2" arriba.
    d.text((40, 10), "F", font=fuente(230), fill=(200, 20, 20))
    d.text((330, 250), "2", font=fuente(220), fill=(20, 60, 200))

    # --- Flecha hacia arriba (sentido en que crece v) con la palabra ARRIBA ---
    # cx = 300 es la x del CENTRO de la flecha. La flecha se arma con dos piezas:
    #   1. la PUNTA: un triángulo con el vértice de arriba en (cx, 40) y la base
    #      de (cx-40, 110) a (cx+40, 110): 80 px de ancho y 70 px de alto;
    #   2. el PALO: un rectángulo de 24 px de ancho (de cx-12 a cx+12) que va
    #      desde la base del triángulo (y = 110) hasta y = 200.
    # Debajo, la palabra "ARRIBA" empezando en x = cx-62 para que quede más o
    # menos centrada bajo la flecha (la palabra mide unos 127 px de ancho).
    cx = 300
    d.polygon([(cx, 40), (cx - 40, 110), (cx + 40, 110)], fill=(20, 20, 20))
    d.rectangle([cx - 12, 110, cx + 12, 200], fill=(20, 20, 20))
    d.text((cx - 62, 205), "ARRIBA", font=fuente(30), fill=(20, 20, 20))

    # --- Coordenadas de las esquinas (convención de OpenGL) ------------------
    # "(0,0)" cerca de la esquina inferior izquierda (55 px por encima del borde
    # de abajo, para no pisar la barra verde) y "(1,1)" cerca de la superior
    # derecha (100 px antes del borde derecho, porque el texto mide unos 80 px).
    # "u ->" abajo, en el medio: indica que u crece hacia la derecha.
    d.text((22, LADO - 55), "(0,0)", font=fuente(30), fill=(0, 0, 0))
    d.text((LADO - 100, 18), "(1,1)", font=fuente(30), fill=(0, 0, 0))
    d.text((200, LADO - 52), "u ->", font=fuente(28), fill=(0, 0, 0))

    # --- Guardar ---------------------------------------------------------------
    # Crea la carpeta si no existe y guarda en PNG (sin pérdida: los colores y
    # los bordes quedan exactos, a diferencia de JPG).
    SALIDA.parent.mkdir(parents=True, exist_ok=True)
    img.save(SALIDA)
    print(f"imagen de prueba escrita en {SALIDA} ({LADO}x{LADO})")


if __name__ == "__main__":
    main()
