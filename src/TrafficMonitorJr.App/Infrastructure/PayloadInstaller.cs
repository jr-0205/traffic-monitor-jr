using System.Diagnostics;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;

namespace TrafficMonitorJr.Infrastructure;

internal sealed class PayloadInstaller
{
    private const string ResourcePrefix = "Payload/";
    private readonly AppPaths _paths;
    private readonly Assembly _assembly;
    private readonly string[] _resources;

    public PayloadInstaller(AppPaths paths, Assembly assembly)
    {
        _paths = paths;
        _assembly = assembly;
        _resources = assembly
            .GetManifestResourceNames()
            .Where(name => name.StartsWith(ResourcePrefix, StringComparison.Ordinal))
            .OrderBy(name => name, StringComparer.Ordinal)
            .ToArray();

        if (_resources.Length == 0)
            throw new InvalidOperationException("El ejecutable no contiene el payload de TrafficMonitor.");
    }

    public string PayloadFingerprint => ComputeFingerprint();

    public void EnsureInstalled(bool forceRepair = false)
    {
        Directory.CreateDirectory(_paths.BaseRoot);
        Directory.CreateDirectory(_paths.AppRoot);
        Directory.CreateDirectory(_paths.BackupRoot);

        var fingerprint = PayloadFingerprint;
        var installedFingerprint = File.Exists(_paths.PayloadMarker)
            ? File.ReadAllText(_paths.PayloadMarker).Trim()
            : string.Empty;

        var needsInstall =
            forceRepair ||
            !File.Exists(_paths.TrafficMonitorExe) ||
            !string.Equals(installedFingerprint, fingerprint, StringComparison.OrdinalIgnoreCase);

        if (!needsInstall)
            return;

        if (Process.GetProcessesByName("TrafficMonitor").Length > 0)
        {
            throw new TrafficMonitorRunningException(
                "Hay una actualización o reparación pendiente. Cierra TrafficMonitor desde el icono de la bandeja y abre TrafficMonitorJr otra vez.");
        }

        foreach (var resourceName in _resources)
            InstallResource(resourceName, overwriteDefaults: false);

        File.WriteAllText(_paths.PayloadMarker, fingerprint, new UTF8Encoding(false));
    }

    public void ResetConfig()
    {
        Directory.CreateDirectory(_paths.BackupRoot);

        if (File.Exists(_paths.ConfigPath))
        {
            var backup = Path.Combine(
                _paths.BackupRoot,
                $"config-{DateTime.Now:yyyyMMdd-HHmmss}.ini");
            File.Copy(_paths.ConfigPath, backup, overwrite: true);
        }

        var configResource = _resources.FirstOrDefault(
            name => name.Equals("Payload/defaults/config.ini", StringComparison.Ordinal));

        if (configResource is null)
            throw new InvalidOperationException("No se encontró la configuración predeterminada embebida.");

        Extract(configResource, _paths.ConfigPath, overwrite: true);
    }

    private void InstallResource(string resourceName, bool overwriteDefaults)
    {
        var relative = resourceName[ResourcePrefix.Length..]
            .Replace('/', Path.DirectorySeparatorChar);

        if (relative.StartsWith($"defaults{Path.DirectorySeparatorChar}", StringComparison.Ordinal))
        {
            var fileName = Path.GetFileName(relative);
            var destination = fileName.Equals("config.ini", StringComparison.OrdinalIgnoreCase)
                ? _paths.ConfigPath
                : fileName.Equals("global_cfg.ini", StringComparison.OrdinalIgnoreCase)
                    ? _paths.GlobalConfigPath
                    : Path.Combine(_paths.AppRoot, fileName);

            if (!overwriteDefaults && File.Exists(destination))
                return;

            Extract(resourceName, destination, overwrite: true);
            return;
        }

        if (relative.StartsWith($"runtime{Path.DirectorySeparatorChar}", StringComparison.Ordinal))
            relative = relative[("runtime" + Path.DirectorySeparatorChar).Length..];

        var destinationPath = Path.Combine(_paths.AppRoot, relative);
        Extract(resourceName, destinationPath, overwrite: true);
    }

    private void Extract(string resourceName, string destinationPath, bool overwrite)
    {
        var destinationDirectory = Path.GetDirectoryName(destinationPath)
            ?? throw new InvalidOperationException($"Ruta inválida: {destinationPath}");

        Directory.CreateDirectory(destinationDirectory);

        using var input = _assembly.GetManifestResourceStream(resourceName)
            ?? throw new InvalidOperationException($"No se pudo abrir el recurso {resourceName}.");

        var temporaryPath = destinationPath + ".tmp";
        using (var output = new FileStream(
                   temporaryPath,
                   FileMode.Create,
                   FileAccess.Write,
                   FileShare.None))
        {
            input.CopyTo(output);
        }

        if (!overwrite && File.Exists(destinationPath))
        {
            File.Delete(temporaryPath);
            return;
        }

        File.Move(temporaryPath, destinationPath, overwrite: true);
    }

    private string ComputeFingerprint()
    {
        using var hash = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);

        foreach (var resourceName in _resources)
        {
            hash.AppendData(Encoding.UTF8.GetBytes(resourceName));

            using var stream = _assembly.GetManifestResourceStream(resourceName)
                ?? throw new InvalidOperationException($"No se pudo leer {resourceName}.");

            var buffer = new byte[64 * 1024];
            int read;
            while ((read = stream.Read(buffer, 0, buffer.Length)) > 0)
                hash.AppendData(buffer, 0, read);
        }

        return Convert.ToHexString(hash.GetHashAndReset());
    }
}

internal sealed class TrafficMonitorRunningException(string message) : Exception(message);
