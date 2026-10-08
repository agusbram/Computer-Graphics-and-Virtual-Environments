# Configuración general
set datafile separator ","
set key autotitle columnhead

# Configuración de títulos y etiquetas
set xlabel "Tiempo"
set ylabel "Valor"
set grid

# GRÁFICO 1: Altitud
set terminal postscript eps color
set output "altitud.eps"
set title "Altitud"
set yrange [0:2500]  # Ajusta min:max según tus datos
plot "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:4 with lines title "h"

#pause -1 "Presiona Enter para continuar..."

# GRÁFICO 2: Ángulos
set output "angulos.eps"
set title "Angulos"
set yrange [-3.14159:3.14159]  # Ajusta min:max según tus datos
plot "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:6 with lines title "theta", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:23 with lines title "alpha"

#pause -1 "Presiona Enter para continuar..."

# GRÁFICO 3: Velocidades lineales en ejes cuerpo
set output "v_body.eps"
set title "Velocidades en ejes cuerpo"
set yrange [-200:200]  # Ajusta min:max según tus datos
plot "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:8 with lines title "u", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:9 with lines title "v", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:10 with lines title "w"

#pause -1 "Presiona Enter para continuar..."

# GRÁFICO 4: velocidades angulares
set output "omega.eps"
set title "Velocidades angulares"
set yrange [-50:50]  # Ajusta min:max según tus datos
plot "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:11 with lines title "p", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:12 with lines title "q", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:13 with lines title "r"

#pause -1 "Presiona Enter para continuar..."

# GRÁFICO 5: fuerzas
set output "forces.eps"
set title "Fuerzas"
set yrange [-20000:20000]  # Ajusta min:max según tus datos
plot "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:14 with lines title "D", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:15 with lines title "Y", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:16 with lines title "L", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:17 with lines title "Xb", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:18 with lines title "Yb", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:19 with lines title "Zb"

#pause -1 "Presiona Enter para continuar..."

# GRÁFICO 6: momentos
set output "torques.eps"
set title "Momentos"
set yrange [-20000:20000]  # Ajusta min:max según tus datos
plot "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:20 with lines title "L", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:21 with lines title "M", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:22 with lines title "N"

#pause -1 "Presiona Enter para continuar..."

# GRÁFICO 7: propulsión
set output "empuje.eps"
set title "Propulsión"
set yrange [0:12000]  # Ajusta min:max según tus datos
set y2range [0:1]
set y2tics
plot "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:40 with lines title "empuje", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:38 axes x1y2 with lines title "Mach", \
     "./build/Desktop_Qt_5_15_0_x64-Release/salida.csv" using 1:39 axes x1y2 with lines title "F/F_SL"

unset y2tics

#pause -1 "Presiona Enter para salir..."
