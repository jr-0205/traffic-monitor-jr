namespace TrafficMonitorJr.Infrastructure;

internal enum AppCommand
{
    Launch,
    Repair,
    ResetConfig,
    OpenData,
    Status
}

internal static class CommandLine
{
    public static AppCommand Parse(string[] args)
    {
        if (args.Length == 0)
            return AppCommand.Launch;

        return args[0].Trim().ToLowerInvariant() switch
        {
            "--repair" => AppCommand.Repair,
            "--reset-config" => AppCommand.ResetConfig,
            "--open-data" => AppCommand.OpenData,
            "--status" => AppCommand.Status,
            _ => AppCommand.Launch
        };
    }
}
