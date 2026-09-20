namespace TrafficMonitorJr.Infrastructure;

internal sealed record AppPaths(
    string BaseRoot,
    string AppRoot,
    string BackupRoot,
    string TrafficMonitorExe,
    string ConfigPath,
    string GlobalConfigPath,
    string PayloadMarker,
    string LegacyImportMarker)
{
    public static AppPaths Create()
    {
        var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
        var baseRoot = Path.Combine(localAppData, "TrafficMonitorJr");
        var appRoot = Path.Combine(baseRoot, "App");

        return new AppPaths(
            BaseRoot: baseRoot,
            AppRoot: appRoot,
            BackupRoot: Path.Combine(baseRoot, "Backups"),
            TrafficMonitorExe: Path.Combine(appRoot, "TrafficMonitor.exe"),
            ConfigPath: Path.Combine(appRoot, "config.ini"),
            GlobalConfigPath: Path.Combine(appRoot, "global_cfg.ini"),
            PayloadMarker: Path.Combine(baseRoot, ".payload.sha256"),
            LegacyImportMarker: Path.Combine(baseRoot, ".legacy-imported"));
    }
}
