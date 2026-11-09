using System.Text;

namespace TrafficMonitorJr.Infrastructure;

internal static class ConfigEnhancer
{
    private const string ItemId = "TMJrIPv4";
    private const string Section = "task_bar";
    private const string Key = "plugin_display_item";

    public static void EnsureTaskBarPluginItem(AppPaths paths)
    {
        if (!File.Exists(paths.ConfigPath))
            return;

        try
        {
            var lines = File.ReadAllLines(paths.ConfigPath).ToList();
            if (ContainsItem(lines))
                return;

            if (!InjectItem(lines))
                return;

            File.WriteAllLines(paths.ConfigPath, lines, new UTF8Encoding(false));
        }
        catch
        {
            // La configuración es opcional; no impedir el arranque si falla.
        }
    }

    private static bool ContainsItem(IReadOnlyList<string> lines)
    {
        var sectionIndex = FindSection(lines);
        if (sectionIndex < 0)
            return false;

        var keyIndex = FindKey(lines, sectionIndex);
        if (keyIndex < 0)
            return false;

        var value = ReadValue(lines[keyIndex]);
        foreach (var token in value.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
        {
            if (token.Equals(ItemId, StringComparison.OrdinalIgnoreCase))
                return true;
        }
        return false;
    }

    private static bool InjectItem(List<string> lines)
    {
        var sectionIndex = FindSection(lines);
        if (sectionIndex < 0)
        {
            if (lines.Count > 0 && !string.IsNullOrWhiteSpace(lines[^1]))
                lines.Add(string.Empty);
            lines.Add($"[{Section}]");
            lines.Add($"{Key} = {ItemId}");
            return true;
        }

        var keyIndex = FindKey(lines, sectionIndex);
        if (keyIndex < 0)
        {
            lines.Insert(sectionIndex + 1, $"{Key} = {ItemId}");
            return true;
        }

        var value = ReadValue(lines[keyIndex]);
        var updated = string.IsNullOrWhiteSpace(value) ? ItemId : $"{value},{ItemId}";
        lines[keyIndex] = $"{Key} = {updated}";
        return true;
    }

    private static int FindSection(IReadOnlyList<string> lines)
    {
        for (var i = 0; i < lines.Count; i++)
        {
            if (IsSection(lines[i], Section))
                return i;
        }
        return -1;
    }

    private static int FindKey(IReadOnlyList<string> lines, int sectionIndex)
    {
        for (var i = sectionIndex + 1; i < lines.Count; i++)
        {
            if (IsSectionHeader(lines[i]))
                break;
            if (ReadKey(lines[i]).Equals(Key, StringComparison.OrdinalIgnoreCase))
                return i;
        }
        return -1;
    }

    private static bool IsSection(string line, string section)
    {
        var trimmed = line.Trim();
        return trimmed.Length >= 2 &&
            trimmed[0] == '[' &&
            trimmed[^1] == ']' &&
            trimmed[1..^1].Trim().Equals(section, StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsSectionHeader(string line)
    {
        var trimmed = line.Trim();
        return trimmed.Length >= 2 && trimmed[0] == '[' && trimmed[^1] == ']';
    }

    private static string ReadKey(string line)
    {
        var equals = line.IndexOf('=');
        return equals < 0 ? string.Empty : line[..equals].Trim();
    }

    private static string ReadValue(string line)
    {
        var equals = line.IndexOf('=');
        if (equals < 0)
            return string.Empty;

        var value = line[(equals + 1)..].Trim();
        if (value.Length >= 2 && value[0] == '"' && value[^1] == '"')
            value = value[1..^1];
        return value;
    }
}
