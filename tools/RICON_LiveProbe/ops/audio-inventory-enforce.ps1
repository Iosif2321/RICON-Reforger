# TOOL_ONLY: read active Windows Core Audio render endpoints and sessions.
# Windows PowerShell 5.1. Read-only: no capture, playback, routing, volume or default-device calls.
[CmdletBinding()]
param([switch]$Pretty)

Set-StrictMode -Version 2.0
$ErrorActionPreference = "Stop"

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;

public sealed class RiconAudioSessionRow {
    public uint pid { get; set; }
    public string process { get; set; }
    public int ordinal { get; set; }
}

public sealed class RiconAudioEndpointRow {
    public int index { get; set; }
    public string idSha256 { get; set; }
    public string name { get; set; }
    public RiconAudioSessionRow[] activeSessions { get; set; }
}

public sealed class RiconAudioInventoryResult {
    public string result { get; set; }
    public string scope { get; set; }
    public RiconAudioEndpointRow[] renderEndpoints { get; set; }
}

public static class RiconAudioInventory {
    const uint DEVICE_STATE_ACTIVE = 0x1;
    const int STGM_READ = 0;
    const uint CLSCTX_ALL = 23;
    static readonly Guid IID_IAudioSessionManager2 = new Guid("77aa99a0-1bd6-484f-8bc7-2c654c9a9b6f");
    static readonly PropertyKey PKEY_Device_FriendlyName = new PropertyKey(
        new Guid("a45c254e-df1c-4efd-8020-67d146a850e0"), 14);

    public static RiconAudioInventoryResult Read() {
        IMMDeviceEnumerator enumerator = null;
        IMMDeviceCollection collection = null;
        var endpoints = new List<RiconAudioEndpointRow>();
        try {
            enumerator = (IMMDeviceEnumerator)new MMDeviceEnumeratorComObject();
            Check(enumerator.EnumAudioEndpoints(EDataFlow.eRender, DEVICE_STATE_ACTIVE, out collection), "EnumAudioEndpoints");
            uint count;
            Check(collection.GetCount(out count), "GetCount(endpoints)");
            for (uint i = 0; i < count; i++) endpoints.Add(ReadEndpoint(collection, i));
            endpoints.Sort((a, b) => StringComparer.Ordinal.Compare(a.idSha256, b.idSha256));
            for (int i = 0; i < endpoints.Count; i++) endpoints[i].index = i;
            return new RiconAudioInventoryResult {
                result = "OK",
                scope = "ACTIVE_RENDER_ENDPOINTS_AND_ACTIVE_SESSIONS",
                renderEndpoints = endpoints.ToArray()
            };
        } finally {
            Release(collection);
            Release(enumerator);
        }
    }

    static RiconAudioEndpointRow ReadEndpoint(IMMDeviceCollection collection, uint sourceIndex) {
        IMMDevice device = null;
        IPropertyStore store = null;
        object managerObject = null;
        try {
            Check(collection.Item(sourceIndex, out device), "Item(endpoint)");
            string id;
            Check(device.GetId(out id), "GetId");
            Check(device.OpenPropertyStore(STGM_READ, out store), "OpenPropertyStore");
            string name = ReadString(store, PKEY_Device_FriendlyName);
            Guid iid = IID_IAudioSessionManager2;
            Check(device.Activate(ref iid, CLSCTX_ALL, IntPtr.Zero, out managerObject), "Activate(IAudioSessionManager2)");
            var sessions = ReadSessions((IAudioSessionManager2)managerObject);
            return new RiconAudioEndpointRow {
                idSha256 = Sha256(id),
                name = name ?? "",
                activeSessions = sessions
            };
        } finally {
            Release(managerObject);
            Release(store);
            Release(device);
        }
    }

    static RiconAudioSessionRow[] ReadSessions(IAudioSessionManager2 manager) {
        IAudioSessionEnumerator sessionEnumerator = null;
        var rows = new List<RiconAudioSessionRow>();
        try {
            Check(manager.GetSessionEnumerator(out sessionEnumerator), "GetSessionEnumerator");
            int count;
            Check(sessionEnumerator.GetCount(out count), "GetCount(sessions)");
            for (int i = 0; i < count; i++) {
                object controlObject = null;
                try {
                    Check(sessionEnumerator.GetSession(i, out controlObject), "GetSession");
                    var control = (IAudioSessionControl2)controlObject;
                    AudioSessionState state;
                    Check(control.GetState(out state), "GetState");
                    if (state != AudioSessionState.Active) continue;
                    uint pid;
                    Check(control.GetProcessId(out pid), "GetProcessId");
                    rows.Add(new RiconAudioSessionRow { pid = pid, process = ProcessName(pid) });
                } finally {
                    Release(controlObject);
                }
            }
            rows.Sort((a, b) => {
                int byPid = a.pid.CompareTo(b.pid);
                return byPid != 0 ? byPid : StringComparer.Ordinal.Compare(a.process, b.process);
            });
            for (int i = 0; i < rows.Count; i++) rows[i].ordinal = i;
            return rows.ToArray();
        } finally {
            Release(sessionEnumerator);
        }
    }

    static string ProcessName(uint pid) {
        if (pid == 0) return "SYSTEM_SOUNDS";
        try { using (var process = Process.GetProcessById((int)pid)) { return process.ProcessName; } }
        catch { return "UNAVAILABLE"; }
    }

    static string ReadString(IPropertyStore store, PropertyKey key) {
        PropVariant value = new PropVariant();
        try {
            Check(store.GetValue(ref key, out value), "GetValue(FriendlyName)");
            return value.vt == 31 && value.pointer != IntPtr.Zero ? Marshal.PtrToStringUni(value.pointer) : "";
        } finally {
            PropVariantClear(ref value);
        }
    }

    static string Sha256(string value) {
        using (var sha = SHA256.Create()) {
            byte[] digest = sha.ComputeHash(Encoding.UTF8.GetBytes(value ?? ""));
            var text = new StringBuilder(64);
            foreach (byte b in digest) text.Append(b.ToString("x2"));
            return text.ToString();
        }
    }

    static void Check(int hr, string operation) {
        if (hr < 0) Marshal.ThrowExceptionForHR(hr, new IntPtr(-1));
    }

    static void Release(object value) {
        if (value != null && Marshal.IsComObject(value)) Marshal.FinalReleaseComObject(value);
    }

    [DllImport("ole32.dll")]
    static extern int PropVariantClear(ref PropVariant value);

    enum EDataFlow { eRender = 0, eCapture = 1, eAll = 2 }
    enum AudioSessionState { Inactive = 0, Active = 1, Expired = 2 }

    [StructLayout(LayoutKind.Sequential)]
    struct PropertyKey {
        public Guid formatId;
        public int propertyId;
        public PropertyKey(Guid formatId, int propertyId) { this.formatId = formatId; this.propertyId = propertyId; }
    }

    [StructLayout(LayoutKind.Explicit)]
    struct PropVariant {
        [FieldOffset(0)] public ushort vt;
        [FieldOffset(8)] public IntPtr pointer;
    }

    [ComImport, Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")]
    class MMDeviceEnumeratorComObject { }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("A95664D2-9614-4F35-A746-DE8DB63617E6")]
    interface IMMDeviceEnumerator {
        [PreserveSig] int EnumAudioEndpoints(EDataFlow flow, uint stateMask, out IMMDeviceCollection devices);
        [PreserveSig] int GetDefaultAudioEndpoint(EDataFlow flow, int role, out IMMDevice endpoint);
        [PreserveSig] int GetDevice([MarshalAs(UnmanagedType.LPWStr)] string id, out IMMDevice device);
        [PreserveSig] int RegisterEndpointNotificationCallback(IntPtr callback);
        [PreserveSig] int UnregisterEndpointNotificationCallback(IntPtr callback);
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("0BD7A1BE-7A1A-44DB-8397-C0A4A5B2A6C8")]
    interface IMMDeviceCollection {
        [PreserveSig] int GetCount(out uint count);
        [PreserveSig] int Item(uint index, out IMMDevice device);
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("D666063F-1587-4E43-81F1-B948E807363F")]
    interface IMMDevice {
        [PreserveSig] int Activate(ref Guid iid, uint classContext, IntPtr activationParams,
            [MarshalAs(UnmanagedType.IUnknown)] out object instance);
        [PreserveSig] int OpenPropertyStore(int access, out IPropertyStore properties);
        [PreserveSig] int GetId([MarshalAs(UnmanagedType.LPWStr)] out string id);
        [PreserveSig] int GetState(out uint state);
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99")]
    interface IPropertyStore {
        [PreserveSig] int GetCount(out uint count);
        [PreserveSig] int GetAt(uint index, out PropertyKey key);
        [PreserveSig] int GetValue(ref PropertyKey key, out PropVariant value);
        [PreserveSig] int SetValue(ref PropertyKey key, ref PropVariant value);
        [PreserveSig] int Commit();
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("77AA99A0-1BD6-484F-8BC7-2C654C9A9B6F")]
    interface IAudioSessionManager2 {
        [PreserveSig] int GetAudioSessionControl(ref Guid sessionGuid, uint streamFlags, out IntPtr control);
        [PreserveSig] int GetSimpleAudioVolume(ref Guid sessionGuid, uint streamFlags, out IntPtr volume);
        [PreserveSig] int GetSessionEnumerator(out IAudioSessionEnumerator enumerator);
        [PreserveSig] int RegisterSessionNotification(IntPtr notification);
        [PreserveSig] int UnregisterSessionNotification(IntPtr notification);
        [PreserveSig] int RegisterDuckNotification([MarshalAs(UnmanagedType.LPWStr)] string sessionId, IntPtr notification);
        [PreserveSig] int UnregisterDuckNotification(IntPtr notification);
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("E2F5BB11-0570-40CA-ACDD-3AA01277DEE8")]
    interface IAudioSessionEnumerator {
        [PreserveSig] int GetCount(out int count);
        [PreserveSig] int GetSession(int index, [MarshalAs(UnmanagedType.IUnknown)] out object control);
    }

    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("BFB7FF88-7239-4FC9-8FA2-07C950BE9C6D")]
    interface IAudioSessionControl2 {
        [PreserveSig] int GetState(out AudioSessionState state);
        [PreserveSig] int GetDisplayName(out IntPtr name);
        [PreserveSig] int SetDisplayName([MarshalAs(UnmanagedType.LPWStr)] string value, ref Guid context);
        [PreserveSig] int GetIconPath(out IntPtr path);
        [PreserveSig] int SetIconPath([MarshalAs(UnmanagedType.LPWStr)] string value, ref Guid context);
        [PreserveSig] int GetGroupingParam(out Guid grouping);
        [PreserveSig] int SetGroupingParam(ref Guid grouping, ref Guid context);
        [PreserveSig] int RegisterAudioSessionNotification(IntPtr notification);
        [PreserveSig] int UnregisterAudioSessionNotification(IntPtr notification);
        [PreserveSig] int GetSessionIdentifier(out IntPtr value);
        [PreserveSig] int GetSessionInstanceIdentifier(out IntPtr value);
        [PreserveSig] int GetProcessId(out uint pid);
        [PreserveSig] int IsSystemSoundsSession();
        [PreserveSig] int SetDuckingPreference([MarshalAs(UnmanagedType.Bool)] bool optOut);
    }
}
'@

$result = [RiconAudioInventory]::Read()
$depth = 6
if ($Pretty) {
    $result | ConvertTo-Json -Depth $depth
} else {
    $result | ConvertTo-Json -Depth $depth -Compress
}
