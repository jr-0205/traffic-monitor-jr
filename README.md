# TrafficMonitorJr

TrafficMonitorJr empaqueta una experiencia personalizada de TrafficMonitor en **un único ejecutable para Windows**.

## Resultado

El archivo que se distribuye es:

```text
TrafficMonitorJr.exe
```

No hace falta entregar junto a él DLL, skins, idiomas, archivos INI ni scripts. El launcher contiene el payload y prepara automáticamente el runtime de usuario en `%LOCALAPPDATA%\TrafficMonitorJr\App`.

## Mejoras incluidas

- Skin **JR Modern**.
- Interfaz compacta en español.
- Selección automática de red sin fijar un adaptador antiguo.
- Dirección IPv4 de la tarjeta de red seleccionada en la barra de tareas (plugin nativo incluido).
- Migración automática desde la versión portátil anterior.
- Configuración persistente entre actualizaciones.
- Reparación del runtime sin perder configuración.
- Restauración con respaldo del perfil JR Modern.
- Distribución autocontenida: el usuario no necesita instalar .NET.
- Build reproducible mediante GitHub Actions.

## Estructura del repositorio

```text
.
├─ src/
│  ├─ TrafficMonitorJr.App/      # launcher y lógica propia
│  └─ TrafficMonitorJr.IpPlugin/ # plugin nativo C++ (IPv4 en la barra)
├─ payload/
│  ├─ runtime/                   # TrafficMonitor y dependencias
│  ├─ defaults/                  # configuración inicial
│  ├─ language/                  # idiomas
│  └─ skins/                     # skins, incluido JR Modern
├─ scripts/
│  ├─ build.ps1                  # publicación local
│  └─ build-plugin.ps1           # compilación del plugin (MSVC)
├─ docs/
│  └─ ARCHITECTURE.md
└─ .github/workflows/
   └─ build.yml
```

## Compilar

Requiere el SDK indicado en `global.json` y, para el plugin nativo, Visual Studio con la carga
**Desktop development with C++** (MSVC x64). `build.ps1` compila el plugin y luego el launcher.

```powershell
.\scripts\build.ps1
```

Para compilar solo el launcher (sin el plugin) usa:

```powershell
.\scripts\build.ps1 -SkipPlugin
```

El resultado queda en:

```text
artifacts\win-x64\TrafficMonitorJr.exe
```

## Acciones del launcher

```text
TrafficMonitorJr.exe
TrafficMonitorJr.exe --status
TrafficMonitorJr.exe --repair
TrafficMonitorJr.exe --reset-config
TrafficMonitorJr.exe --open-data
```

## Proyecto original

El motor de monitorización incluido pertenece al proyecto TrafficMonitor de zhongyang219:

https://github.com/zhongyang219/TrafficMonitor

La licencia original se conserva en `THIRD_PARTY/TrafficMonitor/LICENSE`.

Consulta `docs/ARCHITECTURE.md` para el diseño técnico.
