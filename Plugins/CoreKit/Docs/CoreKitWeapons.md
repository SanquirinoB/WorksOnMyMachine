# CoreKitWeapons

Módulo Runtime para armas de juego local, adaptable a personajes en 3D o a un sidescroller 2.5D. Adapta equipamiento, munición, cadencia y proyectiles de CombatKit. El módulo depende de CoreKitActions y puede usarse con cualquier Pawn o Actor.

1. Añade **Core Kit Weapons Component** al actor y asigna las definiciones en **Initial Loadout**. Las muestras de Content Browser están en `/CoreKit/Weapons`.
2. Suministra **Set Aim** con la posición de salida y dirección. Para XZ, usa la normal `(0,1,0)`. El módulo proyecta la dirección y limita la dispersión a ese plano. El consumidor fija la posición de salida en el carril correcto.
3. Enlaza input de pulsar y soltar a **Press Trigger** y **Release Trigger**, y llama **Equip Slot** o **Request Reload** desde los controles correspondientes.
4. Implementa **Core Kit Weapon Receiver** en los actores que reciben impactos. Aplica ahí salud, equipos, armadura y reacciones. El resultado indica si se aceptó el contacto. Conecta **On State Changed**, **On Fired**, **On Impact** y **On Blast** a UI y presentación. Durante una recarga, **Get State** permite consultar el avance de la operación.

Si el actor carece de CoreKitActions, el componente crea un coordinador y un conjunto de acciones transitorio. Para un conjunto propio, declara `CoreKit.Weapons.Action.Fire` y `CoreKit.Weapons.Action.Reload` con plantillas **Core Kit Weapon Action** y `bTickAction` activo. Usa **Core Kit Command Action** para `CoreKit.Weapons.Action.Equip`. El componente cancela las solicitudes en cola asociadas al gesto, por lo que el disparo no se ejecuta más tarde fuera de ese contexto.

| Muestra | Comportamiento |
| --- | --- |
| Pistola | Semiautomática y recarga de cargador con reserva finita. |
| Rifle | Automático, reserva ilimitada, recarga al vaciar y reanudación si el gatillo sigue pulsado. |
| Escopeta | Seis perdigones por cartucho y recarga interrumpible de un cartucho a la vez. |
| Lanzamisiles | Proyectil recto visible que explota al impactar. La explosión cae con la distancia, afecta una vez por actor y respeta la cobertura. |

Las definiciones también admiten hitscan, ráfagas, melee con ventanas y combos, y proyectiles con penetración o rebote a otro receptor. El estado de munición vive por arma equipada. Las mallas de muestra son formas básicas de Unreal y cada definición permite asignar audio propio. El actor puede usar el evento de equipamiento para montar la malla en un socket.

El módulo usa autoridad local, con proyectiles de recorrido recto. Guiado, trayectorias balísticas y replicación quedan para una ampliación.

El consumidor de prueba en `MachineVault/CoreKitWeaponsValidation` usa un mapa limpio y los cuatro DataAssets. En un clon ejecuta `Scripts/ProjectSetup/Link-CoreKit.ps1 CoreKitWeaponsValidation` y compila Editor o Game con UE 5.8.4. El ejecutable acepta `-CoreKitWeaponsSmoke` para recorrer equipamiento, tiro lateral, pool, recarga y cobertura sin entrada manual.
