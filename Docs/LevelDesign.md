# Diseño de salas en Delerict

Abre `L_DLevelDesignLab` desde `Content/Maps` y pulsa **Play**. Toma carrera con **D** y salta con **Espacio** para cruzar el primer hueco. Pasa por el checkpoint verde y prueba el peligro rojo para volver al checkpoint. Salta sobre el peligro y alcanza la salida azul.

Para armar tu propia sala, crea un nivel o duplica el laboratorio desde el Content Browser. Abre **Window → Place Actors → Delerict** y coloca las piezas en la vista lateral. Usa el mismo valor de **Y** en el inicio y en las piezas jugables. La vista lateral y la rejilla estándar de Unreal sirven para alinear en X y Z. Puedes iniciar una prueba desde el `PlayerStart` de tu nivel o elegir **Play From Here** en el viewport.

- **Suelo, plataforma y pared.** `Dimensions` controla ancho (X), profundidad (Y) y altura (Z) en centímetros. `Finish` cambia el acabado. El pivote está en el centro de la base.
- **Inicio.** Colócalo unos 100 cm sobre la superficie transitable. Su Y fija el carril de Mover.
- **Punto de acople.** Arrastra `BP_DDockingPoint` desde `Content/Delerict/LevelDesign/Prefabs`. Orienta el eje X hacia la aproximación y ajusta `Docking Offset` y `Approach Radius`. El punto expone la pose al sistema de movimiento cuando el Pawn está al alcance y `Docking Enabled` está activo.
- **Peligro.** `Dimensions` define el volumen. Al tocarlo aparece **You Died**. Pulsa **Retry** para volver al checkpoint activo o al inicio del nivel. Para probar la pantalla en cualquier sala, ejecuta `Die` en la consola durante Play.
- **Checkpoint.** `Respawn Offset` define dónde reapareces respecto de la base. Ponlo sobre un suelo seguro y comprueba que quepa la cápsula.
- **Salida.** `Destination` permite asignar el siguiente mapa. En el laboratorio ese campo está vacío y la salida marca la sala como completada.

Las piezas se guardan como actores normales del mapa. Modifica instancias en la sala para cambios locales. Para crear otra pieza reutilizable, duplica su Blueprint dentro de `Content/Delerict/LevelDesign/Prefabs`, cambia sus Class Defaults y reinicia el editor para actualizar la paleta. `On Marker Activated` permite conectar la presentación de un checkpoint o una salida en Blueprint.

El checkpoint vive durante la sesión de la sala y se reinicia al reiniciar el nivel. La herramienta del editor vive en `DelerictLevelEditor`. La colisión y el progreso de la sala viven en el módulo `Delerict`. El comando `-run=DelerictLevelEditor.DLevelDesign` de `UnrealEditor-Cmd.exe` crea los assets iniciales que falten y conserva los prefabs y el laboratorio que ya existan.
