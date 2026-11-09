// TrafficMonitorJrIp
// Plugin nativo para TrafficMonitor que muestra la direccion IPv4 de la
// tarjeta de red seleccionada en TrafficMonitor.
//
// - Se carga desde la carpeta "plugins" junto a TrafficMonitor.exe.
// - Lee la tarjeta seleccionada de config.ini ([connection]).
// - Replica la seleccion automatica de TrafficMonitor (mayor trafico).

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>

#include <string>
#include <vector>
#include <cwchar>
#include <cwctype>

#include "PluginInterface.h"

#pragma comment(lib, "iphlpapi.lib")

#ifndef IF_OPER_STATUS_OPERATIONAL
#define IF_OPER_STATUS_OPERATIONAL 5
#endif

static HINSTANCE g_hInstance = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReason, LPVOID /*lpReserved*/)
{
    if (ulReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hModule;
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Utilidades

static std::wstring TrimCopy(const std::wstring& value)
{
    const wchar_t* ws = L" \t\r\n";
    size_t begin = value.find_first_not_of(ws);
    if (begin == std::wstring::npos)
        return std::wstring();
    size_t end = value.find_last_not_of(ws);
    return value.substr(begin, end - begin + 1);
}

static std::wstring ToLowerCopy(std::wstring value)
{
    for (wchar_t& ch : value)
        ch = static_cast<wchar_t>(towlower(ch));
    return value;
}

static std::wstring AnsiToWide(const std::string& text)
{
    if (text.empty())
        return std::wstring();
    int length = MultiByteToWideChar(
        CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0)
        return std::wstring();
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(
        CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), &result[0], length);
    return result;
}

static std::string WideToAnsi(const std::wstring& text)
{
    if (text.empty())
        return std::string();
    int length = WideCharToMultiByte(
        CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0)
        return std::string();
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(
        CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), &result[0], length, nullptr, nullptr);
    return result;
}

static bool FileExists(const std::wstring& path)
{
    DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static std::wstring GetModuleDirectory(HMODULE module)
{
    wchar_t buffer[MAX_PATH]{};
    DWORD length = GetModuleFileNameW(module, buffer, MAX_PATH);
    std::wstring path(buffer, length);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        path.erase(pos);
    return path;
}

static std::wstring ParentDirectory(std::wstring directory)
{
    while (!directory.empty() &&
        (directory.back() == L'\\' || directory.back() == L'/'))
    {
        directory.pop_back();
    }
    size_t pos = directory.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        directory.erase(pos);
    return directory;
}

static std::wstring GetAppDataTrafficMonitorDirectory()
{
    wchar_t buffer[MAX_PATH]{};
    DWORD length = GetEnvironmentVariableW(L"APPDATA", buffer, MAX_PATH);
    std::wstring directory(buffer, length);
    if (!directory.empty() && directory.back() != L'\\')
        directory += L'\\';
    directory += L"TrafficMonitor\\";
    return directory;
}

///////////////////////////////////////////////////////////////////////////////
// Lectura de config.ini (UTF-8, igual que TrafficMonitor)

class IniFile
{
public:
    bool Load(const std::wstring& path)
    {
        std::string bytes;
        if (!ReadAllBytes(path, bytes))
            return false;

        std::wstring content = Decode(bytes);
        std::wstring section;

        size_t index = 0;
        while (index <= content.size())
        {
            size_t newLine = content.find(L'\n', index);
            std::wstring line = content.substr(
                index,
                (newLine == std::wstring::npos ? content.size() : newLine) - index);
            index = (newLine == std::wstring::npos) ? content.size() + 1 : newLine + 1;

            line = TrimCopy(line);
            if (line.empty() || line[0] == L';' || line[0] == L'#')
                continue;

            if (line.front() == L'[' && line.back() == L']')
            {
                section = TrimCopy(line.substr(1, line.size() - 2));
                continue;
            }

            size_t equals = line.find(L'=');
            if (equals == std::wstring::npos)
                continue;

            std::wstring key = TrimCopy(line.substr(0, equals));
            std::wstring value = TrimCopy(line.substr(equals + 1));
            if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"')
                value = value.substr(1, value.size() - 2);

            m_values.push_back(std::make_pair(MakeKey(section, key), value));
        }
        return true;
    }

    bool GetString(const std::wstring& section, const std::wstring& key, std::wstring& out) const
    {
        std::wstring lookup = MakeKey(section, key);
        for (const auto& item : m_values)
        {
            if (item.first == lookup)
            {
                out = item.second;
                return true;
            }
        }
        return false;
    }

    std::wstring GetString(
        const std::wstring& section, const std::wstring& key, const std::wstring& fallback) const
    {
        std::wstring value;
        return GetString(section, key, value) ? value : fallback;
    }

    bool GetBool(const std::wstring& section, const std::wstring& key, bool fallback) const
    {
        std::wstring value;
        if (!GetString(section, key, value))
            return fallback;
        std::wstring lowered = ToLowerCopy(value);
        if (lowered == L"true" || lowered == L"1")
            return true;
        if (lowered == L"false" || lowered == L"0")
            return false;
        return fallback;
    }

private:
    static std::wstring MakeKey(const std::wstring& section, const std::wstring& key)
    {
        return ToLowerCopy(section) + L"\n" + ToLowerCopy(key);
    }

    static bool ReadAllBytes(const std::wstring& path, std::string& out)
    {
        HANDLE handle = CreateFileW(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (handle == INVALID_HANDLE_VALUE)
            return false;

        LARGE_INTEGER size{};
        if (!GetFileSizeEx(handle, &size) || size.QuadPart > 8 * 1024 * 1024)
        {
            CloseHandle(handle);
            return false;
        }

        out.resize(static_cast<size_t>(size.QuadPart));
        DWORD read = 0;
        BOOL ok = out.empty()
            ? TRUE
            : ReadFile(handle, &out[0], static_cast<DWORD>(out.size()), &read, nullptr);
        CloseHandle(handle);
        if (!ok)
            return false;
        out.resize(read);
        return true;
    }

    static std::wstring Decode(const std::string& bytes)
    {
        size_t offset = 0;
        if (bytes.size() >= 3 &&
            static_cast<unsigned char>(bytes[0]) == 0xEF &&
            static_cast<unsigned char>(bytes[1]) == 0xBB &&
            static_cast<unsigned char>(bytes[2]) == 0xBF)
        {
            offset = 3;
        }

        const char* data = bytes.data() + offset;
        int length = static_cast<int>(bytes.size() - offset);
        if (length <= 0)
            return std::wstring();

        UINT codePage = CP_UTF8;
        int wideLength = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, data, length, nullptr, 0);
        if (wideLength <= 0)
        {
            codePage = CP_ACP;
            wideLength = MultiByteToWideChar(CP_ACP, 0, data, length, nullptr, 0);
        }
        if (wideLength <= 0)
            return std::wstring();

        std::wstring result(static_cast<size_t>(wideLength), L'\0');
        MultiByteToWideChar(
            codePage,
            codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0,
            data,
            length,
            &result[0],
            wideLength);
        return result;
    }

    std::vector<std::pair<std::wstring, std::wstring>> m_values;
};

///////////////////////////////////////////////////////////////////////////////
// Enumeracion de tarjetas de red (replica de TrafficMonitor)

struct NetConnection
{
    int index{ -1 };
    std::string description;    // descripcion de GetAdaptersInfo
    std::string description2;   // bDescr de MIB_IFROW (lo que guarda TrafficMonitor)
    unsigned long long inBytes{};
    unsigned long long outBytes{};
    DWORD operStatus{};
    std::wstring ip;
};

static bool LoadIfTable(std::vector<BYTE>& buffer)
{
    ULONG size = sizeof(MIB_IFTABLE);
    buffer.resize(size);

    DWORD result = GetIfTable(
        reinterpret_cast<MIB_IFTABLE*>(buffer.data()), &size, FALSE);
    if (result == ERROR_INSUFFICIENT_BUFFER)
    {
        buffer.resize(size);
        result = GetIfTable(
            reinterpret_cast<MIB_IFTABLE*>(buffer.data()), &size, FALSE);
    }
    return result == NO_ERROR;
}

static std::string RowDescription(const MIB_IFROW& row)
{
    DWORD length = row.dwDescrLen;
    if (length > MAXLEN_IFDESCR)
        length = MAXLEN_IFDESCR;
    return std::string(reinterpret_cast<const char*>(row.bDescr), length);
}

static int FindConnectionInIfTable(const std::string& connection, const MIB_IFTABLE* table)
{
    for (DWORD i = 0; i < table->dwNumEntries; ++i)
    {
        if (RowDescription(table->table[i]) == connection)
            return static_cast<int>(i);
    }
    return -1;
}

static int FindConnectionInIfTableFuzzy(const std::string& connection, const MIB_IFTABLE* table)
{
    for (DWORD i = 0; i < table->dwNumEntries; ++i)
    {
        std::string description = RowDescription(table->table[i]);
        size_t found;
        if (description.size() >= connection.size())
            found = description.find(connection);
        else
            found = connection.find(description);
        if (found != std::string::npos)
            return static_cast<int>(i);
    }
    return -1;
}

static void GetAdapterInfo(std::vector<NetConnection>& adapters)
{
    ULONG size = 0;
    if (GetAdaptersInfo(nullptr, &size) != ERROR_BUFFER_OVERFLOW || size == 0)
        return;

    std::vector<BYTE> buffer(size);
    PIP_ADAPTER_INFO info = reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());
    if (GetAdaptersInfo(info, &size) != ERROR_SUCCESS)
        return;

    for (PIP_ADAPTER_INFO current = info; current != nullptr; current = current->Next)
    {
        NetConnection connection;
        connection.description = current->Description;
        connection.ip = AnsiToWide(current->IpAddressList.IpAddress.String);
        adapters.push_back(connection);
    }
}

static void FillIfTableInfo(std::vector<NetConnection>& adapters, const MIB_IFTABLE* table)
{
    for (NetConnection& connection : adapters)
    {
        if (connection.description.empty())
            continue;

        int index = FindConnectionInIfTable(connection.description, table);
        if (index < 0)
            index = FindConnectionInIfTableFuzzy(connection.description, table);
        if (index < 0)
            continue;

        const MIB_IFROW& row = table->table[index];
        connection.index = index;
        connection.inBytes = row.dwInOctets;
        connection.outBytes = row.dwOutOctets;
        connection.description2 = RowDescription(row);
        connection.operStatus = row.dwOperStatus;
    }
}

static void GetAllIfTableInfo(std::vector<NetConnection>& adapters, const MIB_IFTABLE* table)
{
    std::vector<NetConnection> withIp;
    GetAdapterInfo(withIp);
    adapters.clear();

    for (DWORD i = 0; i < table->dwNumEntries; ++i)
    {
        const MIB_IFROW& row = table->table[i];
        NetConnection connection;
        connection.description = RowDescription(row);
        connection.description2 = connection.description;
        connection.index = static_cast<int>(i);
        connection.inBytes = row.dwInOctets;
        connection.outBytes = row.dwOutOctets;
        connection.operStatus = row.dwOperStatus;

        for (const NetConnection& candidate : withIp)
        {
            if (!candidate.description.empty() &&
                connection.description.find(candidate.description) != std::string::npos)
            {
                connection.ip = candidate.ip;
                break;
            }
        }
        adapters.push_back(connection);
    }
}

static void SplitList(const std::wstring& text, std::vector<std::wstring>& items)
{
    size_t start = 0;
    while (start <= text.size())
    {
        size_t separator = text.find(L',', start);
        std::wstring item = TrimCopy(text.substr(
            start,
            (separator == std::wstring::npos ? text.size() : separator) - start));
        if (!item.empty())
            items.push_back(item);
        if (separator == std::wstring::npos)
            break;
        start = separator + 1;
    }
}

static std::wstring NormalizeIp(const std::wstring& ip)
{
    if (ip.empty() || ip == L"0.0.0.0")
        return L"-.-.-.-";
    return ip;
}

///////////////////////////////////////////////////////////////////////////////
// Resolucion de la IPv4

static std::wstring ResolveConfigPath()
{
    std::wstring pluginDirectory = GetModuleDirectory(g_hInstance);
    std::wstring exeDirectory = ParentDirectory(pluginDirectory);
    if (exeDirectory.empty())
        exeDirectory = pluginDirectory;

    std::wstring appDataDirectory = GetAppDataTrafficMonitorDirectory();

    IniFile globalConfig;
    globalConfig.Load(exeDirectory + L"\\global_cfg.ini");

    bool portableDefault = !FileExists(appDataDirectory + L"config.ini");
    bool portable = globalConfig.GetBool(L"config", L"portable_mode", portableDefault);

    std::wstring primary = (portable ? exeDirectory : appDataDirectory) + L"config.ini";
    std::wstring secondary = (portable ? appDataDirectory : exeDirectory) + L"config.ini";

    if (FileExists(primary))
        return primary;
    if (FileExists(secondary))
        return secondary;
    return primary;
}

static std::wstring ResolveIpv4()
{
    static std::wstring configPath;

    IniFile config;
    if (configPath.empty())
        configPath = ResolveConfigPath();
    config.Load(configPath);

    bool autoSelect = config.GetBool(L"connection", L"auto_select", true);
    bool selectAll = config.GetBool(L"connection", L"select_all", false);
    std::wstring connectionName = config.GetString(L"connection", L"connection_name", L"");
    bool showAll = config.GetBool(L"general", L"show_all_interface", false);

    std::vector<BYTE> ifTableBuffer;
    MIB_IFTABLE* table = LoadIfTable(ifTableBuffer)
        ? reinterpret_cast<MIB_IFTABLE*>(ifTableBuffer.data())
        : nullptr;

    std::vector<NetConnection> connections;

    if (showAll && table != nullptr)
    {
        GetAllIfTableInfo(connections, table);
    }
    else
    {
        std::vector<NetConnection> adapters;
        GetAdapterInfo(adapters);

        std::vector<std::wstring> hidden;
        SplitList(config.GetString(L"general", L"connections_hide", L""), hidden);

        for (NetConnection& adapter : adapters)
        {
            std::wstring name = AnsiToWide(adapter.description);
            bool isHidden = false;
            for (const std::wstring& hide : hidden)
            {
                if (hide == name)
                {
                    isHidden = true;
                    break;
                }
            }
            if (!isHidden)
                connections.push_back(adapter);
        }

        if (table != nullptr)
            FillIfTableInfo(connections, table);
    }

    if (connections.empty())
        return L"-.-.-.-";

    if (selectAll)
    {
        std::wstring result;
        for (const NetConnection& connection : connections)
        {
            std::wstring ip = NormalizeIp(connection.ip);
            if (ip == L"-.-.-.-")
                continue;
            if (!result.empty())
                result += L' ';
            result += ip;
        }
        return result.empty() ? L"-.-.-.-" : result;
    }

    int selected = 0;
    if (autoSelect)
    {
        unsigned long long maxBytes = 0;
        bool found = false;
        for (size_t i = 0; i < connections.size(); ++i)
        {
            if (connections[i].operStatus == IF_OPER_STATUS_OPERATIONAL)
            {
                unsigned long long bytes =
                    connections[i].inBytes + connections[i].outBytes;
                if (!found || bytes > maxBytes)
                {
                    maxBytes = bytes;
                    selected = static_cast<int>(i);
                    found = true;
                }
            }
        }
    }
    else if (!connectionName.empty())
    {
        std::string expected = WideToAnsi(connectionName);
        for (size_t i = 0; i < connections.size(); ++i)
        {
            if (connections[i].description2 == expected)
            {
                selected = static_cast<int>(i);
                break;
            }
        }
    }

    return NormalizeIp(connections[static_cast<size_t>(selected)].ip);
}

///////////////////////////////////////////////////////////////////////////////
// Interfaces del plugin

class CIpAddressItem : public IPluginItem
{
public:
    const wchar_t* GetItemName() const override { return L"IPv4 (TrafficMonitorJr)"; }
    const wchar_t* GetItemId() const override { return L"TMJrIPv4"; }
    const wchar_t* GetItemLableText() const override { return L"IP "; }
    const wchar_t* GetItemValueText() const override { return m_value.c_str(); }
    const wchar_t* GetItemValueSampleText() const override { return L"255.255.255.255"; }

    void Update()
    {
        m_value = ResolveIpv4();
    }

private:
    std::wstring m_value{ L"-.-.-.-" };
};

class CIpAddressPlugin : public ITMPlugin
{
public:
    static CIpAddressPlugin& Instance()
    {
        static CIpAddressPlugin instance;
        return instance;
    }

    IPluginItem* GetItem(int index) override
    {
        return index == 0 ? &m_item : nullptr;
    }

    void DataRequired() override
    {
        m_item.Update();
    }

    const wchar_t* GetInfo(PluginInfoIndex index) override
    {
        switch (index)
        {
        case TMI_NAME:
            return L"TrafficMonitorJr IPv4";
        case TMI_DESCRIPTION:
            return L"Muestra la direccion IPv4 de la tarjeta de red seleccionada en TrafficMonitor.";
        case TMI_AUTHOR:
            return L"TrafficMonitorJr";
        case TMI_COPYRIGHT:
            return L"MIT";
        case TMI_VERSION:
            return L"1.0.0";
        case TMI_URL:
            return L"https://github.com/zhongyang219/TrafficMonitor";
        default:
            return L"";
        }
    }

private:
    CIpAddressItem m_item;
};

extern "C" __declspec(dllexport) ITMPlugin* TMPluginGetInstance()
{
    return &CIpAddressPlugin::Instance();
}
