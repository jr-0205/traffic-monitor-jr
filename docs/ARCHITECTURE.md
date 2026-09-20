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

## Comandos

- `TrafficMonitorJr.exe`: instala/actualiza y abre TrafficMonitor.
- `TrafficMonitorJr.exe --status`: muestra estado y rutas.
- `TrafficMonitorJr.exe --repair`: reinstala el runtime sin borrar la configuración.
- `TrafficMonitorJr.exe --reset-config`: respalda y restaura el perfil JR Modern.
- `TrafficMonitorJr.exe --open-data`: abre la carpeta de datos.

## Regla de arquitectura

La raíz del repositorio contiene solamente documentación y archivos de desarrollo. Los binarios de terceros y recursos que deben empaquetarse pertenecen a `payload/`; el código propio pertenece a `src/`; scripts de desarrollo a `scripts/`; datos generados nunca se versionan.
