"""Static/synthetic review of audio-inventory-enforce.ps1; never invokes PowerShell or Core Audio."""
from pathlib import Path
import os
import re


SCRIPT = Path(__file__).with_name("audio-inventory-enforce.ps1")
data = SCRIPT.read_bytes()
assert data.isascii(), "PowerShell 5.1 source must remain ASCII"
text = data.decode("ascii")

match = re.search(r"(?s)Add-Type -TypeDefinition @'\n(.*)\n'@", text)
assert match, "Unexpected script/C# boundary"
assert text.count("[RiconAudioInventory]::Read()") == 1
assert text.count("[RiconAudioInventory]::LayoutCheck()") == 1
csharp = match.group(1)
implementation = csharp.split("    [ComImport", 1)[0]
interfaces = csharp[len(implementation):]

# Scope: only active render endpoints and active sessions; raw endpoint/session IDs are not returned.
required = [
    "EnumAudioEndpoints(EDataFlow.eRender, DEVICE_STATE_ACTIVE",
    "state != AudioSessionState.Active",
    "GetProcessId(out pid)",
    "idSha256 = Sha256(id)",
    'scope = "ACTIVE_RENDER_ENDPOINTS_AND_ACTIVE_SESSIONS"',
    "endpoints.Sort(",
    "rows.Sort(",
    "ConvertTo-Json -Depth $depth",
    "[StructLayout(LayoutKind.Explicit, Size = 24)]",
    "Marshal.SizeOf(typeof(PropVariant))",
    "Marshal.OffsetOf(typeof(PropVariant), \"vt\")",
    "Marshal.OffsetOf(typeof(PropVariant), \"pointer\")",
    "AUDCLNT_S_NO_SINGLE_PROCESS = 0x0889000D",
    "SHARED_INITIAL_PID",
    "INCOMPLETE_SUCCESS_STATUS",
    "hresultHex",
    "Point-in-time observation",
]
for token in required:
    assert token in text, f"Missing scope/determinism guard: {token}"
assert 'if ([IntPtr]::Size -ne 8) { throw "X64_REQUIRED_FOR_PROPVARIANT_LAYOUT" }' in text
assert "int pidHr = control.GetProcessId(out pid);" in implementation
assert "bool singleProcess = pidHr == 0;" in implementation
for forbidden in ("GetSessionIdentifier(", "GetSessionInstanceIdentifier(", "GetDefaultAudioEndpoint("):
    assert forbidden not in implementation, f"Privacy/scope violation: {forbidden}"
assert "public string id { get; set; }" not in csharp
assert "public string session" not in csharp

# Mutating COM methods must exist only as vtable declarations, never as implementation calls.
for forbidden_call in (
    ".SetDisplayName(", ".SetIconPath(", ".SetGroupingParam(", ".SetDuckingPreference(",
    ".SetValue(", ".Commit(", ".GetSimpleAudioVolume(", ".GetAudioSessionControl(",
    ".Register", ".Unregister",
):
    assert forbidden_call not in implementation, f"Mutating or registration call present: {forbidden_call}"
assert "EnumAudioEndpoints(EDataFlow.eCapture" not in implementation

# Every acquired COM category has an explicit finally release; PROPVARIANT is always cleared.
for release in (
    "Release(collection);", "Release(enumerator);", "Release(managerObject);",
    "Release(store);", "Release(device);", "Release(controlObject);",
    "Release(sessionEnumerator);", "PropVariantClear(ref value);",
):
    assert release in implementation, f"Missing cleanup: {release}"
assert implementation.count("finally {") >= 5
assert "Marshal.FinalReleaseComObject(value)" in implementation

# COM identities and vtable order come from the installed Microsoft SDK, not copied constants.
def version_key(path):
    return tuple(int(part) for part in path.name.split(".") if part.isdigit())


configured = os.environ.get("RICON_WINDOWS_SDK_INCLUDE")
include_root = Path(configured) if configured else Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Windows Kits" / "10" / "Include"
candidates = [path for path in include_root.iterdir() if (path / "um" / "mmdeviceapi.h").is_file()
              and (path / "um" / "audiopolicy.h").is_file() and (path / "um" / "propsys.h").is_file()]
assert candidates, f"Installed Windows SDK headers not found under {include_root}"
sdk = max(candidates, key=version_key)
headers = {
    "mmdeviceapi": (sdk / "um" / "mmdeviceapi.h").read_text(encoding="utf-8-sig"),
    "audiopolicy": (sdk / "um" / "audiopolicy.h").read_text(encoding="utf-8-sig"),
    "propsys": (sdk / "um" / "propsys.h").read_text(encoding="utf-8-sig"),
    "deviceKeys": (sdk / "um" / "functiondiscoverykeys_devpkey.h").read_text(encoding="utf-8-sig"),
}
interface_headers = {
    "IMMDeviceEnumerator": "mmdeviceapi",
    "IMMDeviceCollection": "mmdeviceapi",
    "IMMDevice": "mmdeviceapi",
    "IPropertyStore": "propsys",
    "IAudioSessionManager2": "audiopolicy",
    "IAudioSessionEnumerator": "audiopolicy",
    "IAudioSessionControl2": "audiopolicy",
}


def native_guid(name, header):
    match = re.search(r'MIDL_INTERFACE\("([0-9A-Fa-f-]+)"\)\s+' + re.escape(name) + r"\b", header)
    assert match, f"SDK GUID not found for {name}"
    return match.group(1).lower()


def managed_guid(name):
    pattern = (r'\[ComImport, InterfaceType\(ComInterfaceType\.InterfaceIsIUnknown\), Guid\("([0-9A-Fa-f-]+)"\)\]\s+'
               r'interface\s+' + re.escape(name) + r"\b")
    match = re.search(pattern, interfaces)
    assert match, f"Managed GUID not found for {name}"
    return match.group(1).lower()


def native_vtable(name, header):
    match = re.search(r"typedef struct " + re.escape(name) + r"Vtbl\s*\{(.*?)END_INTERFACE", header, re.S)
    assert match, f"SDK vtable not found for {name}"
    methods = re.findall(r"DECLSPEC_XFGVIRT\([^,]+,\s*(\w+)\)", match.group(1))
    return [method for method in methods if method not in ("QueryInterface", "AddRef", "Release")]


def managed_vtable(name):
    match = re.search(r"interface\s+" + re.escape(name) + r"\s*\{(.*?)\n    \}", interfaces, re.S)
    assert match, f"Managed vtable not found for {name}"
    return re.findall(r"\[PreserveSig\]\s+int\s+(\w+)\s*\(", match.group(1))


for interface_name, header_name in interface_headers.items():
    header = headers[header_name]
    assert managed_guid(interface_name) == native_guid(interface_name, header), f"IID drift: {interface_name}"
    assert managed_vtable(interface_name) == native_vtable(interface_name, header), f"vtable drift: {interface_name}"

class_match = re.search(r'\[ComImport, Guid\("([0-9A-Fa-f-]+)"\)\]\s+class\s+MMDeviceEnumeratorComObject', interfaces)
sdk_class = re.search(r'class\s+DECLSPEC_UUID\("([0-9A-Fa-f-]+)"\)\s+MMDeviceEnumerator', headers["mmdeviceapi"])
assert class_match and sdk_class and class_match[1].lower() == sdk_class[1].lower(), "MMDeviceEnumerator CLSID drift"
manager_iid = re.search(r'IID_IAudioSessionManager2\s*=\s*new Guid\("([0-9A-Fa-f-]+)"\)', csharp)
assert manager_iid and manager_iid[1].lower() == native_guid("IAudioSessionManager2", headers["audiopolicy"])
assert csharp.lower().count(manager_iid[1].lower()) == 2, "Manager2 activation/interface IID drift"
assert re.search(r"DEFINE_PROPERTYKEY\(PKEY_Device_FriendlyName,\s*0xa45c254e,\s*0xdf1c,\s*0x4efd,\s*0x80,\s*0x20,\s*0x67,\s*0xd1,\s*0x46,\s*0xa8,\s*0x50,\s*0xe0,\s*14\)", headers["deviceKeys"])
assert 'new Guid("a45c254e-df1c-4efd-8020-67d146a850e0"), 14' in csharp

# Synthetic ordering mirrors the public output rule without touching the OS.
endpoints = [
    {"idSha256": "ff", "sessions": [(9, "z"), (2, "b")]},
    {"idSha256": "00", "sessions": [(2, "a"), (2, "a")]},
]
endpoints.sort(key=lambda row: row["idSha256"])
for endpoint_index, endpoint in enumerate(endpoints):
    endpoint["index"] = endpoint_index
    endpoint["sessions"].sort(key=lambda row: (row[0], row[1]))
assert [row["idSha256"] for row in endpoints] == ["00", "ff"]
assert endpoints[0]["sessions"] == [(2, "a"), (2, "a")]
assert endpoints[1]["sessions"] == [(2, "b"), (9, "z")]

print(f"audio-inventory static selftest: PASS (SDK {sdk.name} GUID/vtables, read-only scope, privacy, ordering, cleanup; Core Audio not invoked)")
