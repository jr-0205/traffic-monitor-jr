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
│  └─ TrafficMonitorJr.App/      # launcher y lógica propia
├─ payload/
│  ├─ runtime/                   # TrafficMonitor y dependencias
│  ├─ defaults/                  # configuración inicial
│  ├─ language/                  # idiomas
│  └─ skins/                     # skins, incluido JR Modern
├─ scripts/
│  └─ build.ps1                  # publicación local
├─ docs/
│  └─ ARCHITECTURE.md
└─ .github/workflows/
   └─ build.yml
```

## Compilar

Requiere el SDK indicado en `global.json`.

```powershell
.\scripts\build.ps1
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
