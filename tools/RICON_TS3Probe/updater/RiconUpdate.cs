using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Net;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.AccessControl;
using System.Security.Cryptography;
using System.Security.Principal;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using Microsoft.Win32.SafeHandles;

// A temporary copy waits for the verified TS3 process. Feed and signing key belong to the publisher build.
internal static class RiconUpdate
{
    private const string InstallerSha256 = "830bd91496d0192f19771a9ddbdb1a096eee16fb88ff45954770a5bbdaebc49a";
    private const string PluginName = "ricon_pcm_probe.dll";
    private const string UpdaterEntry = "plugins/ricon_pcm_probe/RiconUpdate.exe";
    private const long PackageLimit = 64L * 1024 * 1024;
    private const long EntryLimit = 24L * 1024 * 1024;
    private const int DownloadBudgetMs = 60000;
    private const int InstallerBudgetMs = 60000;
    private const string HelperRelative = "ricon_pcm_probe/RiconUpdate.exe";
    private static readonly Encoding StrictUtf8 = new UTF8Encoding(false, true);

    private sealed class UpdateError : Exception
    {
        internal string RecoveryPath;
        internal UpdateError(string reason) : base(reason) { }
        internal UpdateError(string reason, string recoveryPath) : base(reason) { RecoveryPath = recoveryPath; }
    }

    private sealed class ReleaseVersion : IComparable<ReleaseVersion>
    {
        internal string Text;
        internal int[] Parts;

        internal static ReleaseVersion Parse(string text)
        {
            Require(text != null && Regex.IsMatch(text,
                @"\A(0|[1-9][0-9]{0,4})\.(0|[1-9][0-9]{0,4})\.(0|[1-9][0-9]{0,4})\z"), "invalid-version");
            string[] fields = text.Split('.');
            int[] parts = new int[3];
            for (int i = 0; i < parts.Length; i++)
            {
                parts[i] = Int32.Parse(fields[i], CultureInfo.InvariantCulture);
                Require(parts[i] <= 65535, "invalid-version");
            }
            return new ReleaseVersion { Text = text, Parts = parts };
        }

        public int CompareTo(ReleaseVersion other)
        {
            for (int i = 0; i < Parts.Length; i++)
            {
                int difference = Parts[i].CompareTo(other.Parts[i]);
                if (difference != 0) return difference;
            }
            return 0;
        }
    }

    private sealed class Installed
    {
        internal string Prefix;
        internal string Version;
        internal string Files;
        internal string PluginHash;
        internal string UpdaterHash;
        internal string Description;
    }

    private sealed class Package
    {
        internal string Name;
        internal string Version;
        internal string PluginHash;
        internal string UpdaterHash;
        internal string Description;
    }

    private sealed class Pins : IDisposable
    {
        private readonly List<SafeFileHandle> handles = new List<SafeFileHandle>();

        internal void Directory(string path)
        {
            string full = FullPath(path);
            Require(System.IO.Directory.Exists(full), "directory-missing");
            string root = Path.GetPathRoot(full);
            string current = root;
            OpenDirectory(current);
            foreach (string segment in full.Substring(root.Length).Split(new char[] { Path.DirectorySeparatorChar }, StringSplitOptions.RemoveEmptyEntries))
            {
                current = Path.Combine(current, segment);
                OpenDirectory(current);
            }
        }

        private void OpenDirectory(string path)
        {
            NoReparse(path);
            SafeFileHandle handle = CreateFile(path, 0, 3, IntPtr.Zero, 3, 0x02200000, IntPtr.Zero);
            Require(!handle.IsInvalid, "directory-pin-failed");
            handles.Add(handle);
            NoReparse(path);
            Require(SamePath(FinalPath(handle), path), "directory-redirected");
        }

        public void Dispose()
        {
            for (int i = handles.Count - 1; i >= 0; i--) handles[i].Dispose();
            handles.Clear();
        }
    }

    private static int Main(string[] args)
    {
        string status = null;
        string jobDirectory = null;
        FileStream ownerLock = null;
        bool preserveRecovery = false;
        try
        {
            Require(Environment.Is64BitProcess && args.Length == 5, "invalid-arguments");
            int pid;
            long creation;
            Require(Int32.TryParse(args[0], NumberStyles.None, CultureInfo.InvariantCulture, out pid) && pid > 0
                && pid.ToString(CultureInfo.InvariantCulture) == args[0], "invalid-pid");
            Require(Int64.TryParse(args[1], NumberStyles.None, CultureInfo.InvariantCulture, out creation) && creation > 0
                && creation.ToString(CultureInfo.InvariantCulture) == args[1], "invalid-creation-time");
            string profile = FullPath(args[2]);
            ReleaseVersion installedVersion = ReleaseVersion.Parse(args[3]);
            ReleaseVersion wanted = ReleaseVersion.Parse(args[4]);
            Require(wanted.CompareTo(installedVersion) > 0, "version-not-newer");
            using (Pins pins = new Pins())
            {
                pins.Directory(profile);
                string updateRoot = Path.Combine(profile, "ricon-update");
                EnsurePrivateDirectory(updateRoot);
                pins.Directory(updateRoot);
                string ownExecutable = FullPath(Assembly.GetExecutingAssembly().Location);
                Require(Within(updateRoot, ownExecutable), "helper-outside-update-directory");
                NoReparse(ownExecutable);
                string jobKey = pid.ToString(CultureInfo.InvariantCulture) + "-" + creation.ToString(CultureInfo.InvariantCulture) + "-" + wanted.Text;
                status = Path.Combine(updateRoot, "status-" + jobKey + ".txt");
                NoReparseIfExists(status);
                try { ownerLock = ExclusiveLock(Path.Combine(updateRoot, "owner-" + jobKey + ".lock")); }
                catch (IOException error) { if (SharingViolation(error)) return 0; throw; }
                if (File.Exists(status) && ReadText(status, 32) == "APPLIED") return 0;
                using (Process host = Process.GetProcessById(pid))
                {
                    string hostImage = VerifyHost(host, creation);
                    pins.Directory(Path.GetDirectoryName(hostImage));
                    string pluginPath = Path.Combine(profile, "plugins", PluginName);
                    pins.Directory(Path.GetDirectoryName(pluginPath));
                    Require(HasModule(host, pluginPath) && !host.HasExited, "profile-plugin-not-loaded");
                    using (FileStream original = LockedRead(pluginPath))
                    {
                        string originalHash = Hash(original);
                        Installed before = ReadInstalled(profile, originalHash);
                        Require(before.Version == installedVersion.Text, "installed-version-mismatch");
                        string installer = Path.Combine(Path.GetDirectoryName(hostImage), "package_inst.exe");
                        using (FileStream installerPin = LockedRead(installer))
                        {
                            Require(Hash(installerPin) == InstallerSha256, "installer-pin-mismatch");
                            jobDirectory = Path.Combine(updateRoot, "stage-" + Guid.NewGuid().ToString("N"));
                            EnsurePrivateDirectory(jobDirectory);
                            pins.Directory(jobDirectory);
                            string archive = Path.Combine(jobDirectory, "RICON.ts3_plugin");
                            string signature = Path.Combine(jobDirectory, "RICON.sig");
                            Stage(profile, wanted.Text, archive, signature);
                            using (FileStream staged = LockedRead(archive))
                            {
                                Authenticate(wanted.Text, Hash(staged), ReadText(signature, 8192));
                                Package package = InspectPackage(staged, wanted.Text);
                                Require(Uri.UnescapeDataString(before.Prefix) == package.Name, "addon-name-mismatch");
                                Publish(status, "STAGED", "signature-and-package-verified");
                                original.Dispose();
                                host.WaitForExit();
                                WaitForProfileClients(pluginPath);
                                using (FileStream applyLock = ApplyLock(updateRoot))
                                {
                                    WaitForProfileClients(pluginPath);
                                    Installed current = ReadInstalled(profile, HashPath(pluginPath));
                                    if (MatchesTarget(profile, before, package))
                                    {
                                        Publish(status, "APPLIED", "already-applied");
                                        return 0;
                                    }
                                    Require(current.Version == before.Version && current.PluginHash == before.PluginHash
                                        && current.Prefix == before.Prefix && current.Files == before.Files, "installed-state-changed");
                                    // The vendor must be able to replace the old DLL only after every loaded client has exited.
                                    Require(HashPath(pluginPath) == before.PluginHash && Hash(installerPin) == InstallerSha256,
                                        "installation-input-changed");
                                    string applied = ApplyRelease(profile, archive, jobDirectory, before, package,
                                        delegate(string input) { Install(installer, input, profile, jobDirectory); });
                                    Publish(status, "APPLIED", applied);
                                }
                            }
                        }
                    }
                }
            }
            return 0;
        }
        catch (Exception error)
        {
            string reason = error is UpdateError ? error.Message : "failure-" + error.GetType().Name;
            UpdateError failedUpdate = error as UpdateError;
            if (failedUpdate != null && failedUpdate.RecoveryPath != null)
            {
                preserveRecovery = true;
                reason += " recovery=" + failedUpdate.RecoveryPath;
            }
            if (status != null)
            {
                try { Publish(status, "ERROR", reason); }
                catch { Console.Error.WriteLine("ERROR status-publication-failed"); }
            }
            Console.Error.WriteLine("ERROR " + reason);
            return 1;
        }
        finally
        {
            if (ownerLock != null) ownerLock.Dispose();
            if (jobDirectory != null && !preserveRecovery) CleanupStage(jobDirectory);
        }
    }

    private static string VerifyHost(Process host, long expectedCreation)
    {
        long created, exited, kernel, user;
        Require(GetProcessTimes(host.Handle, out created, out exited, out kernel, out user) && created == expectedCreation, "host-identity-mismatch");
        StringBuilder name = new StringBuilder(32768);
        int length = name.Capacity;
        Require(QueryFullProcessImageName(host.Handle, 0, name, ref length), "host-image-unavailable");
        string path = FullPath(name.ToString());
        Require(String.Equals(Path.GetFileName(path), "ts3client_win64.exe", StringComparison.OrdinalIgnoreCase), "wrong-host-image");
        NoReparse(path);
        Require(!host.HasExited, "host-exited-before-verification");
        return path;
    }

    private static bool HasModule(Process process, string target)
    {
        foreach (ProcessModule module in process.Modules)
            if (SamePath(module.FileName, target)) return true;
        return false;
    }

    private static void WaitForProfileClients(string pluginPath)
    {
        for (;;)
        {
            if (!ProfileClientLoaded(pluginPath)) return;
            Thread.Sleep(500);
        }
    }

    private static bool ProfileClientLoaded(string pluginPath)
    {
        bool loaded = false;
        bool unreadable = false;
        foreach (Process peer in Process.GetProcessesByName("ts3client_win64"))
        {
            using (peer)
            {
                try { if (!peer.HasExited && HasModule(peer, pluginPath)) loaded = true; }
                catch (InvalidOperationException) { }
                catch (System.ComponentModel.Win32Exception) { unreadable = true; }
            }
        }
        return loaded || (unreadable && TargetWriteLocked(pluginPath));
    }

    private static bool TargetWriteLocked(string path)
    {
        NoReparseIfExists(path);
        if (!File.Exists(path)) return false;
        try
        {
            using (FileStream probe = new FileStream(path, FileMode.Open, FileAccess.Write, FileShare.None))
                Require(SamePath(FinalPath(probe.SafeFileHandle), path), "write-probe-redirected");
            return false;
        }
        catch (FileNotFoundException) { return false; }
        catch (IOException error) { if (SharingViolation(error)) return true; throw; }
    }

    private static Installed ReadInstalled(string profile, string pluginHash)
    {
        Dictionary<string, string> fields = Ini(ReadText(Path.Combine(profile, "addons.ini"), 1024 * 1024), "plugin");
        string selected = null;
        foreach (KeyValuePair<string, string> field in fields)
        {
            if (!field.Key.EndsWith("\\files", StringComparison.OrdinalIgnoreCase)) continue;
            if (!IncludesFile(field.Value, PluginName)) continue;
            Require(selected == null, "ambiguous-installed-addon");
            selected = field.Key.Substring(0, field.Key.Length - 6);
        }
        Require(selected != null, "installed-addon-missing");
        string version, author;
        Require(fields.TryGetValue(selected + "\\version", out version) && fields.TryGetValue(selected + "\\author", out author)
            && author == "RICON", "installed-metadata-invalid");
        ReleaseVersion.Parse(version);
        string description;
        fields.TryGetValue(selected + "\\description", out description);
        return new Installed { Prefix = selected, Version = version, Files = fields[selected + "\\files"],
            PluginHash = pluginHash, Description = description ?? "" };
    }

    private static bool IncludesFile(string files, string expected)
    {
        foreach (string file in files.Split(','))
            if (String.Equals(file.Trim().Replace('\\', '/'), expected, StringComparison.OrdinalIgnoreCase)) return true;
        return false;
    }

    private static HashSet<string> OwnedFiles(string files, bool requirePlugin)
    {
        HashSet<string> result = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (string raw in files.Split(','))
        {
            string file = raw.Trim().Replace('\\', '/');
            if (file.Length == 0) continue;
            if ((!String.Equals(file, PluginName, StringComparison.OrdinalIgnoreCase)
                && !String.Equals(file, HelperRelative, StringComparison.OrdinalIgnoreCase)) || !result.Add(file))
                return null;
        }
        if (requirePlugin && !result.Contains(PluginName)) return null;
        return result;
    }

    private static bool SameOwnedFiles(string first, string second)
    {
        HashSet<string> a = OwnedFiles(first, true);
        HashSet<string> b = OwnedFiles(second, true);
        return a != null && b != null && a.SetEquals(b);
    }

    private static Dictionary<string, string> InstalledFields(string profile)
    {
        string path = Path.Combine(profile, "addons.ini");
        NoReparseIfExists(path);
        return File.Exists(path) ? Ini(ReadText(path, 1024 * 1024), "plugin")
            : new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
    }

    private static bool NoForeignClaims(Dictionary<string, string> fields, string prefix)
    {
        foreach (KeyValuePair<string, string> field in fields)
        {
            if (!field.Key.EndsWith("\\files", StringComparison.OrdinalIgnoreCase)
                || String.Equals(field.Key, prefix + "\\files", StringComparison.OrdinalIgnoreCase)) continue;
            if (IncludesFile(field.Value, PluginName) || IncludesFile(field.Value, HelperRelative)) return false;
        }
        return true;
    }

    private static bool MatchesTarget(string profile, Installed before, Package target)
    {
        try
        {
            Installed actual = ReadInstalled(profile, HashPath(Path.Combine(profile, "plugins", PluginName)));
            HashSet<string> files = OwnedFiles(actual.Files, true);
            return actual.Version == target.Version && actual.Prefix == before.Prefix && actual.PluginHash == target.PluginHash
                && actual.Description == target.Description && files != null && files.Count == 2 && files.Contains(HelperRelative)
                && HashPath(Path.Combine(profile, "plugins", HelperRelative)) == target.UpdaterHash
                && NoForeignClaims(InstalledFields(profile), before.Prefix);
        }
        catch { return false; }
    }

    private static bool MatchesOld(string profile, Installed before)
    {
        try
        {
            Installed actual = ReadInstalled(profile, HashPath(Path.Combine(profile, "plugins", PluginName)));
            string helper = Path.Combine(profile, "plugins", HelperRelative);
            NoReparseIfExists(helper);
            bool helperMatches = before.UpdaterHash == null ? !File.Exists(helper) && !Directory.Exists(helper)
                : HashPath(helper) == before.UpdaterHash;
            return actual.Version == before.Version && actual.Prefix == before.Prefix && actual.PluginHash == before.PluginHash
                && actual.Description == before.Description && SameOwnedFiles(actual.Files, before.Files) && helperMatches
                && NoForeignClaims(InstalledFields(profile), before.Prefix);
        }
        catch { return false; }
    }

    private static bool KnownFileOrAbsent(string path, string oldHash, string targetHash)
    {
        NoReparseIfExists(path);
        if (!File.Exists(path)) return !Directory.Exists(path);
        string actual = HashPath(path);
        return actual == oldHash || actual == targetHash;
    }

    private static bool SafeToRestore(string profile, Installed before, Package target)
    {
        try
        {
            if (!KnownFileOrAbsent(Path.Combine(profile, "plugins", PluginName), before.PluginHash, target.PluginHash)
                || !KnownFileOrAbsent(Path.Combine(profile, "plugins", HelperRelative), before.UpdaterHash, target.UpdaterHash)) return false;
            Dictionary<string, string> fields = InstalledFields(profile);
            if (!NoForeignClaims(fields, before.Prefix)) return false;
            string value;
            if (fields.TryGetValue(before.Prefix + "\\version", out value) && value.Length != 0 && value != before.Version && value != target.Version) return false;
            if (fields.TryGetValue(before.Prefix + "\\author", out value) && value.Length != 0 && value != "RICON") return false;
            if (fields.TryGetValue(before.Prefix + "\\description", out value) && value.Length != 0 && value != before.Description && value != target.Description) return false;
            if (fields.TryGetValue(before.Prefix + "\\files", out value) && OwnedFiles(value, false) == null) return false;
            return true;
        }
        catch { return false; }
    }

    private static string CreateRollback(string profile, string jobDirectory, Installed before)
    {
        HashSet<string> files = OwnedFiles(before.Files, true);
        Require(files != null && NoForeignClaims(InstalledFields(profile), before.Prefix), "unsupported-old-owned-files");
        string helperPath = Path.Combine(profile, "plugins", HelperRelative);
        NoReparseIfExists(helperPath);
        bool hasHelper = files.Contains(HelperRelative);
        Require(File.Exists(helperPath) == hasHelper, "old-helper-state-inconsistent");
        string archive = Path.Combine(jobDirectory, "RICON-rollback.ts3_plugin");
        FileStream helper = null;
        try
        {
            using (FileStream dll = LockedRead(Path.Combine(profile, "plugins", PluginName)))
            {
                Require(Hash(dll) == before.PluginHash, "old-plugin-changed");
                if (hasHelper)
                {
                    helper = LockedRead(helperPath);
                    before.UpdaterHash = Hash(helper);
                }
                Require(MatchesOld(profile, before), "old-state-changed-before-backup");
                using (FileStream output = new FileStream(archive, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                using (ZipArchive zip = new ZipArchive(output, ZipArchiveMode.Create, true))
                {
                    zip.CreateEntry("plugins/");
                    string metadata = "Name = " + Uri.UnescapeDataString(before.Prefix) + "\nType = Plugin\nAuthor = RICON\nVersion = "
                        + before.Version + "\nPlatforms = win64\nDescription = " + before.Description + "\n";
                    using (Stream ini = zip.CreateEntry("package.ini").Open())
                    {
                        byte[] bytes = StrictUtf8.GetBytes(metadata);
                        ini.Write(bytes, 0, bytes.Length);
                    }
                    using (Stream entry = zip.CreateEntry("plugins/" + PluginName).Open()) dll.CopyTo(entry);
                    if (hasHelper)
                    {
                        zip.CreateEntry("plugins/ricon_pcm_probe/");
                        using (Stream entry = zip.CreateEntry(UpdaterEntry).Open()) helper.CopyTo(entry);
                    }
                }
            }
        }
        finally { if (helper != null) helper.Dispose(); }
        return archive;
    }

    // runInstaller is the same pinned vendor for the target and at most one repair.
    private static string ApplyRelease(string profile, string archive, string jobDirectory, Installed before, Package target,
        Action<string> runInstaller)
    {
        string rollback = CreateRollback(profile, jobDirectory, before);
        using (FileStream rollbackPin = LockedRead(rollback))
        {
            string rollbackHash = Hash(rollbackPin);
            Exception failed = null;
            try
            {
                runInstaller(archive);
                Require(MatchesTarget(profile, before, target), "installed-output-mismatch");
                return "verified-installed-output";
            }
            catch (Exception error) { failed = error; }
            string cause = FailureCode(failed);
            if (cause == "installer-timeout-still-running")
                throw KeepRecovery(rollback, jobDirectory, cause);
            if (MatchesTarget(profile, before, target)) return "verified-target-vendor-warning-" + cause;
            if (MatchesOld(profile, before)) throw new UpdateError("apply-failed-unchanged-" + cause);
            bool safe = SafeToRestore(profile, before, target);
            try { safe = safe && !ProfileClientLoaded(Path.Combine(profile, "plugins", PluginName)); }
            catch { safe = false; }
            if (!safe) throw KeepRecovery(rollback, jobDirectory, "recovery-blocked-" + cause);
            Exception repairError = null;
            try
            {
                NoReparse(profile);
                NoReparse(Path.Combine(profile, "plugins"));
                Require(Hash(rollbackPin) == rollbackHash && SafeToRestore(profile, before, target), "recovery-input-changed");
                runInstaller(rollback);
            }
            catch (Exception error) { repairError = error; }
            if (repairError != null && FailureCode(repairError) == "installer-timeout-still-running")
                throw KeepRecovery(rollback, jobDirectory, "recovery-still-running-" + cause);
            if (MatchesOld(profile, before)) throw new UpdateError("apply-failed-restored-" + cause);
            throw KeepRecovery(rollback, jobDirectory, "recovery-unverified-" + cause);
        }
    }

    private static string FailureCode(Exception error)
    {
        return error is UpdateError ? error.Message : "failure-" + error.GetType().Name;
    }

    private static UpdateError KeepRecovery(string archive, string directory, string reason)
    {
        try
        {
            string note = Path.Combine(directory, "recovery.txt");
            NoReparseIfExists(note);
            File.WriteAllText(note, reason + "\r\n" + archive + "\r\n", StrictUtf8);
        }
        catch { }
        return new UpdateError(reason, archive);
    }

    private static Package InspectPackage(FileStream stream, string version)
    {
        stream.Position = 0;
        Package package = new Package();
        using (ZipArchive zip = new ZipArchive(stream, ZipArchiveMode.Read, true))
        {
            HashSet<string> seen = new HashSet<string>(StringComparer.Ordinal);
            Dictionary<string, string> metadata = null;
            foreach (ZipArchiveEntry entry in zip.Entries)
            {
                Require(seen.Add(entry.FullName), "duplicate-package-entry");
                if (entry.FullName == "plugins/" || entry.FullName == "plugins/ricon_pcm_probe/")
                {
                    Require(entry.Length == 0, "invalid-package-directory");
                    continue;
                }
                Require(entry.FullName == "package.ini" || entry.FullName == "plugins/" + PluginName || entry.FullName == UpdaterEntry,
                    "unexpected-package-entry");
                Require(entry.Length > 0 && entry.Length <= EntryLimit, "package-entry-size");
                using (Stream content = entry.Open())
                {
                    if (entry.FullName == "package.ini")
                    {
                        byte[] bytes = ReadBounded(content, 16384);
                        metadata = Ini(StrictUtf8.GetString(bytes), null);
                    }
                    else
                    {
                        string hash = Hash(content, EntryLimit);
                        if (entry.FullName == UpdaterEntry) package.UpdaterHash = hash;
                        else package.PluginHash = hash;
                    }
                }
            }
            string type, author, platforms;
            Require(metadata != null && metadata.TryGetValue("Name", out package.Name)
                && metadata.TryGetValue("Version", out package.Version), "package-metadata-missing");
            Require(metadata.TryGetValue("Type", out type) && type == "Plugin"
                && metadata.TryGetValue("Author", out author) && author == "RICON"
                && metadata.TryGetValue("Platforms", out platforms) && platforms == "win64", "wrong-package-type");
            Require(package.Version == version && package.PluginHash != null && package.UpdaterHash != null, "package-version-or-files-missing");
            Require(package.Name == "RICON PCM proof (TOOL_ONLY)", "wrong-package-name");
            metadata.TryGetValue("Description", out package.Description);
            if (package.Description == null) package.Description = "";
        }
        stream.Position = 0;
        return package;
    }

    private static Dictionary<string, string> Ini(string text, string wantedSection)
    {
        Dictionary<string, string> fields = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        string section = null;
        foreach (string raw in text.Replace("\r", "").Split('\n'))
        {
            string line = raw.Trim();
            if (line.Length == 0 || line.StartsWith(";") || line.StartsWith("#")) continue;
            if (line.StartsWith("[") && line.EndsWith("]")) { section = line.Substring(1, line.Length - 2); continue; }
            if (wantedSection != null && !String.Equals(section, wantedSection, StringComparison.OrdinalIgnoreCase)) continue;
            int equals = line.IndexOf('=');
            Require(equals > 0, "malformed-addon-metadata");
            string key = line.Substring(0, equals).Trim();
            Require(!fields.ContainsKey(key), "duplicate-addon-metadata");
            fields.Add(key, line.Substring(equals + 1).Trim());
        }
        return fields;
    }

    private static void Stage(string profile, string version, string package, string signature)
    {
        string offline = Path.Combine(profile, "ricon-update", "feed", version);
        string localPackage = Path.Combine(offline, "RICON.ts3_plugin");
        string localSignature = Path.Combine(offline, "RICON.sig");
        Stopwatch budget = Stopwatch.StartNew();
        if (File.Exists(localPackage) || File.Exists(localSignature))
        {
            Require(File.Exists(localPackage) && File.Exists(localSignature), "incomplete-offline-feed");
            CopyLocal(localSignature, signature, 8192);
            CopyLocal(localPackage, package, PackageLimit);
            return;
        }
        Uri feed;
        Require(Uri.TryCreate(RiconReleaseConfig.Feed, UriKind.Absolute, out feed)
            && (feed.IsFile || feed.Scheme == Uri.UriSchemeHttps) && feed.UserInfo.Length == 0
            && feed.Query.Length == 0 && feed.Fragment.Length == 0, "invalid-publisher-feed");
        string prefix = feed.AbsoluteUri.TrimEnd('/') + "/";
        Fetch(new Uri(prefix + version + "/RICON.sig"), signature, 8192, budget);
        Fetch(new Uri(prefix + version + "/RICON.ts3_plugin"), package, PackageLimit, budget);
    }

    private static void CopyLocal(string source, string destination, long maximum)
    {
        using (Pins pins = new Pins())
        {
            pins.Directory(Path.GetDirectoryName(FullPath(source)));
            using (FileStream input = LockedRead(source)) Save(input, destination, maximum, null);
        }
    }

    private static void Fetch(Uri address, string destination, long maximum, Stopwatch budget)
    {
        if (address.IsFile) { CopyLocal(address.LocalPath, destination, maximum); return; }
        ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
        for (int redirects = 0; redirects <= 5; redirects++)
        {
            Require(address.Scheme == Uri.UriSchemeHttps && address.UserInfo.Length == 0, "unsafe-feed-redirect");
            Require(budget.ElapsedMilliseconds < DownloadBudgetMs, "feed-timeout");
            HttpWebRequest request = (HttpWebRequest)WebRequest.Create(address);
            request.AllowAutoRedirect = false;
            int remaining = DownloadBudgetMs - (int)budget.ElapsedMilliseconds;
            Require(remaining > 0, "feed-timeout");
            request.Timeout = Math.Min(10000, remaining);
            request.ReadWriteTimeout = request.Timeout;
            request.UseDefaultCredentials = false;
            using (HttpWebResponse response = (HttpWebResponse)request.GetResponse())
            {
                int code = (int)response.StatusCode;
                if (code == 301 || code == 302 || code == 303 || code == 307 || code == 308)
                {
                    string location = response.Headers["Location"];
                    Require(redirects < 5 && !String.IsNullOrEmpty(location), "feed-redirect-limit");
                    address = new Uri(address, location);
                    continue;
                }
                Require(code == 200 && (response.ContentLength < 0 || response.ContentLength <= maximum), "feed-response-invalid");
                using (Stream input = response.GetResponseStream()) Save(input, destination, maximum, budget);
                return;
            }
        }
        throw new UpdateError("feed-redirect-limit");
    }

    private static void Save(Stream input, string destination, long maximum, Stopwatch budget)
    {
        using (FileStream output = new FileStream(destination, FileMode.CreateNew, FileAccess.Write, FileShare.None))
        {
            byte[] bytes = new byte[65536];
            long count = 0;
            for (;;)
            {
                if (budget != null) Require(budget.ElapsedMilliseconds < DownloadBudgetMs, "feed-timeout");
                int read = input.Read(bytes, 0, bytes.Length);
                if (read == 0) break;
                count += read;
                Require(count <= maximum, "feed-file-too-large");
                output.Write(bytes, 0, read);
            }
            Require(count > 0, "empty-feed-file");
            output.Flush(true);
        }
    }

    private static void Authenticate(string version, string hash, string encoded)
    {
        string canonical = encoded.Trim();
        byte[] signature = Convert.FromBase64String(canonical);
        Require(Convert.ToBase64String(signature) == canonical, "invalid-signature-format");
        byte[] statement = Encoding.ASCII.GetBytes("RICON-TS3-PACKAGE-1\n" + version + "\n" + hash + "\n");
        using (RSACryptoServiceProvider rsa = new RSACryptoServiceProvider(new CspParameters(24)))
        {
            rsa.PersistKeyInCsp = false;
            rsa.FromXmlString(RiconReleaseConfig.PublicKey);
            Require(rsa.KeySize >= 2048 && rsa.KeySize <= 8192, "invalid-publisher-key");
            Require(rsa.VerifyData(statement, CryptoConfig.MapNameToOID("SHA256"), signature), "package-signature-invalid");
        }
    }

    private static FileStream ApplyLock(string updateRoot)
    {
        string path = Path.Combine(updateRoot, "apply.lock");
        NoReparseIfExists(path);
        for (;;)
        {
            try { return ExclusiveLock(path); }
            catch (IOException error) { if (!SharingViolation(error)) throw; Thread.Sleep(250); }
        }
    }

    private static void Install(string installer, string package, string profile, string jobDirectory)
    {
        string temp = Path.Combine(jobDirectory, "temp");
        EnsurePrivateDirectory(temp);
        ProcessStartInfo info = new ProcessStartInfo(installer, "-silent " + Quote(package));
        info.UseShellExecute = false;
        info.CreateNoWindow = true;
        info.WorkingDirectory = Path.GetDirectoryName(installer);
        info.EnvironmentVariables["TS3_CONFIG_DIR"] = profile;
        info.EnvironmentVariables["TEMP"] = temp;
        info.EnvironmentVariables["TMP"] = temp;
        using (Process install = Process.Start(info))
        {
            Require(install != null, "installer-start-failed");
            WaitInstaller(install, InstallerBudgetMs);
        }
    }

    private static void WaitInstaller(Process installer, int budgetMs)
    {
        IntPtr ownedHandle = installer.Handle;
        if (installer.WaitForExit(budgetMs))
        {
            Require(installer.ExitCode == 0, "installer-returned-error");
            return;
        }
        // Terminate only the exact kernel process object returned by our own Process.Start.
        TerminateProcess(ownedHandle, 0xE0520001U);
        if (!installer.WaitForExit(5000)) throw new UpdateError("installer-timeout-still-running");
        throw new UpdateError("installer-timeout");
    }

    private static string Quote(string value)
    {
        Require(value.IndexOf('"') < 0 && !value.EndsWith("\\"), "invalid-installer-path");
        return "\"" + value + "\"";
    }

    private static bool SharingViolation(IOException error)
    {
        int code = Marshal.GetHRForException(error) & 65535;
        return code == 32 || code == 33;
    }

    private static FileStream ExclusiveLock(string path)
    {
        string full = FullPath(path);
        NoReparseIfExists(full);
        FileStream stream = new FileStream(full, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
        try
        {
            Require(SamePath(FinalPath(stream.SafeFileHandle), full), "lock-redirected");
            NoReparse(full);
            return stream;
        }
        catch { stream.Dispose(); throw; }
    }

    private static string HashPath(string path) { using (FileStream stream = LockedRead(path)) return Hash(stream); }
    private static string Hash(Stream stream) { return Hash(stream, PackageLimit); }
    private static string Hash(Stream stream, long maximum)
    {
        if (stream.CanSeek) stream.Position = 0;
        using (SHA256 sha = SHA256.Create())
        {
            byte[] bytes = new byte[65536];
            long count = 0;
            int read;
            while ((read = stream.Read(bytes, 0, bytes.Length)) != 0)
            {
                count += read;
                Require(count <= maximum, "hashed-file-too-large");
                sha.TransformBlock(bytes, 0, read, bytes, 0);
            }
            sha.TransformFinalBlock(new byte[0], 0, 0);
            Require(count > 0, "empty-hashed-file");
            if (stream.CanSeek) stream.Position = 0;
            return BitConverter.ToString(sha.Hash).Replace("-", "").ToLowerInvariant();
        }
    }

    private static byte[] ReadBounded(Stream input, int maximum)
    {
        using (MemoryStream bytes = new MemoryStream())
        {
            byte[] buffer = new byte[4096];
            int read;
            while ((read = input.Read(buffer, 0, buffer.Length)) != 0)
            {
                Require(bytes.Length + read <= maximum, "text-file-too-large");
                bytes.Write(buffer, 0, read);
            }
            return bytes.ToArray();
        }
    }

    private static string ReadText(string path, int maximum)
    {
        using (FileStream input = LockedRead(path)) return StrictUtf8.GetString(ReadBounded(input, maximum)).TrimStart('\uFEFF');
    }

    private static FileStream LockedRead(string path)
    {
        string full = FullPath(path);
        NoReparse(full);
        FileStream stream = new FileStream(full, FileMode.Open, FileAccess.Read, FileShare.Read);
        try
        {
            Require(SamePath(FinalPath(stream.SafeFileHandle), full), "file-redirected");
            NoReparse(full);
            return stream;
        }
        catch { stream.Dispose(); throw; }
    }

    private static void EnsurePrivateDirectory(string path)
    {
        string full = FullPath(path);
        if (Directory.Exists(full)) { NoReparse(full); return; }
        DirectorySecurity security = new DirectorySecurity();
        security.SetAccessRuleProtection(true, false);
        IdentityReference user = WindowsIdentity.GetCurrent().User;
        security.AddAccessRule(new FileSystemAccessRule(user, FileSystemRights.FullControl,
            InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit, PropagationFlags.None, AccessControlType.Allow));
        security.AddAccessRule(new FileSystemAccessRule(new SecurityIdentifier(WellKnownSidType.LocalSystemSid, null), FileSystemRights.FullControl,
            InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit, PropagationFlags.None, AccessControlType.Allow));
        security.AddAccessRule(new FileSystemAccessRule(new SecurityIdentifier(WellKnownSidType.BuiltinAdministratorsSid, null), FileSystemRights.FullControl,
            InheritanceFlags.ContainerInherit | InheritanceFlags.ObjectInherit, PropagationFlags.None, AccessControlType.Allow));
        Directory.CreateDirectory(full, security);
        NoReparse(full);
    }

    private static void Publish(string path, string state, string reason)
    {
        Require(state == "STAGED" || state == "APPLIED" || state == "ERROR", "invalid-state");
        NoReparseIfExists(path);
        string temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            using (FileStream file = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None))
            {
                byte[] bytes = Encoding.ASCII.GetBytes(state);
                file.Write(bytes, 0, bytes.Length);
                file.Flush(true);
            }
            for (int attempt = 0; ; attempt++)
            {
                try
                {
                    NoReparseIfExists(path);
                    if (File.Exists(path)) File.Replace(temporary, path, null);
                    else File.Move(temporary, path);
                    break;
                }
                catch (IOException) { if (attempt >= 9) throw; Thread.Sleep(50); }
            }
            string diagnostic = path + ".log";
            NoReparseIfExists(diagnostic);
            File.AppendAllText(diagnostic, DateTime.UtcNow.ToString("o", CultureInfo.InvariantCulture) + " " + state + " " + reason + "\r\n", StrictUtf8);
        }
        finally { if (File.Exists(temporary)) File.Delete(temporary); }
    }

    private static string FullPath(string path)
    {
        Require(!String.IsNullOrEmpty(path) && Path.IsPathRooted(path) && !path.StartsWith(@"\\?\") && !path.StartsWith(@"\\.\"), "path-not-absolute");
        string root = Path.GetPathRoot(path);
        Require(root.Length >= 3, "path-not-absolute");
        string full = Path.GetFullPath(path);
        if (full.Length == Path.GetPathRoot(full).Length) return full;
        return full.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
    }

    private static bool SamePath(string first, string second)
    {
        return String.Equals(FullPath(first), FullPath(second), StringComparison.OrdinalIgnoreCase);
    }

    private static bool Within(string directory, string path)
    {
        return FullPath(path).StartsWith(FullPath(directory) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
    }

    private static void NoReparse(string path)
    {
        string full = FullPath(path);
        string root = Path.GetPathRoot(full);
        string current = root;
        Require((File.GetAttributes(current) & FileAttributes.ReparsePoint) == 0, "reparse-path");
        foreach (string part in full.Substring(root.Length).Split(new char[] { Path.DirectorySeparatorChar }, StringSplitOptions.RemoveEmptyEntries))
        {
            current = Path.Combine(current, part);
            Require((File.GetAttributes(current) & FileAttributes.ReparsePoint) == 0, "reparse-path");
        }
    }

    private static void NoReparseIfExists(string path)
    {
        string full = FullPath(path);
        string root = Path.GetPathRoot(full);
        string current = root;
        Require((File.GetAttributes(current) & FileAttributes.ReparsePoint) == 0, "reparse-path");
        foreach (string part in full.Substring(root.Length).Split(new char[] { Path.DirectorySeparatorChar }, StringSplitOptions.RemoveEmptyEntries))
        {
            current = Path.Combine(current, part);
            FileAttributes attributes;
            try { attributes = File.GetAttributes(current); }
            catch (FileNotFoundException) { return; }
            catch (DirectoryNotFoundException) { return; }
            Require((attributes & FileAttributes.ReparsePoint) == 0, "reparse-path");
        }
    }

    private static string FinalPath(SafeFileHandle handle)
    {
        StringBuilder path = new StringBuilder(32768);
        uint length = GetFinalPathNameByHandle(handle, path, (uint)path.Capacity, 0);
        Require(length > 0 && length < path.Capacity, "handle-path-unavailable");
        string full = path.ToString();
        if (full.StartsWith(@"\\?\UNC\")) return @"\\" + full.Substring(8);
        if (full.StartsWith(@"\\?\")) return full.Substring(4);
        return full;
    }

    private static void CleanupStage(string path)
    {
        try
        {
            string full = FullPath(path);
            string parent = Path.GetDirectoryName(full);
            Require(Path.GetFileName(parent) == "ricon-update" && Path.GetFileName(full).StartsWith("stage-"), "cleanup-scope-invalid");
            NoReparse(full);
            foreach (string item in Directory.GetFileSystemEntries(full, "*", SearchOption.AllDirectories)) NoReparse(item);
            Directory.Delete(full, true);
        }
        catch { }
    }

    private static void Require(bool condition, string reason) { if (!condition) throw new UpdateError(reason); }

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern SafeFileHandle CreateFile(string file, uint access, uint share, IntPtr security, uint mode, uint flags, IntPtr template);
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern uint GetFinalPathNameByHandle(SafeFileHandle file, StringBuilder path, uint size, uint flags);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool GetProcessTimes(IntPtr process, out long creation, out long exit, out long kernel, out long user);
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern bool QueryFullProcessImageName(IntPtr process, uint flags, StringBuilder path, ref int size);
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool TerminateProcess(IntPtr process, uint exitCode);
}
