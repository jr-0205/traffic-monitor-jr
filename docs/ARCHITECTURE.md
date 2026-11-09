# Arquitectura de TrafficMonitorJr

## Objetivo

TrafficMonitorJr se distribuye como un único archivo `TrafficMonitorJr.exe`, pero mantiene separado el estado del usuario del binario distribuible.

## Capas

```text
TrafficMonitorJr.exe
│
├─ Launcher .NET 10
│  ├─ Single instance
│  ├─ Instalación / reparación del payload
│  ├─ Migración desde la versión portátil anterior
│  └─ Lanzamiento de TrafficMonitor
│
└─ Payload embebido
   ├─ runtime/
   │  ├─ TrafficMonitor.exe
   │  ├─ DLL de hardware
   │  ├─ plugins/
   │  │  └─ TrafficMonitorJrIp.dll   # plugin IPv4 (C++/Win32)
   │  └─ recursos del runtime
   ├─ defaults/
   │  ├─ config.ini
   │  └─ global_cfg.ini
   ├─ language/
   └─ skins/
```

## Estado en tiempo de ejecución

El launcher instala el payload en:

```text
%LOCALAPPDATA%\TrafficMonitorJr\
├─ App\
│  ├─ TrafficMonitor.exe
│  ├─ config.ini
│  ├─ global_cfg.ini
│  ├─ plugins\
│  │  └─ TrafficMonitorJrIp.dll
│  ├─ language\
│  ├─ skins\
│  └─ historial generado por TrafficMonitor
├─ Backups\
└─ .payload.sha256
```

La huella SHA-256 de todos los recursos embebidos determina si el runtime debe actualizarse. Los archivos de configuración no se sobrescriben durante una actualización normal.

## Migración

Si el nuevo `TrafficMonitorJr.exe` se ejecuta desde una carpeta que contiene la versión portátil anterior, el launcher importa una sola vez:

- `config.ini`
- `global_cfg.ini`
- `history_traffic.dat`
- `history_traffic.dat.bak`

## Plugin de IPv4

`TrafficMonitorJrIp.dll` es un plugin nativo x64 (C++/Win32, API v8 del sistema de plugins de
TrafficMonitor) que publica un ítem de barra de tareas llamado `TMJrIPv4` con la dirección IPv4
del adaptador seleccionado.

- Se compila con `scripts/build-plugin.ps1` (MSVC) y se coloca en `payload/runtime/plugins/`.
- En tiempo de ejecución vive en `App\plugins\` y lee `global_cfg.ini` (`portable_mode`) y
  `config.ini` para resolver el adaptador igual que TrafficMonitor: `select_all`, `auto_select`
  (mayor tráfico entre adaptadores operativos) o `connection_name`.
- `ConfigEnhancer` añade `TMJrIPv4` a `[task_bar] plugin_display_item` en configuraciones
  existentes para que el ítem aparezca sin reconfigurar.

## Comandos

- `TrafficMonitorJr.exe`: instala/actualiza y abre TrafficMonitor.
- `TrafficMonitorJr.exe --status`: muestra estado y rutas.
- `TrafficMonitorJr.exe --repair`: reinstala el runtime sin borrar la configuración.
- `TrafficMonitorJr.exe --reset-config`: respalda y restaura el perfil JR Modern.
- `TrafficMonitorJr.exe --open-data`: abre la carpeta de datos.

## Regla de arquitectura

La raíz del repositorio contiene solamente documentación y archivos de desarrollo. Los binarios de terceros y recursos que deben empaquetarse pertenecen a `payload/`; el código propio pertenece a `src/`; scripts de desarrollo a `scripts/`; datos generados nunca se versionan.
