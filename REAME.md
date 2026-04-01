# Información
Práctica de Gráficos por Ordenador MUII

# Requisitos previos
1. Los drivers con aceleración 3D del fabricante con soporte para OpenGL
2. Un entornos de programación para lenguaje C/C++. En linux con tener el GCC y G++ podría ser suficiente. 
Se pueden utilizar entornos como VScode o CLion. 
En Windows se aconseja Visual Studio 2019/2022 y en Mac XCode. Otros compiladores como MinGW tamibén podrian funcionar.
3. CMake instalado y accesible desde el terminal o consola de comandos.

# Puesta en funcionamiento del proyecto
1. Una vez descomprimido el zip y acceder mediante el terminal al directorio build.
En entornos Linux con GCC/G++ instalado ejecutar
cmake .. -DOpenGL_GL_PREFERENCE=GLVND
make -j4

2. En entornos Windows con Visual Studio 2019/2022
Abrir diréctamente el fichero de solución de VS y compilar

#  Ejecución
En el directio /build/bin se habrán generado los binarios correspondientes al proyecto.
Se pueden lanzar haciendo click sobre ellos o desde el entorno de programacion.
