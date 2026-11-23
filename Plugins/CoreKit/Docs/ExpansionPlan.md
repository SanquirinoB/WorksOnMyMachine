# Plan de ampliación de CoreKit

El objetivo es que un proyecto nuevo pueda montar un menú, entrar a un mapa y continuar una partida con objetos persistentes. Esta iteración reúne las mejoras propuestas para la API actual y los patrones revisados en SignalVeil. La [validación actual](Validation.md) es el punto de partida para las regresiones.

Las siete etapas están implementadas. El [registro de la ampliación](Validation.md#expansion) recoge la cobertura en UE 5.8.3, incluida la prueba del ejemplo empaquetado.

## Orden y dependencias

| Orden | Etapa | Resultado |
| --- | --- | --- |
| 1 | [Save](#1-save) | Menú de slots y restauración de actores inscritos. |
| 2 | [Loading](#2-loading) | Viaje de mapa con confirmación de que la partida está lista. |
| 3 | [Navigation](#3-navigation) | Menús que conservan el foco, actualizan glifos y permiten reasignar controles. |
| 4 | [Tween](#4-tween) | Interpolación por reloj, secuencias y cancelación por dueño y canal. |
| 5 | [Data](#5-data) | Catálogos con carga selectiva y validación por tipo de asset. |
| 6 | [Actions](#6-actions) | Diagnóstico en PIE y un ejemplo de intención sostenida. |
| 7 | [Proyecto de ejemplo](#7-proyecto-de-ejemplo) | Proyecto editable con Blueprints y un módulo C++ consumidor. |

Save posee el estado de almacenamiento y restauración. Loading posee el viaje, las solicitudes de carga y el overlay. El proyecto consumidor conecta los resultados de Save con las condiciones de Loading. Navigation y Tween exponen servicios que los widgets consumen por separado. Las herramientas de depuración de Actions se alojan en Editor y el adaptador de intención vive en el ejemplo. Los módulos de runtime conservan sus límites de dependencia.

Cada etapa requiere compilar Editor y Game Development en **UE 5.8.3 para Win64**, ejecutar sus casos de aceptación en un proyecto limpio y actualizar la guía del módulo con los resultados. Los casos de foco y presentación requieren una pasada con render. La carga por catálogo y la restauración tras reiniciar requieren un build empaquetado. Cerrar las regresiones de la etapa antes de iniciar la siguiente.

## 1. Save

1. Incorporar `List Slots` y `Get Slot Info` asíncronos. Consolidar las dos copias de cada slot y ordenar por fecha de la revisión disponible. La consulta entrega metadatos sin deserializar el objeto `SaveGame`. Distinguir un estado leído desde metadatos de uno confirmado mediante la validación del registro completo. La carga informa si recurrió a una copia de respaldo y conserva la lectura del formato de almacenamiento actual.
2. Agregar un contrato opt-in de captura y restauración para actores con `FGuid` estable. Guardar identidad de instancia y datos versionados por participante. Validar GUID duplicados antes de capturar el slot. Resolver participantes de forma diferida al estar disponible su nivel y devolver un resultado de restauración separado del resultado de lectura de disco. Para la demo, un actor ausente se omite con un diagnóstico y un duplicado se rechaza antes de aplicar cambios.

**Aceptación.** Listar slots con dos copias, recuperar un slot con una copia dañada, leer un slot escrito por la versión actual y mantener el último guardado tras un fallo de escritura. Reiniciar el ejecutable y restaurar una puerta por GUID, probar la llegada tardía de un participante y comprobar los casos de actor ausente y GUID duplicado.

## 2. Loading

1. Agregar `Open Level With Loading`. El nodo valida el destino, muestra el overlay y despacha el viaje después de que el overlay tenga oportunidad de dibujarse. El viaje y la preparación del destino reportan resultados separados. Al terminar o fallar, liberar la solicitud del viaje y restaurar foco e input cuando se cierre la última solicitud.
2. Agregar condiciones de disponibilidad como `MapLoaded`, `PlayerReady` y `RestoreComplete`. El proyecto declara las condiciones necesarias y comunica cuándo se cumplen o fallan. Exponer un evento de visibilidad y diagnósticos de las solicitudes activas con dueño, motivo y tiempo transcurrido. Vincular las señales de disponibilidad al viaje que las originó para descartar respuestas antiguas.

**Aceptación.** Ver el overlay antes del viaje en un proyecto con render. Tras cargar el mapa, mantenerlo visible hasta completar una restauración diferida. Probar destino inválido, fallo de restauración, evento tardío de otro viaje y dos solicitudes simultáneas. Comprobar la liberación de input y la restauración de foco.

## 3. Navigation

1. Crear un widget de glifo que se actualice al cambiar el dispositivo y al reconstruirse las asignaciones de Enhanced Input. Usar identificadores para asociar el foco a cada menú y resolver el mejor foco válido cuando se cierre o destruya fuera del orden habitual.
2. Añadir captura de tecla por perfil y contexto, con restricciones por dispositivo y detección de conflictos en contextos que pueden coincidir. Ofrecer **Intercambiar**, **Cancelar** y **Restaurar valores**. Validar el cambio completo antes de aplicar un intercambio y usar la configuración de usuario de Enhanced Input para persistirlo.

**Aceptación.** Abrir y cerrar tres menús en distinto orden. Verificar teclado, mouse y mando, cambio de glifo tras reasignar y persistencia al reiniciar. Probar conflicto real, tecla compartida entre contextos excluyentes, cancelación y restauración a los valores iniciales.

## 4. Tween

Agregar elección de reloj de juego o tiempo real por tween, secuencias cancelables y operaciones por dueño y canal. Definir políticas explícitas para rechazar, reemplazar o permitir otro tween en el mismo canal. Una secuencia posee sus pasos y cancela el activo y los pendientes al finalizar por cancelación. El consumidor aplica el alpha a la opacidad, un transform o un objetivo para un controlador físico.

**Aceptación.** Animar el menú durante pausa con tiempo real y comprobar que la puerta con reloj de juego respeta pausa y dilatación. Ejecutar una secuencia, cancelarla durante un paso y reemplazar el tween de un canal mientras otro del mismo dueño sigue activo. Verificar un solo resultado terminal y limpieza al destruir el dueño.

## 5. Data

1. Separar el descubrimiento de la carga. Consultar un catálogo por clase y ruta para devolver referencias blandas en orden estable, y cargar de forma asíncrona solo las seleccionadas. Usar las reglas de cook del proyecto para los assets que se encuentren únicamente por catálogo.
2. Permitir que cada tipo de contenedor declare clases de fragmento requeridas y cardinalidad. La validación del asset informa qué fragmento falta o cuál excede el límite. Las búsquedas siguen usando el orden de autoría para decidir la primera coincidencia.

**Aceptación.** Detectar un fragmento obligatorio ausente y un duplicado inválido en Editor. En el juego empaquetado, descubrir un catálogo sin cargar todos sus objetos, cargar una selección, manejar una referencia ausente y comprobar el orden de los fragmentos.

## 6. Actions

1. Crear un panel de depuración en PIE que muestre la acción activa, la solicitud en cola, el motivo de rechazo y el dueño de cada bloqueo de IASA. Reutilizar el historial de solicitudes y los datos del coordinador para identificar la ejecución observada.
2. Crear un ejemplo de intención sostenida con un botón de bloqueo. Mantener la intención mientras la tecla siga presionada, reevaluar por la admisión normal cuando cambie IASA y limpiar la intención al soltar, destruir el dueño o cambiar de mundo. Distinguir una reanudación de una pulsación nueva para evitar repetir efectos de inicio.

**Aceptación.** Mantener pulsado durante un bloqueo, soltar antes de recuperar el control, mantener dos bloqueos independientes y cancelar durante un cambio de mundo. Verificar el historial mostrado en PIE y volver a ejecutar la suite existente de 20 casos, incluidos los de identidad de reproducción de montage.

## 7. Proyecto de ejemplo

Entregar un proyecto separado que use el plugin y se pueda abrir desde el Quick Start. Incluye un menú de **Nueva partida**, **Continuar** y **Controles**, un catálogo de interacciones, una puerta con GUID que se anima con Tween, un checkpoint y un controlador con el ejemplo de intención sostenida. Los Blueprints configuran los assets editables y el módulo C++ del proyecto coordina las llamadas a CoreKit. Consulta la [guía del ejemplo](CoreKitExample.md).

**Aceptación final.** Desde un build empaquetado, reasignar un control y verificar su glifo, abrir la puerta, guardar, cerrar el proceso y elegir **Continuar**. Comprobar que la puerta recupera su estado antes de que Loading devuelva el control. Repetir con la copia más reciente dañada y con el actor de la puerta ausente, verificando el resultado que muestra el menú.
