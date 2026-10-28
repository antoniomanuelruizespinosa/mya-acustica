# Mya Acústica Analyzer

Prototipo privado de aplicación de medición acústica para macOS y Windows, desarrollado en C++17 y JUCE 8.0.12.

## Estado

La versión 0.3.3 adquiere entradas dBFS y calcula magnitud, fase y coherencia de una TF M/R seleccionada. Incluye loop rosa interno y físico, con canal de referencia configurable por TF; Input 5 es solo una ruta de ejemplo. El proyecto, las preferencias, las calibraciones y la biblioteca se guardan en la carpeta de datos del usuario «Mya Acustica», independiente de la ubicación del código. La migración desde una instalación anterior requiere indicar explícitamente la carpeta antigua a `mya_migrate_data`; conserva el origen y no sobrescribe un destino existente. La IR, las demás TF visibles, la optimización IA, el DSP y la transferencia a procesadores siguen siendo demostraciones o funciones pendientes. La adquisición, la calibración y las medidas aún requieren validación acústica completa. Consultar `Documentacion/07_Verificacion` para el alcance de cada entrega. Compilar y superar pruebas no equivale a aceptación visual ni validación acústica.

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
