# Practicas_Primer_Parcial

Eduardo Cerón

## Descripción del proyecto

Este proyecto implementa una aplicación multitarea utilizando FreeRTOS sobre una tarjeta ESP32. El objetivo es demostrar el uso de tareas concurrentes, sincronización mediante variables compartidas, monitoreo de recursos del sistema y ejecución periódica de sensores.

El sistema está compuesto por cuatro tareas principales:

- **vTaskLedRapido()**: controla el parpadeo rápido del LED integrado de la ESP32 con un periodo de 100 ms. Al detectar la pulsación del botón, cambia el sistema al modo lento y se autosuspende.
- **vTaskLedLento()**: controla el parpadeo lento del LED con un periodo de 500 ms. Mantiene este modo durante 5 segundos y posteriormente reactiva la tarea de parpadeo rápido.
- **vTaskSensor()**: realiza la lectura periódica del ADC conectado al GPIO34 cada 300 ms y reporta los valores obtenidos a través de la terminal serial cuando el sensor se encuentra habilitado.
- **vTaskMonitor()**: supervisa continuamente el estado del botón mediante polling, detecta flancos de activación y genera reportes periódicos del uso de memoria dinámica (heap) y del stack disponible de las tareas.

Adicionalmente, se implementa un **Idle Hook** que permite identificar cuándo el procesador se encuentra libre debido a que todas las tareas están bloqueadas o suspendidas.

Durante la ejecución del programa se utilizan conceptos fundamentales de FreeRTOS como:

- Creación de tareas mediante `xTaskCreate()`.
- Suspensión y reanudación de tareas mediante `vTaskSuspend()` y `vTaskResume()`.
- Variables compartidas entre tareas.
- Prioridades de ejecución.
- Retardos periódicos con `vTaskDelay()` y `vTaskDelayUntil()`.
- Monitoreo de recursos mediante `uxTaskGetStackHighWaterMark()`.
- Uso del Idle Hook para detectar tiempos de inactividad del procesador.

El funcionamiento general consiste en iniciar el sistema con un LED parpadeando rápidamente. Al presionar el botón BOOT de la ESP32, el sistema cambia temporalmente a un modo de parpadeo lento durante 5 segundos mientras se habilita la lectura del ADC. Una vez transcurrido ese tiempo, el sistema regresa automáticamente al modo inicial.

## Preguntas:
1. ¿Por qué la variable `g_ledRapido` debe declararse como `volatile`? ¿Qué ocurre si se omite esa palabra clave?

La variable `g_ledRapido` debe declararse como `volatile` porque es compartida entre varias tareas de FreeRTOS. Una tarea puede modificar su valor mientras otra la está leyendo. La palabra clave `volatile` indica al compilador que el valor de la variable puede cambiar en cualquier momento y evita que se realicen optimizaciones que podrían provocar que una tarea utilice un valor antiguo almacenado en registros. Si se omite `volatile`, una tarea podría no detectar los cambios realizados por otra tarea, ocasionando comportamientos inesperados en el sistema.


2. ¿En qué momento exacto aparece el mensaje `[IDLE]` en la terminal? Describe el estado de las cuatro tareas en ese instante.

El mensaje `[IDLE]` aparece cuando el planificador de FreeRTOS no tiene ninguna tarea lista para ejecutarse y el procesador entra en estado de reposo (Idle). En ese momento las tareas se encuentran bloqueadas o suspendidas:

- `vTaskLedRapido()`: bloqueada por `vTaskDelay()` o suspendida durante el modo lento.
- `vTaskLedLento()`: bloqueada por `vTaskDelay()`.
- `vTaskSensor()`: bloqueada por `vTaskDelayUntil()`.
- `vTaskMonitor()`: bloqueada por `vTaskDelay()`.

Al no existir tareas listas para ejecutarse, se ejecuta el Idle Hook y se imprime el mensaje correspondiente.



3. ¿Qué diferencia existe entre `vTaskDelay()` y `vTaskDelayUntil()`? ¿En cuál de las tareas de esta práctica sería más apropiado usar `vTaskDelayUntil()`?

`vTaskDelay()` bloquea una tarea durante un tiempo determinado contado a partir del momento en que se ejecuta la función.

`vTaskDelayUntil()` permite ejecutar una tarea de forma periódica manteniendo un intervalo constante entre ejecuciones, independientemente del tiempo que tarde el código en ejecutarse. En esta práctica, la función más adecuada para utilizar `vTaskDelayUntil()` es `vTaskSensor()`, ya que se requiere realizar una lectura del ADC cada 300 ms de manera periódica y precisa.



4. ¿Por qué `vTaskLedRapido` tiene prioridad menor que `vTaskMonitor`? Describe qué ocurriría si se invirtieran esas prioridades.

`vTaskMonitor()` tiene una prioridad mayor porque es la encargada de supervisar el botón y detectar los eventos de usuario. Esto permite que la respuesta al presionar el botón sea rápida y consistente.
Si `vTaskLedRapido()` tuviera una prioridad mayor, podría ejecutarse con más frecuencia que la tarea de monitoreo, provocando retrasos en la detección de pulsaciones y una menor capacidad de respuesta del sistema ante la interacción del usuario.



5. ¿Qué riesgo existe al leer una variable `volatile` desde dos tareas distintas sin protección? Investiga el concepto de sección crítica.

El principal riesgo es la aparición de condiciones de carrera (*race conditions*). Estas ocurren cuando dos o más tareas acceden simultáneamente a una misma variable y al menos una de ellas la modifica.

La palabra clave `volatile` únicamente garantiza que el compilador lea el valor actualizado de memoria, pero no protege el acceso concurrente a la variable. Para evitar este problema se utilizan mecanismos de sincronización como las secciones críticas. Una sección crítica es una región de código protegida donde se impide temporalmente la interrupción o el cambio de contexto entre tareas mientras se accede a recursos compartidos. En FreeRTOS una sección crítica puede implementarse mediante:

```c
taskENTER_CRITICAL();

/* Código protegido */

taskEXIT_CRITICAL();
```

De esta forma se evita que otra tarea modifique la variable mientras está siendo utilizada.

## Conclusión

En esta práctica se logró implementar un sistema multitarea utilizando FreeRTOS sobre la plataforma ESP32, permitiendo comprender el funcionamiento del planificador, las prioridades de ejecución y la interacción entre diferentes tareas. Además, se aplicaron conceptos como suspensión y reanudación de tareas, temporización periódica, monitoreo de memoria y uso del Idle Hook para analizar el estado del procesador. Los resultados obtenidos permitieron verificar el correcto funcionamiento del sistema y reforzar los conocimientos relacionados con sistemas operativos en tiempo real y programación de sistemas embebidos.

## Evidencia de monitor serial

La salida completa del monitor serial se encuentra en el archivo:

`Serial_Monitor_Practica2.txt`
