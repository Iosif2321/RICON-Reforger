"""Static/synthetic review of audio-inventory-enforce.ps1; never invokes PowerShell or Core Audio."""
from pathlib import Path
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

# Interface identities and read methods are pinned so vtable drift is visible in review.
assert csharp.lower().count("77aa99a0-1bd6-484f-8bc7-2c654c9a9b6f") == 2, "Manager2 activation/interface IID drift"
for guid in (
    "BCDE0395-E52F-467C-8E3D-C4579291692E",
    "A95664D2-9614-4F35-A746-DE8DB63617E6",
    "0BD7A1BE-7A1A-44DB-8397-C0A4A5B2A6C8",
    "D666063F-1587-4E43-81F1-B948E807363F",
    "886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99",
    "77AA99A0-1BD6-484F-8BC7-2C654C9A9B6F",
    "E2F5BB11-0570-40CA-ACDD-3AA01277DEE8",
    "BFB7FF88-7239-4FC9-8FA2-07C950BE9C6D",
):
    assert guid.lower() in interfaces.lower(), f"Missing COM identity: {guid}"

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

print("audio-inventory static selftest: PASS (read-only scope, privacy, deterministic ordering, COM cleanup; Core Audio not invoked)")
