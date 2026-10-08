# Configuración general
set datafile separator ","
set key autotitle columnhead

# Configuración del archivo de datos
#datafile = "../../build/Desktop_Qt_5_15_0_x64-Release/salida.csv"
datafile = "../salida.csv"

# Configuración de títulos y etiquetas
set xlabel "t [seg]"

set grid

# GRÁFICO 1: Altitud
#set terminal postscript eps color
#set output "altitud.eps"
set title "Altitud"
set ylabel "h [m]"
set yrange [0:6000]  # Ajusta min:max según tus datos
plot datafile using 1:(-1.0*$4) with lines title "h"

pause -1 "Presiona Enter para continuar..."

# GRÁFICO 2: Ángulos
#set output "angulos.eps"
set title "Angulos"
set ylabel "ángulo [deg]"
set yrange [-30.0:30.0]  # Ajusta min:max según tus datos
plot datafile using 1:($6*57.2958) with lines title "theta", \
     datafile using 1:($23*57.2958) with lines title "alpha", \
     datafile using 1:($5*57.2958) with lines title "phi", \
     datafile using 1:($7*57.2958) with lines title "psi", \
     datafile using 1:($24*57.2958) with lines title "beta"

pause -1 "Presiona Enter para continuar..."

# GRÁFICO 3: Velocidades lineales en ejes cuerpo
#set output "v_body.eps"
set title "Velocidades en ejes cuerpo"
set ylabel "vel [m/s]"
set yrange [-200:200]  # Ajusta min:max según tus datos
plot datafile using 1:8 with lines title "u", \
     datafile using 1:9 with lines title "v", \
     datafile using 1:10 with lines title "w"

pause -1 "Presiona Enter para continuar..."

# GRÁFICO 4: velocidades angulares
#set output "omega.eps"
set title "Velocidades angulares"
set ylabel "vel ang [rad/s]"
set yrange [-50:50]  # Ajusta min:max según tus datos
plot datafile using 1:11 with lines title "p", \
     datafile using 1:12 with lines title "q", \
     datafile using 1:13 with lines title "r"

pause -1 "Presiona Enter para continuar..."

# GRÁFICO 5: fuerzas
#set output "forces.eps"
set title "Fuerzas"
set ylabel "F [N]"
set yrange [-20000:20000]  # Ajusta min:max según tus datos
plot datafile using 1:14 with lines title "D", \
     datafile using 1:15 with lines title "Y", \
     datafile using 1:16 with lines title "L", \
     datafile using 1:17 with lines title "Xb", \
     datafile using 1:18 with lines title "Yb", \
     datafile using 1:19 with lines title "Zb"

pause -1 "Presiona Enter para continuar..."

# GRÁFICO 6: momentos
#set output "torques.eps"
set title "Momentos"
set ylabel "Momento [N·m]"
set yrange [-20000:20000]  # Ajusta min:max según tus datos
plot datafile using 1:20 with lines title "L", \
     datafile using 1:21 with lines title "M", \
     datafile using 1:22 with lines title "N"

pause -1 "Presiona Enter para continuar..."

# GRÁFICO 7: propulsión
#set output "empuje.eps"
set title "Propulsión"
set ylabel "F [N]"
set yrange [0:12000]  # Ajusta min:max según tus datos
set y2label "M [-] / F/F_SL [-]"
set y2range [0:1]
set y2tics
plot datafile using 1:40 with lines title "empuje", \
     datafile using 1:38 axes x1y2 with lines title "Mach", \
     datafile using 1:39 axes x1y2 with lines title "F/F_SL"

unset y2tics
unset y2label

pause -1 "Presiona Enter para salir..."
