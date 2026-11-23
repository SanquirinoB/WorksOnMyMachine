# Validación de CoreKitBasicUI

Validación de la extracción realizada el 18 de noviembre de 2026, en la copia `C:/Users/Deltalo/MachineVault/CoreKit`, con Unreal Engine **5.8.4**, Win64 Development y D3D12.

## Resultado

| Puerta | Resultado |
| --- | --- |
| Editor del consumidor limpio | Compilado correctamente, incluyendo UHT y todas las unidades del módulo |
| Juego Win64 Development | Compilado correctamente |
| Automatización final | 9 pruebas correctas, 0 fallos, 0 pruebas con warnings |
| Contenido | 491 paquetes / 27.236.326 bytes en `Content/BasicUI` |
| Dependencias de los assets | 0 referencias a `/Game`, `/Script/DeadDraw*` o `/Script/CombatKit*` |
| Cook + stage + Pak/IoStore + archive | Correctos, con símbolos de depuración excluidos |
| Licencias OFL en el paquete | Presentes para las cuatro familias de fuentes |
| Recorrido en el ejecutable empaquetado | `BASICUI_SMOKE_COMPLETE failed=0 picks=1` |
| Fuentes originales de la extracción | Hashes SHA-256 conservados; módulos existentes de CoreKit sin cambios de código |

El consumidor de prueba solo instala CoreKit y plugins del motor. No instala DeadDraw, CombatKit, AttributeSystem ni RewardSystem. Las antiguas copias temporales de `/Game/DeadDraw/UI` y sus redirectors se retiraron del Content del consumidor antes de la aceptación final, para que no ocultaran referencias rotas.

Los assets propios de DeadDraw que tenían schemas dependientes del juego se reconstruyeron con clases del módulo: tabla/definición de números flotantes y definición del minimapa. Esta última conserva la textura y los bounds como ejemplo, sin referencia al nivel fuente. El resto se trasladó con AssetTools y se guardó con sus referencias actualizadas.

## Cobertura

Las pruebas `CoreKit.BasicUI.*` cubren:

- Modelos separados entre dos contextos de jugador y despacho de comandos al consumidor.
- Suscripción, payload tipado y retirada de suscripciones del canal de eventos.
- Carga y uso de los schemas propios de números flotantes y minimapa sin plugins de gameplay.
- Formato del reloj y cuerpos de confirmación.
- Separación del texto resaltado de las cartas, recorte UV y tamaños de variantes.
- Bloqueo inicial y confirmación única de una elección de carta.
- Prioridad del router entre oferta, pausa, modal y pantallas finales.

El recorrido renderizado del ejecutable empaquetado abre menú, ajustes, ayuda y créditos, activa el binding PLAY, viaja al HUD, comprueba cursor de gameplay, pausa el mundo desde el consumidor, presenta una oferta de tres cartas, elige mediante el widget real, confirma una vez, reanuda el mundo, muestra un número flotante y abre victoria/derrota antes de volver al menú. Las animaciones de pausa/cartas continúan con el mundo pausado. Se inspeccionaron las capturas de las pantallas resultantes.

Durante la extracción se corrigió el fallback sin CoreKitFrontend: retira el menú antes de viajar y limpia las pantallas propiedad del controlador saliente. La prueba empaquetada final verifica el cursor restaurado. El ajuste no modifica la UI del proyecto original.

## Evidencia reproducible

Proyecto y archivos generados, fuera del plugin:

`C:/Users/Deltalo/MachineVault/CoreKitBasicUIValidation/Project/BasicUIValidation.uproject`

Paquete de aceptación:

`C:/Users/Deltalo/MachineVault/CoreKitBasicUIValidation/Build/Windows/BasicUIValidation.exe`

Logs e informes en `C:/Users/Deltalo/MachineVault/CoreKitBasicUIValidation/Evidence`:

- `EditorBuildFinalAccepted.log`
- `PackageFinal.log`
- `AutomationAccepted/index.json`
- `PackagedSmoke.log`
- `MigratedAssetDependencies.json`

Los comandos de aceptación fueron Build del target Editor, `Automation RunTests CoreKit.BasicUI`, y BuildCookRun con `-build -cook -stage -pak -iostore -archive -nodebuginfo`. El ejecutable de prueba admite `-BasicUISmoke -RenderOffscreen -windowed -NoSound -unattended` para repetir el recorrido sin manejar el escritorio del usuario.

Dentro del plugin quedan el [inventario de referencias](BasicUIAssetDependencies.json), los [hashes de procedencia](BasicUISourceManifest.json), las [claves de textos](BasicUITextKeys.json) y las [capturas](BasicUIPreviews/01_Menu.png). El código de las pruebas está en `Tests/CoreKitBasicUI`.

## Límites de esta validación

El recorrido usa un adaptador de datos de ejemplo y ejecuta los bindings de los widgets; no es una partida de DeadDraw ni una prueba de clics físicos, mando o reasignación contra acciones de otro juego. Se ejecutó con NoSound: el contenido de audio se migró y se cocinó, pero no se validó su mezcla mediante escucha. La sesión visual capturada usó un viewport de 888×500 con escalado UMG; no constituye una revisión de todas las resoluciones.

El recorrido empaquetado verifica el modo independiente de BasicUI. La configuración opcional del host CoreKitFrontend está documentada como punto de integración; sus readiness gates, guardados y viajes particulares deben verificarse con el consumidor que los configure. Las señales de mundo y efectos opcionales de cámara compilan, pero no hubo una escena de combate real para evaluar todas sus variantes.

Los logs intermedios de trabajo conservan los intentos anteriores. La aceptación corresponde exclusivamente a los archivos finales enumerados y al código cuyo inventario está en `BasicUIValidation.json`.
