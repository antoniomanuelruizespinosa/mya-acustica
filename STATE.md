# Estado de control de Mya Acústica

## Arquitectura visual autorizada

- Los únicos módulos oficiales visibles son **Mya Analyzer**, **Mya DSP** y **Mya 3D**. El tercer módulo permanece deshabilitado hasta su desarrollo.
- No reactivar pestañas, botones de modo ni menús de módulos antiguos eliminados. El rótulo actual del tercer módulo en el código es «Mya Mapp Vision 3D»; cualquier ajuste de nombre requiere revisión aparte.

## Áreas protegidas

- La calibración asociada al número de serie del dispositivo, la suma vectorial compleja y el generador de ruido rosa independiente son bloques protegidos. No modificar su lógica, sus callbacks de audio ni los búferes de entrada/salida como parte de una corrección geométrica.
- Preservar el eje X logarítmico, las alarmas de los medidores y la separación entre Mya Analyzer y Mya DSP.

## Protocolo para cambios futuros

1. Inspeccionar y presentar una propuesta acotada antes de escribir código. Esperar aprobación explícita del ingeniero.
2. Crear un `git worktree` temporal a partir del HEAD de `feature/mya-dsp-framework`, con una rama de prueba independiente, para cualquier corrección de código. La rama original ya está ocupada por el taller principal y no debe compartirse entre dos worktrees.
3. Compilar y ejecutar las pruebas en ese worktree. La verificación física del ingeniero precede a cualquier integración o cambio de la versión de producción.
4. No alterar el acceso directo del Escritorio, ni hacer commit, push o fusión sin una autorización posterior específica.

## Línea de base de esta auditoría

- Rama inspeccionada: `feature/mya-dsp-framework`.
- HEAD al crear este archivo: `b4fc07d`.
- Menú de macOS: nativo (`MenuBarModel::setMacMainMenu`); no hay `MenuBarComponent` dentro de la ventana cuya altura deba restarse.
- `MainLayout.cpp` no existe en esta revisión: la jerarquía principal y su método `resized()` están en `Aplicacion/src/Main.cpp`.
- Estado del desfase físico del ratón: **reproducido, medido y corregido en el worktree de geometría**. La inspección física del ingeniero aprobó la alineación del binario QA; no aplicar compensaciones de altura sin medir el marco nativo y el evento en el componente real.
