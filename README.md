# Mya Acústica Analyzer

Prototipo de aplicación de medición acústica para macOS y Windows, desarrollado en C++17 y JUCE 8.0.12.

## Estado

La versión 0.3.6 incorpora adquisición simultánea de cuatro TF M/R, magnitud, fase, coherencia, IR real, Find/Track individual y general, RTA y espectrograma configurables, capturas de datos medidos y generadores reales de ruido rosa filtrable y sweep logarítmico. El loop físico se configura en su propia ventana. Los medidores principal, lateral y flotante ofrecen dBFS Peak y SPL Z/A/C Fast, Slow, Peak y Leq tras confirmar la calibración del canal. A/C son filtros digitales aproximados; Mya no está certificado como sonómetro IEC. El proyecto y las calibraciones se guardan en la carpeta de datos del usuario «Mya Acustica». La optimización IA, el DSP completo, Mya Mapp Vision 3D y la transferencia a procesadores siguen siendo fases posteriores. Las pruebas de software no sustituyen la validación acústica con equipo real. Consultar `Documentacion/07_Verificacion` para el alcance de cada entrega.

## Compilar

Requisitos: CMake 3.22 o posterior, compilador C++17 y fuentes de JUCE 8.0.12 obtenidas por separado desde su repositorio oficial, respetando su licencia. En macOS se necesita Xcode/Command Line Tools; en Windows, Visual Studio con herramientas C++.

```sh
cmake -S Aplicacion -B build -DJUCE_DIR=/ruta/a/JUCE-8.0.12 -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

ASIO en Windows es opcional y requiere su SDK: `MYA_ENABLE_ASIO=ON` y `MYA_ASIO_SDK_DIR`. No se incluye el SDK ni se declara validación Windows hasta probarlo.

## Migrar datos de 0.3.2

Con la aplicación cerrada y antes de abrir la 0.3.3 por primera vez, ejecuta `mya_migrate_data "<carpeta-antigua>"` desde una compilación local. La herramienta copia únicamente proyecto, preferencias, calibraciones, Biblioteca y Capturas a la carpeta estable del usuario. Conserva el origen y se detiene si el destino ya existe; en ese caso hay que revisar los datos antes de cualquier traslado manual. La herramienta no se incluye en el DMG.

## Organización

- `Aplicacion/src`: implementación.
- `Aplicacion/tests`: pruebas.
- `Documentacion`: requisitos, decisiones e informes propios.
- `docs`: estado del repositorio y coordinación.
- `.github`: plantilla de revisión.

Los manuales externos, mediciones personales, configuraciones locales, dependencias y compilaciones quedan fuera de Git.

## Trabajo

`main` contiene únicamente entregas revisadas, salvo el punto inicial de respaldo expresamente identificado como pendiente de aceptación. Cada cambio posterior se prepara en una rama y una pull request. No modificar decisiones confirmadas sin solicitud expresa de Antonio.

Ver `CONTRIBUTING.md` y `docs/estado-inicial.txt`. Repositorio independiente de cualquier otro proyecto del propietario.
