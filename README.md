# TrafficMonitorJr

Personalización portátil de **TrafficMonitor 1.86** enfocada en una interfaz más limpia y en una configuración más resistente a cambios de red.

## Qué cambia

- Skin **JR Modern**: dark technical minimalism, alto contraste, bordes discretos y densidad media.
- Etiquetas compactas en español para CPU, RAM, GPU, red y disco.
- Auto-selección de red sin dejar fijado un adaptador concreto.
- Configuración de la barra de tareas visualmente más limpia y coherente con Windows.
- Script de aplicación por secciones INI: evita reemplazos globales accidentales.
- Respaldo automático de `config.ini` antes de aplicar cambios.
- Restauración rápida del respaldo más reciente.
- Validación del ejecutable, configuración y archivos de skin antes de modificar nada.

## Uso rápido

1. Cierra TrafficMonitor desde el icono de la bandeja.
2. Ejecuta **Aplicar mejoras.cmd**.
3. TrafficMonitor vuelve a abrirse con el perfil **JR Modern**.

Para volver al último estado guardado, ejecuta **Restaurar configuracion.cmd**.

Para revisar la configuración activa, ejecuta **Estado.cmd**.

## Red

El perfil usa **Auto Select** y deja vacío `connection_name` para evitar que una descripción antigua de adaptador quede fijada después de que Windows recree o cambie interfaces.

Si alguna vez la velocidad queda en 0, abre el menú contextual de TrafficMonitor y selecciona manualmente la conexión correcta o actualiza la lista de conexiones.

## Archivos principales

- `TrafficMonitor.exe`: aplicación compilada.
- `config.ini`: configuración activa.
- `Aplicar mejoras.ps1`: aplica, restaura y diagnostica el perfil.
- `skins/JR Modern/`: skin personalizada.
- `backups/`: respaldos locales de configuración; no se versionan.

## Proyecto original

TrafficMonitor es desarrollado por **zhongyang219**. Este repositorio conserva la aplicación compilada y añade una configuración/skin personalizada. Consulta el proyecto original para documentación, licencia y nuevas versiones:

https://github.com/zhongyang219/TrafficMonitor
