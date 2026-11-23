# CoreKitBasicUI

Módulo Runtime de presentación reutilizable, extraído de la UI nativa de DeadDraw. Está en esta copia de CoreKit y no requiere instalar DeadDraw, CombatKit, AttributeSystem ni RewardSystem. Conserva el estilo visual industrial, las animaciones y las protecciones de dibujo de la versión fuente.

## Qué contiene

| Área | Componentes |
| --- | --- |
| Base | Estilo, fuentes, helpers de construcción UMG, números tabulares, botones, keycaps y arcos Slate |
| Frontend | Menú principal, ajustes con aplicar/descartar, ayuda, créditos, pantalla de carga y wipe |
| Partida | HUD de salud, recursos, munición, habilidades, progreso, tiempo, minimapa, cartas y avisos |
| Overlays | Pausa, elección de cartas, confirmaciones, victoria y derrota |
| Mundo | Barras de salud y anillos de carga para cualquier actor con un componente de presentación |
| Feedback | Eventos transitorios, números flotantes con pool y efectos opcionales de cámara |
| Contenido | 491 assets en `/CoreKit/BasicUI`: fuentes, texturas, iconos, cartas, materiales, curvas, glifos y sonidos |

Los adaptadores que leían el jugador, enemigos, armas y progresión de DeadDraw fueron reemplazados por contratos para el consumidor. La lógica de daño, recompensas, pausado del mundo y reglas de partida pertenece a tu juego. Las cartas e instrucciones heredadas son un preset de presentación: no implementan esos poderes.

## Instalación y configuración

1. Instala CoreKit completo en `Plugins/CoreKit`, incluyendo **Content** y **Resources**. Usa un proyecto C++ con UE 5.8. Se habilitan los plugins de motor Enhanced Input y Model View ViewModel desde el descriptor.
2. Compila Editor y activa **Project Settings → Game → CoreKit Basic UI → Enabled**. El módulo está desactivado por defecto para no aplicar ajustes ni crear contextos UI en un juego que no lo usa.
3. Configura los títulos, tagline, equipo, duración de sesión y mapa de juego. Puedes usar el menú nativo o abrir pantallas desde tu controlador existente.
4. Fusiona [el ejemplo de configuración](../Samples/BasicUI/DefaultGame.example.ini) con `Config/DefaultGame.ini`. Incluye `/CoreKit/BasicUI` en **Packaging → Additional Asset Directories to Cook**: los estilos cargan fuentes, texturas y sonidos por nombre. Añade también tus mapas al cook.
5. Si usas los glifos incluidos, asigna las tres referencias de `CoreKitNavigationSettings` del ejemplo. Configura `InputMapping` y el mapa `InputActions` con los assets de tu propio juego; sin ellos los widgets muestran sus teclas de fallback y no reasignan controles de gameplay.

Las licencias OFL y la procedencia del contenido están en `Resources/BasicUI/ThirdParty`. El módulo las añade como archivos NonUFS en los paquetes. No elimines esa carpeta al distribuir el plugin.

## Menú principal

Para un nivel de título independiente, usa `ACoreKitBasicUIMenuGameMode`. No crea pawn; su controlador abre el menú, puede usar una cámara del nivel etiquetada `MenuCamera` y reproduce `MenuMusic` si lo configuras. PLAY abre `GameplayMap` cuando CoreKitFrontend está deshabilitado.

Con tu propio controlador, llama al nodo **Open Main Menu** de `CoreKitBasicUILibrary`. También existen **Open Settings**, **Open How To Play** y **Open Credits**. Las pantallas usan la pila de CoreKitNavigation; Back cierra la pantalla superior y devuelve el foco a la anterior. No cambies el input mode desde un segundo administrador de menús mientras esa pila posea el input.

Para viajes gestionados, configura CoreKitFrontend con tus dos mapas y:

```ini
[/Script/CoreKitFrontend.CoreKitFrontendSettings]
bEnabled=True
MenuWidgetClass=/Script/CoreKitBasicUI.CoreKitBasicUIFrontEndHost
LoadingWidgetClass=/Script/CoreKitBasicUI.CoreKitBasicUILoadingScreen
```

El host es una implementación interna seleccionable por configuración. Loading conserva su gestión de input y readiness. El glue de BasicUI añade el wipe y un hold de 0,2 segundos reales tras cargar. No modifica los ajustes de los otros módulos al cargarse.

## Conectar el HUD a otro juego

Asigna `ACoreKitBasicUIHUD` como HUDClass del GameMode de partida, o deriva de él. Crea el layout y el router para el jugador local. Desde Blueprint usa **Get UI Context** con tu PlayerController; desde C++:

```cpp
#include "CoreKitBasicUI/CoreKitBasicUILibrary.h"
#include "CoreKitBasicUI/Context/CoreKitBasicUIContextSubsystem.h"

if (auto* UI = UCoreKitBasicUILibrary::GetUIContext(PlayerController))
{
    UI->GetVitals()->SetHealth(CurrentHealth);
    UI->GetVitals()->SetMaxHealth(MaxHealth);
    UI->GetLoadout()->SetAmmo(CurrentAmmo);
    UI->GetProgression()->SetXPFraction(XPProgress);
    UI->GetRun()->SetRemainingSeconds(TimeRemaining);
}
```

Los setters están expuestos a Blueprint y conservan FieldNotify. Actualiza los modelos cuando cambien los datos del juego. Los widgets leen esos modelos; las animaciones usan tiempo real y continúan con el mundo pausado. Cada jugador local tiene sus propias instancias; las referencias globales MVVM son opcionales y llevan sufijo `.PlayerN`.

Para un adaptador duradero, deriva de `UCoreKitBasicUIPresenter` y añádelo a `PresenterClasses`. En Blueprint implementa **Receive Start**, **Receive Tick** y **Receive Stop**; en C++ puedes sobrescribir Start/Tick/Stop. Start recibe el contexto y controlador; Stop debe liberar las suscripciones que creó tu adaptador. El HUD inicia/detiene los presenters en BeginPlay/EndPlay.

## Acciones y overlays

- Enlaza **On Command Requested** del contexto. Los comandos son `TogglePause`, `Restart` y `QuitToMenu`. Tu juego ejecuta la acción y actualiza el Run ViewModel. Para pausa, actualiza `bPaused` y pausa/reanuda el mundo desde tu lógica.
- El router presenta pausa con `Run.bPaused`, las pantallas finales con `Run.Phase = Victory/Defeat`, y cartas cuando `Progression.bOfferOpen` está activo. Las confirmaciones usan la capa Modal y tienen prioridad sobre los menús.
- Para cartas, rellena `Offer` con `FCoreKitBasicUICardData`, cambia `OfferSerial` y activa `bOfferOpen`. `ArtScene` y `ArtEffect` son texturas opcionales; sin ellas se dibuja un placeholder. Enlaza **Set Card Choice Handler** con una función Blueprint que recibe el índice y devuelve bool. Devuelve true solo después de aplicar una elección válida y actualizar los modelos. El widget envía una sola petición al terminar la animación; si el consumidor devuelve false, permite elegir de nuevo.
- En C++ también puedes enlazar `Run->TogglePause/Restart/QuitToMenu` y `Progression->ChooseCard` directamente desde tu presenter. Estos enlaces sustituyen el puente de eventos del contexto.

El módulo no decide qué guarda una partida, qué efecto aplica una carta ni cuándo termina un combate. Usa Summary para rellenar las estadísticas de victoria/derrota y Build para las cartas poseídas.

## Eventos, ajustes y señales de mundo

`UI->GetEvents()` publica payloads tipados: `FCoreKitBasicUIDamageTakenEvent`, `FCoreKitBasicUIHitEvent`, pickups, cambios de arma, nivel y jefe. Sus tags están bajo `CoreKit.BasicUI.Event.*`. En C++ usa `Publish(CoreKitBasicUIEvent::DamageTaken, Payload)`; en Blueprint, **Publish** recibe un Instanced Struct. Las suscripciones nativas se liberan al destruir los widgets.

`UCoreKitBasicUIUserSettings::Get()` guarda la configuración en la sección propia de GameUserSettings. Video y audio se aplican mediante el motor/CoreKitFrontend; para sensibilidad, inversión Y, FOV y otras opciones de gameplay, lee y aplica los valores al iniciar tu adaptador y enlaza **On Applied** para los cambios posteriores. Los snapshots de aplicar/descartar copian solo propiedades Config, conservando las suscripciones. Los efectos de cámara requieren `bEnableCameraEffects`; están apagados por defecto.

Para barras/anillos de mundo, activa `bEnableWorldSignals`, añade `UCoreKitBasicUIEnemySignalComponent` a cualquier actor y llama a **Set Health** y **Set Charge** desde tu sistema. No detecta enemigos por clase ni cambia materiales de gameplay automáticamente. El subsistema mantiene una capa de proyección por jugador local y limpia referencias al descargar el mundo.

Los números flotantes son independientes: obtén `UCoreKitBasicUIFloatingTextSubsystem`, llama a **Configure** con `/CoreKit/BasicUI/FloatingText/DA_DamageNumbers` y usa **Show Text** o **Show Screen Text** con el StyleId. Los estilos heredados `DDRifle`, `DDKatana` y `DDZap` son identificadores de presentación, sin dependencia de sus armas originales.

El minimapa incluido es un ejemplo visual. `/CoreKit/BasicUI/Minimap/DA_Minimap_Example` usa la textura copiada y conserva sus bounds, pero no referencia el mapa de DeadDraw. Para otro nivel crea tu propia definición y alimenta el Minimap ViewModel con tu posición, entidades, textura y bounds.

## Personalización

Los títulos y datos vienen del consumidor. `TextOverrides` permite cambiar todos los textos heredados sin editar C++; [BasicUITextKeys.json](BasicUITextKeys.json) lista las claves `Namespace.Key` y sus valores por defecto. Puedes reemplazar las instrucciones, nombres de categorías, etiquetas del HUD y créditos para tu juego. La paleta y las formas conservan el preset visual de DeadDraw; los estilos están en `UCoreKitBasicUIStyle` y los helpers C++ en `CoreKitBasicUIBuild`.

Los widgets se construyen nativamente cuando el WidgetTree está vacío. Puedes derivar de los widgets o del layout para especializar la composición. Los widgets pasivos no toman foco ni capturan clicks; la pila de navegación administra los menús. Las señales fuera de pantalla usan los límites del viewport; las cartas recortan su arte dentro de UV válidas.

## Dependencias y validación

`CoreKitBasicUI` usa UMG/Slate, MVVM/FieldNotification, EnhancedInput, GameplayTags, DeveloperSettings y CoreKitNavigation. Frontend/Loading/Save, RenderCore y AudioMixer quedan como dependencias privadas. No hay dependencias circulares hacia los módulos existentes.

Consulta [la evidencia de validación](BasicUIValidation.md). El proyecto de validación y sus archivos generados se mantienen fuera del checkout del plugin. La extracción no cambia el código ni los assets de la UI original de DeadDraw.
