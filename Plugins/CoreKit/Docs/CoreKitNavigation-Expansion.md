# CoreKitNavigation — expansión hacia paridad funcional con UINavigation

Estado: **diseño aprobado para implementación incremental** (2026-11-15). Primer consumidor: la UI final de DeadDraw (`Docs/Architecture/DeadDraw-UI-ImplementationPlan.md` del proyecto), donde CoreKitNavigation **reemplaza a CommonUI**.
Referencia de comportamiento: UINavigation 3.15.x (goncasmage1). Como en el resto de CoreKit, es una implementación propia: no se copia código ni assets de UINavigation.

## Principios
- Contrato pequeño y reutilizable; nada específico de DeadDraw.
- Estado mutable en el `UCoreKitNavigationSubsystem` (por LocalPlayer) o en el menú que lo posee; configuración en `UCoreKitNavigationSettings` (Developer Settings) y data assets.
- Sin dependencia de CommonUI. Dependencias nuevas solo privadas cuando sea posible.
- Compatibilidad: la API actual (`PushMenuFocusScope`, `FindActionGlyph`, `UCoreKitNavigationGlyphs`, `UCoreKitActionGlyphWidget`, `UCoreKitKeyBindings`, captura de tecla) sigue funcionando sin cambios para CoreKitFrontend.
- La navegación direccional sigue siendo la de UMG (reglas de navegación de cada widget); CoreKit no reimplementa el modelo de selección de UINavigation.

## Inventario y alcance

| Área de UINavigation | Equivalente CoreKit | Estado |
|---|---|---|
| `UINavInputProcessor` (dispositivo) | `FCoreKitNavigationInput` | Existe |
| `ReturnToParent` (restaurar foco) | Focus scopes | Existe |
| `UINavInputBox`/`InputCollisionData`/`SwapKeysWidget` (rebinding) | `UCoreKitKeyBindings` + captura | Existe |
| `InputIconMapping`/`InputNameMapping`/`UINavInputDisplay` | `UCoreKitNavigationGlyphs` (solo ícono) | **Ampliar (N1)** |
| `PlatformConfigData` (íconos por plataforma) | — | **Nuevo (N1)** |
| `GoToWidget`/`ReturnToParent`/`UINavPCComponent` (pila, input mode, cursor) | — | **Nuevo (N2)** |
| Return/Back, permisos de input, `WidgetTransitionInputCooldown` | — | **Nuevo (N2)** |
| `AutoHideMouse` | — | **Nuevo (N2)** |
| `UINavSectionsWidget`, next/previous section | — | **Nuevo (N3)** |
| `UINavPromptWidget`, `GoToPromptWidget` | — | **Nuevo (N4)** |
| `UINavButtonBase`, eventos de navegación, sonidos | — | **Nuevo (N5)** |
| `UINavOptionBox`/`UINavSlider`/`UINavSliderBox` | — | **Nuevo (N6)**: Settings de DeadDraw tiene 9 filas segmentadas, 7 sliders y 2 selectores operables con mando |
| Hold behaviors, cursor por stick, widgets 3D/VR, viewport client propio | — | Fuera de alcance inicial |

## N1 — Glifos por dispositivo y tipo de mando

> **Estado (2026-11-15): implementado.** Compila en DeadDrawEditor; tests `CoreKit.Navigation.IconSetLookup`, `.GamepadTypeResolution`, `.KeyPresentation` en verde. Pendiente de prueba manual: detección real de DualSense/Switch con hardware conectado y `FindActionPresentation` con un jugador real (no cubierto por tests).

- `UCoreKitInputIconSet` (DataAsset): `TMap<FKey, FCoreKitKeyPresentation>` con ícono (`TSoftObjectPtr<UTexture2D>`) y texto (`FText`), más `FCoreKitKeyPresentation FindKey(FKey)`. `UCoreKitNavigationGlyphs` se mantiene.
- `UCoreKitNavigationSettings` (Developer Settings, `Config=Game`): set de teclado/mouse, sets de gamepad por tipo (`TMap<FName, TSoftObjectPtr<UCoreKitInputIconSet>>`: `Xbox`, `PlayStation`, `Switch`, `Generic`), tipo por defecto, y reglas de identificación (`FName` del hardware → tipo).
- Subsystem: `GetGamepadType()`, override del jugador `SetGamepadTypeOverride(FName)` (para la opción "Gamepad glyphs" de Settings), evento `OnGamepadTypeChanged`. Detección con `UInputDeviceSubsystem::GetMostRecentlyUsedHardwareDevice` / `OnInputHardwareDeviceChanged`. En Windows sin GameInput todo gamepad aparece como XInput: se usa el tipo por defecto o el override.
- Resolución: `FindActionPresentation(Action)` → `FCoreKitKeyPresentation` del dispositivo/tipo actual; `FindKeyPresentation(Key)`. `UCoreKitActionGlyphWidget` usa la configuración global cuando no tiene `Glyphs` asignado.
- Nuevo `UCoreKitInputPrompt` (UUserWidget nativo con `BindWidgetOptional` de `UImage` + `UTextBlock`): muestra ícono o, si falta, el texto; se refresca con dispositivo, tipo de mando y cambios de mapeo.

## N2 — Pila de menús, input mode y Return

> **Estado (2026-11-15): implementado (N2–N6).** Cambio frente al diseño: Return y las secciones se leen en el pre-procesador de Slate del subsystem (listas de teclas en settings) en lugar de un contexto de Enhanced Input, porque en modo UI Only el viewport no recibe teclas y el pre-procesador funciona también con el juego en pausa. Estas teclas no son rebindeables (no lo pide la spec).

- `UCoreKitMenuWidget` (UUserWidget base): `GetInitialFocus()`, `HandleReturn()` (BlueprintNativeEvent; por defecto cierra el menú), `OnMenuActivated/Deactivated`, y política: `InputMode` (`Game`, `GameAndMenu`, `Menu`), `bShowCursor`, `bAllowDirectional/Select/Return/Section`, `ActivationInputCooldown` (segundos).
- Subsystem: `PushMenu(Menu, Layer)`, `PopMenu(Menu)`, `GetTopMenu(Layer)`, capas por `FName` con prioridad (la más alta con menús recibe input). Al cambiar el menú superior aplica input mode y cursor al PlayerController y restaura los del juego al vaciarse; reutiliza los focus scopes existentes para el foco.
- Return/Section: contexto de Enhanced Input propio (`UInputMappingContext` en settings, acciones `Return`, `NextSection`, `PreviousSection`) añadido mientras haya menús, con un `UEnhancedInputComponent` del subsystem empujado al PlayerController. Las teclas son rebindeables con `UCoreKitKeyBindings`. La captura de tecla en curso tiene prioridad (su pre-processor consume Escape).
- Cooldown: durante `ActivationInputCooldown` el menú ignora Select/Return y los botones N5 no aceptan clic.
- Auto-ocultar cursor al usar gamepad y volver a mostrarlo con el mouse cuando la política del menú lo permite.

## N3 — Secciones (tabs)
- `UCoreKitMenuWidget::HandleSection(int32 Delta)` (BlueprintNativeEvent) y `UCoreKitSectionList` (UUserWidget nativo): lista de botones N5 enlazada opcionalmente a un `UWidgetSwitcher`; responde a `NextSection`/`PreviousSection` del menú superior y a clic/foco. Evento `OnSectionChanged`.

## N4 — Prompts de confirmación
- `UCoreKitPromptWidget` (subclase de `UCoreKitMenuWidget`): título, cuerpo, opciones (confirmar/cancelar o N opciones), `OnPromptDecided(int32 Option)`; Return = cancelar. `PushPrompt(Class, Title, Body, Options, Callback)` en el subsystem.

## N5 — Botones y eventos de navegación
- `UCoreKitButton` (UButton nativo): unifica hover y foco (pasar el mouse enfoca; el foco por teclado/gamepad aplica el estilo hovered) para que exista un único "seleccionado"; eventos `OnSelected/OnDeselected/OnActivated`; sonidos de navegación/confirmación desde settings; respeta cooldown y `bAllowSelect` del menú.

## N6 — Controles de valor
- `UCoreKitOptionSelector` (UUserWidget nativo): opciones `FText`, índice, modos segmentado y ◀ valor ▶; izquierda/derecha (teclado, D-pad, stick) cambian el valor cuando el selector o su fila tiene el foco; clic directo en un segmento; `OnValueChanged`; respeta el cooldown del menú.
- Paso de `USlider` con mando (`StepSize`) y fila enfocable que reenvía izquierda/derecha al control.

## Validación
- Pruebas de automatización por incremento en el consumidor DeadDraw (`DeadDrawValidation`, prefijo `CoreKit.Navigation.*`) usando input sintético como las pruebas existentes de CoreKit.
- Build Editor y Game de DeadDraw; los cambios reflejados se documentan en `Docs/CoreKitNavigation.md`.
- La validación en un proyecto limpio de UE 5.8.3 que pide `AGENTS.md` de CoreKit queda registrada como pendiente si no está disponible en esta máquina (aquí hay 5.8.4).
