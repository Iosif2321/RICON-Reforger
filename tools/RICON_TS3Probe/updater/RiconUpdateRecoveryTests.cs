using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;

// Compile with RiconUpdate.cs, publisher Config.cs and /main:RiconUpdateRecoveryTests.
internal static class RiconUpdateRecoveryTests
{
    private static readonly Type Updater = typeof(RiconUpdate);
    private static int checks;
    private const BindingFlags StaticPrivate = BindingFlags.Static | BindingFlags.NonPublic;
    private const BindingFlags InstanceFields = BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public;
    private const string Prefix = "RICON%20PCM%20proof%20%28TOOL_ONLY%29";
    private const string Helper = "ricon_pcm_probe/RiconUpdate.exe";

    private sealed class Fixture : IDisposable
    {
        internal string Root, Profile, Stage, Archive;
        internal string Other = "keep";
        internal byte[] OldDll = Encoding.ASCII.GetBytes("previous DLL");
        internal byte[] OldHelper = Encoding.ASCII.GetBytes("previous updater");
        internal byte[] NewDll = Encoding.ASCII.GetBytes("authenticated target DLL");
        internal byte[] NewHelper = Encoding.ASCII.GetBytes("authenticated target updater");
        internal object Before, Target;
        internal bool HadHelper;

        internal Fixture(bool helper)
        {
            Root = Path.Combine(Path.GetTempPath(), "ricon-repair-" + Guid.NewGuid().ToString("N"));
            Profile = Path.Combine(Root, "profile");
            Stage = Path.Combine(Profile, "ricon-update", "stage-test");
            Directory.CreateDirectory(Stage);
            Directory.CreateDirectory(Path.Combine(Profile, "plugins"));
            HadHelper = helper;
            Write("ricon_pcm_probe.dll", OldDll);
            if (helper) Write(Helper, OldHelper);
            Metadata("0.6.2", "old", helper);
            Before = Call("ReadInstalled", Profile, Digest(OldDll));
            Target = Activator.CreateInstance(Updater.GetNestedType("Package", BindingFlags.NonPublic), true);
            Set(Target, "Name", "RICON PCM proof (TOOL_ONLY)");
            Set(Target, "Version", "0.6.3");
            Set(Target, "Description", "new");
            Set(Target, "PluginHash", Digest(NewDll));
            Set(Target, "UpdaterHash", Digest(NewHelper));
            Archive = Path.Combine(Stage, "authenticated-target.ts3_plugin");
            File.WriteAllText(Archive, "signature checked by Main before transaction");
        }

        internal void Write(string relative, byte[] bytes)
        {
            string path = Path.Combine(Profile, "plugins", relative);
            Directory.CreateDirectory(Path.GetDirectoryName(path));
            File.WriteAllBytes(path, bytes);
        }

        internal void Metadata(string version, string description, bool helper)
        {
            string files = helper ? Helper + ", ricon_pcm_probe.dll" : "ricon_pcm_probe.dll";
            File.WriteAllText(Path.Combine(Profile, "addons.ini"),
                "[plugin]\n" + Prefix + "\\author=RICON\n" + Prefix + "\\version=" + version + "\n"
                + Prefix + "\\description=" + description + "\n" + Prefix + "\\files=" + files
                + "\nOtherAddon\\version=9.0.0\nOtherAddon\\files=other.dll\n[unrelated]\nvalue=" + Other + "\n");
        }

        internal void InstallTarget()
        {
            Write("ricon_pcm_probe.dll", NewDll);
            Write(Helper, NewHelper);
            Metadata("0.6.3", "new", true);
        }

        internal void Restore(string archive, bool removeExtra)
        {
            string metadata = null;
            bool helper = false;
            using (ZipArchive zip = ZipFile.OpenRead(archive))
            {
                foreach (ZipArchiveEntry entry in zip.Entries)
                {
                    Assert(entry.FullName != "addons.ini", "rollback never contains global addons.ini");
                    if (entry.FullName.EndsWith("/")) continue;
                    using (Stream stream = entry.Open())
                    using (MemoryStream bytes = new MemoryStream())
                    {
                        stream.CopyTo(bytes);
                        if (entry.FullName == "package.ini") metadata = Encoding.UTF8.GetString(bytes.ToArray());
                        else
                        {
                            Assert(entry.FullName == "plugins/ricon_pcm_probe.dll" || entry.FullName == "plugins/" + Helper,
                                "rollback contains only the two owned paths");
                            string relative = entry.FullName.Substring("plugins/".Length);
                            Write(relative, bytes.ToArray());
                            if (relative == Helper) helper = true;
                        }
                    }
                }
            }
            Assert(metadata != null, "rollback includes own metadata");
            if (!helper && removeExtra) File.Delete(Path.Combine(Profile, "plugins", Helper));
            string version = Regex.Match(metadata, "(?m)^Version = (.+)$").Groups[1].Value.Trim();
            string description = Regex.Match(metadata, "(?m)^Description = (.*)$").Groups[1].Value.Trim();
            Metadata(version, description, helper);
        }

        internal string Apply(Action<string> installer)
        {
            return (string)Call("ApplyRelease", Profile, Archive, Stage, Before, Target, installer);
        }

        internal string Recovery(Exception error)
        {
            return (string)error.GetType().GetField("RecoveryPath", InstanceFields).GetValue(error);
        }

        public void Dispose()
        {
            string full = Path.GetFullPath(Root);
            string temp = Path.GetFullPath(Path.GetTempPath()).TrimEnd(Path.DirectorySeparatorChar);
            if (!String.Equals(Path.GetDirectoryName(full), temp, StringComparison.OrdinalIgnoreCase) ||
                !Regex.IsMatch(Path.GetFileName(full), @"\Aricon-repair-[0-9a-f]{32}\z")) throw new IOException("Invalid fixture root");
            if (Directory.Exists(full)) Directory.Delete(full, true);
        }
    }

    private static int Main(string[] args)
    {
        if (args.Length == 1 && args[0] == "--owned-wait") { Thread.Sleep(30000); return 0; }
        try
        {
            using (Fixture f = new Fixture(true))
            {
                Assert(f.Apply(delegate(string path) { f.InstallTarget(); }) == "verified-installed-output", "successful target");
            }
            using (Fixture f = new Fixture(true))
            {
                string result = f.Apply(delegate(string path) { f.InstallTarget(); throw new IOException(); });
                Assert(result.StartsWith("verified-target-vendor-warning-"), "complete target survives vendor failure");
            }
            using (Fixture f = new Fixture(true))
            {
                int attempts = 0;
                f.Other = "manual unrelated setting";
                Exception error = Fails(delegate
                {
                    f.Apply(delegate(string path)
                    {
                        if (++attempts == 1) { f.Write("ricon_pcm_probe.dll", f.NewDll); throw new IOException(); }
                        f.Restore(path, true);
                    });
                });
                Assert(attempts == 2 && error.Message.StartsWith("apply-failed-restored-"), "partial DLL repaired once");
                Assert(File.ReadAllText(Path.Combine(f.Profile, "addons.ini")).Contains("value=manual unrelated setting"),
                    "repair preserves unrelated metadata changes");
            }
            using (Fixture f = new Fixture(true))
            {
                int attempts = 0;
                Exception error = Fails(delegate
                {
                    f.Apply(delegate(string path)
                    {
                        if (++attempts == 1) { f.Write("ricon_pcm_probe.dll", f.NewDll); return; }
                        f.Restore(path, true);
                    });
                });
                Assert(attempts == 2 && error.Message == "apply-failed-restored-installed-output-mismatch",
                    "zero vendor exit still verifies output and repairs partial result");
            }
            using (Fixture f = new Fixture(true))
            {
                int attempts = 0;
                Exception error = Fails(delegate
                {
                    f.Apply(delegate(string path)
                    {
                        if (++attempts == 1)
                        {
                            f.Metadata("0.6.3", "new", true);
                            File.Delete(Path.Combine(f.Profile, "plugins", Helper));
                            throw new IOException();
                        }
                        f.Restore(path, true);
                    });
                });
                Assert(attempts == 2 && error.Message.StartsWith("apply-failed-restored-"), "metadata-first missing helper repaired");
            }
            using (Fixture f = new Fixture(true))
            {
                int attempts = 0;
                Exception error = Fails(delegate
                {
                    f.Apply(delegate(string path)
                    {
                        attempts++;
                        f.Metadata("0.7.0", "manual newer", true);
                        f.Write("ricon_pcm_probe.dll", Encoding.ASCII.GetBytes("foreign newer DLL"));
                        throw new IOException();
                    });
                });
                Assert(attempts == 1 && File.Exists(f.Recovery(error)), "foreign newer state retains recovery without overwrite");
                Assert(File.ReadAllText(Path.Combine(f.Profile, "addons.ini")).Contains("\\version=0.7.0"), "newer version preserved");
            }
            using (Fixture f = new Fixture(true))
            {
                int attempts = 0;
                Exception error = Fails(delegate
                {
                    f.Apply(delegate(string path)
                    {
                        if (++attempts == 1) f.Write("ricon_pcm_probe.dll", f.NewDll);
                        throw new IOException();
                    });
                });
                Assert(attempts == 2 && File.Exists(f.Recovery(error)), "failed single repair retains rollback");
            }
            using (Fixture f = new Fixture(false))
            {
                int attempts = 0;
                Exception error = Fails(delegate
                {
                    f.Apply(delegate(string path)
                    {
                        if (++attempts == 1) { f.Write(Helper, f.NewHelper); throw new IOException(); }
                        f.Restore(path, false);
                    });
                });
                Assert(attempts == 2 && File.Exists(f.Recovery(error)), "leftover new helper prevents false restored result");
            }
            using (Fixture f = new Fixture(true))
            {
                int attempts = 0;
                Exception error = Fails(delegate { f.Apply(delegate(string path) { attempts++; throw new IOException(); }); });
                Assert(attempts == 1 && error.Message.StartsWith("apply-failed-unchanged-"), "unchanged state needs no repair");
            }
            using (Fixture f = new Fixture(true))
            {
                Exception error = Fails(delegate
                {
                    f.Apply(delegate(string path)
                    {
                        f.Write("ricon_pcm_probe.dll", f.NewDll);
                        File.AppendAllText(Path.Combine(f.Profile, "addons.ini"), "\n[plugin]\nForeign\\files=" + Helper + "\n");
                        throw new IOException();
                    });
                });
                Assert(error.Message.StartsWith("recovery-blocked-") && File.Exists(f.Recovery(error)), "foreign helper ownership prevents rollback");
            }
            using (Fixture f = new Fixture(true))
            {
                object parsed = Call("ReadInstalled", f.Profile, Digest(f.OldDll));
                Set(parsed, "Files", "ricon_pcm_probe.dll, unrelated-user-file.txt");
                Fails(delegate { Call("CreateRollback", f.Profile, f.Stage, parsed); });
                Assert(!File.Exists(Path.Combine(f.Stage, "RICON-rollback.ts3_plugin")), "unbacked old asset refused before apply");
            }
            ProcessStartInfo info = new ProcessStartInfo(Assembly.GetExecutingAssembly().Location, "--owned-wait");
            info.UseShellExecute = false;
            info.CreateNoWindow = true;
            using (Process owned = Process.Start(info))
            {
                Exception timeout = Fails(delegate { Call("WaitInstaller", owned, 50); });
                Assert(timeout.Message == "installer-timeout" && owned.HasExited, "bounded wait terminates only own test process");
            }
            Console.WriteLine("RICON_UPDATE_RECOVERY_PASS checks=" + checks);
            return 0;
        }
        catch (Exception error) { Console.Error.WriteLine(error); return 1; }
    }

    private static object Call(string name, params object[] args)
    {
        try { return Updater.GetMethod(name, StaticPrivate).Invoke(null, args); }
        catch (TargetInvocationException error) { throw error.InnerException; }
    }

    private static void Set(object instance, string field, object value)
    {
        instance.GetType().GetField(field, InstanceFields).SetValue(instance, value);
    }

    private static string Digest(byte[] bytes)
    {
        using (SHA256 sha = SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant();
    }

    private static Exception Fails(Action action)
    {
        try { action(); }
        catch (Exception error) { checks++; return error; }
        throw new Exception("Expected failure");
    }

    private static void Assert(bool condition, string message)
    {
        checks++;
        if (!condition) throw new Exception(message);
    }
}
