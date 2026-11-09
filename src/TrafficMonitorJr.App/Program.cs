using System.Diagnostics;
using System.Reflection;
using System.Windows.Forms;
using TrafficMonitorJr.Infrastructure;

namespace TrafficMonitorJr;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        ApplicationConfiguration.Initialize();

        using var mutex = new Mutex(
            initiallyOwned: true,
            name: @"Local\TrafficMonitorJr.Launcher",
            createdNew: out var createdNew);

        if (!createdNew)
            return 0;

        try
        {
            var paths = AppPaths.Create();
            var installer = new PayloadInstaller(paths, Assembly.GetExecutingAssembly());
            var command = CommandLine.Parse(args);

            if (command == AppCommand.OpenData)
            {
                Directory.CreateDirectory(paths.AppRoot);
                Process.Start(new ProcessStartInfo("explorer.exe", paths.AppRoot)
                {
                    UseShellExecute = true
                });
                return 0;
            }

            if (command == AppCommand.Status)
            {
                installer.EnsureInstalled();
                ShowStatus(paths, installer);
                return 0;
            }

            if (command == AppCommand.ResetConfig)
            {
                EnsureTrafficMonitorClosed();
                installer.EnsureInstalled();
                installer.ResetConfig();
                MessageBox.Show(
                    "La configuración se restauró al perfil JR Modern.",
                    "TrafficMonitorJr",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return 0;
            }

            if (command == AppCommand.Repair)
            {
                EnsureTrafficMonitorClosed();
                installer.EnsureInstalled(forceRepair: true);
                MessageBox.Show(
                    "El runtime de TrafficMonitorJr se reparó correctamente.",
                    "TrafficMonitorJr",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
                return 0;
            }

            installer.EnsureInstalled();

            var launcherDirectory = Path.GetDirectoryName(Environment.ProcessPath);
            if (!string.IsNullOrWhiteSpace(launcherDirectory))
                LegacyStateMigrator.TryImport(launcherDirectory, paths);

            ConfigEnhancer.EnsureTaskBarPluginItem(paths);

            if (Process.GetProcessesByName("TrafficMonitor").Length > 0)
                return 0;

            Process.Start(new ProcessStartInfo(paths.TrafficMonitorExe)
            {
                WorkingDirectory = paths.AppRoot,
                UseShellExecute = true
            });

            return 0;
        }
        catch (TrafficMonitorRunningException ex)
        {
            MessageBox.Show(
                ex.Message,
                "TrafficMonitorJr",
                MessageBoxButtons.OK,
                MessageBoxIcon.Warning);
            return 2;
        }
        catch (Exception ex)
        {
            MessageBox.Show(
                $"No se pudo iniciar TrafficMonitorJr.\n\n{ex.Message}",
                "TrafficMonitorJr",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return 1;
        }
    }

    private static void EnsureTrafficMonitorClosed()
    {
        if (Process.GetProcessesByName("TrafficMonitor").Length > 0)
        {
            throw new TrafficMonitorRunningException(
                "Cierra TrafficMonitor desde el icono de la bandeja y vuelve a ejecutar esta acción.");
        }
    }

    private static void ShowStatus(AppPaths paths, PayloadInstaller installer)
    {
        var status =
            $"TrafficMonitorJr 2.0\n\n" +
            $"Runtime: {(File.Exists(paths.TrafficMonitorExe) ? "OK" : "Falta")}\n" +
            $"Payload: {installer.PayloadFingerprint[..12]}\n" +
            $"Datos: {paths.AppRoot}\n" +
            $"Configuración: {paths.ConfigPath}\n\n" +
            "Comandos disponibles:\n" +
            "--status        Estado\n" +
            "--repair        Reinstalar runtime\n" +
            "--reset-config  Restaurar configuración JR Modern\n" +
            "--open-data     Abrir carpeta de datos";

        MessageBox.Show(status, "TrafficMonitorJr", MessageBoxButtons.OK, MessageBoxIcon.Information);
    }
}
