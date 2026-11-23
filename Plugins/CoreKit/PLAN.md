# Plan de CoreKit

CoreKit será un plugin reutilizable para iniciar proyectos nuevos de Unreal Engine. La primera validación será en UE 5.8.3 para Win64, en un proyecto de prueba limpio. SignalVeil aporta referencias de comportamiento y de trabajo.

Los módulos de runtime y el módulo de pruebas de Actions están implementados. Cada módulo tiene su guía en el [Quick Start](Docs/QuickStart.md). Las [skills de CoreKit](AGENTS.md#project-skills) están disponibles en el repositorio.

## Módulos y entregas

Un solo `CoreKit.uplugin` agrupará los módulos. Cada módulo tendrá su propia API C++ y Blueprint, con nombres de CoreKit y dependencias acotadas. El proyecto consumidor decide qué servicios usar y cómo coordinarlos.

| Fase | Módulo | Alcance inicial y verificación |
| --- | --- | --- |
| 1 | `CoreKitData` | Adaptar de DataDriven los contenedores y fragmentos de datos, el registro por tags y el descubrimiento de assets. Crear un Data Asset, consultar sus fragmentos desde Blueprint y verificar el descubrimiento en un build empaquetado. |
| 2 | `CoreKitNavigation` | Adaptar de UINavigation el foco UMG, el cambio de dispositivo y los glifos configurables. Probar teclado, mouse, mando y restauración de foco al cerrar un menú. |
| 2 | `CoreKitTween` | Interpolar valores para cualquier consumidor, con easing, pausa y cancelación. Validar una transición de menú y una puerta móvil. Para física simulada, el tween puede suministrar un objetivo a un controlador físico. El módulo funciona independientemente de UMG. |
| 3 | `CoreKitLoading` | Adaptar de CommonLoadingScreen la coordinación de cargas y la presentación configurable. Comprobar la liberación de la pantalla y del input al completar o fallar una transición. |
| 3 | `CoreKitSave` | Implementar un formato propio sobre `USaveGame`, con slots, metadatos, versionado y operaciones asíncronas. Probar continuar tras reiniciar, slots independientes y recuperación ante un fallo de escritura. EasyMultiSave sirve como referencia de comportamiento. |
| 4 | `CoreKitActions` | Adaptar ActionFramework para acciones locales con requisitos, conflictos, cancelación e IASA. Reutilizar sus casos de prueba para el ciclo de vida y la interrupción de una reproducción concreta de montage. |
| 5 | `CoreKitFrontend` | Proveer Nueva partida, Continuar y Ajustes dentro del plugin. El módulo coordina Save, Loading y Navigation. El proyecto configura mapas, participantes y presentación. |
| 6 | `CoreKitBasicUI` | Copia reutilizable de la UI de DeadDraw, con ViewModels y adaptadores del consumidor, menús/HUD/overlays y contenido propio del plugin. Ver `Docs/CoreKitBasicUI.md` y su evidencia de validación. |
| 7 | `CoreKitWeapons` | Armas y equipamiento reutilizables, con puntería libre o planar y ejemplos de pistola, rifle, escopeta y lanzamisiles. |

UINavigation `v3.15.14`, FCTween, CommonLoadingScreen y EasyMultiSave sirvieron como referencias de comportamiento. Navigation, Tween, Loading y Save tienen implementaciones propias. Data y Actions adaptan los contratos de DataDriven y ActionFramework, respectivamente.

La prueba de aceptación **menú → partida → guardado → continuar** pasó en un proyecto limpio empaquetado. Actions pasó su suite independiente de 20 pruebas. Consulta la [validación](Docs/Validation.md) para la cobertura por módulo.

La [ampliación](Docs/ExpansionPlan.md) añadió catálogos y restauración de mundo, carga con condiciones, reasignación de controles, secuencias de Tween, reglas de fragmentos y diagnóstico de Actions. El [proyecto de ejemplo](Docs/CoreKitExample.md) integra los módulos. La [validación de la ampliación](Docs/Validation.md#expansion) registra los casos probados.

La [guía de CoreKitFrontend](Docs/CoreKitFrontend.md) documenta cómo conectar Nueva partida, Continuar y Ajustes desde un proyecto vacío. El [registro de Frontend](Docs/Validation.md#frontend) reúne la cobertura y la prueba reproducible del juego empaquetado.

## Documentación y skills

`Docs/QuickStart.md` guiará a quien usa CoreKit desde un proyecto vacío. Cada módulo entregado tendrá un ejemplo mínimo con configuración, pasos en el Editor y resultado esperado. Las guías de `Docs/` serán la referencia para el comportamiento y la validación de cada módulo.

Las skills se versionarán en `.agents/skills/<nombre>/SKILL.md` y se enlazarán desde `AGENTS.md`. Cada una tendrá un disparador, un flujo corto y enlaces a las guías correspondientes. Las adaptaciones parten de las skills de SignalVeil y usarán rutas, ejemplos y contratos propios de CoreKit.

| Skill prevista | Referencia en SignalVeil | Cuándo incorporarla |
| --- | --- | --- |
| `corekit-maintainable-development` | `signalveil-maintainable-development` | Fase 1. Definir quién posee el estado, revisar consumidores y limitar el cambio a su módulo. Validar el target de Editor y el de juego cuando cambien dependencias o API reflejadas. |
| `corekit-blueprint-authoring` | `signalveil-blueprint-authoring` | Fase 1, con el primer ejemplo de Data. Inspeccionar clases y pines, editar el grafo de forma acotada, compilar, revisar visualmente y probar en PIE. Ampliar el flujo con Widget Blueprints al llegar a Navigation. |
| `corekit-quality-review` | `signalveil-quality-review` | Fase 1. Revisar el diff con evidencia, comprobar el caso principal y el fallo relevante, y registrar la validación pendiente. |

La skill `blueprint-semantic-extractor` pertenece al plugin de herramientas de SignalVeil. El Quick Start podrá enlazarla como flujo opcional cuando la herramienta esté instalada. Blueprint Assist también podrá documentarse como herramienta opcional de Editor.

