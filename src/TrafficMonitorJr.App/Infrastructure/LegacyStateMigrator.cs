namespace TrafficMonitorJr.Infrastructure;

internal static class LegacyStateMigrator
{
    private static readonly string[] UserStateFiles =
    [
        "config.ini",
        "global_cfg.ini",
        "history_traffic.dat",
        "history_traffic.dat.bak"
    ];

    public static void TryImport(string sourceDirectory, AppPaths paths)
    {
        if (File.Exists(paths.LegacyImportMarker))
            return;

        var sourceFullPath = Path.GetFullPath(sourceDirectory)
            .TrimEnd(Path.DirectorySeparatorChar);
        var targetFullPath = Path.GetFullPath(paths.AppRoot)
            .TrimEnd(Path.DirectorySeparatorChar);

        if (string.Equals(sourceFullPath, targetFullPath, StringComparison.OrdinalIgnoreCase))
        {
            MarkImported(paths);
            return;
        }

        var importedAny = false;

        foreach (var fileName in UserStateFiles)
        {
            var source = Path.Combine(sourceDirectory, fileName);
            if (!File.Exists(source))
                continue;

            var destination = Path.Combine(paths.AppRoot, fileName);
            File.Copy(source, destination, overwrite: true);
            importedAny = true;
        }

        MarkImported(paths, importedAny);
    }

    private static void MarkImported(AppPaths paths, bool importedAny = false)
    {
        Directory.CreateDirectory(paths.BaseRoot);
        File.WriteAllText(
            paths.LegacyImportMarker,
            importedAny ? "legacy-state-imported" : "no-legacy-state-found");
    }
}
