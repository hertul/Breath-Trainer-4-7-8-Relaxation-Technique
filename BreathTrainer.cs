# BreathTrainer.cs
/**
 * 🌬️ Breath Trainer – 4-7-8 Relaxation Technique (C# Edition)
 * Features: guided breathing, progress bars, stats, custom timing, sound
 * Requires: .NET 6.0+
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading;
using System.Threading.Tasks;

class BreathTrainer
{
    // ─── Colors ────────────────────────────────────────────────────────────

    private static readonly string Reset = "\u001B[0m";
    private static readonly string Bright = "\u001B[1m";
    private static readonly string Dim = "\u001B[2m";
    private static readonly string Red = "\u001B[31m";
    private static readonly string Green = "\u001B[32m";
    private static readonly string Yellow = "\u001B[33m";
    private static readonly string Blue = "\u001B[34m";
    private static readonly string Magenta = "\u001B[35m";
    private static readonly string Cyan = "\u001B[36m";

    private static string C(string text, string color) => color + text + Reset;

    // ─── Sound ─────────────────────────────────────────────────────────────

    private static void Beep()
    {
        try
        {
            Console.Beep(800, 200);
        }
        catch
        {
            Console.Write('\u0007');
        }
    }

    // ─── Data Model ──────────────────────────────────────────────────────

    public class Timing
    {
        [JsonPropertyName("inhale")]
        public int Inhale { get; set; } = 4;
        [JsonPropertyName("hold")]
        public int Hold { get; set; } = 7;
        [JsonPropertyName("exhale")]
        public int Exhale { get; set; } = 8;
    }

    public class Session
    {
        [JsonPropertyName("date")]
        public string Date { get; set; } = "";
        [JsonPropertyName("cycles")]
        public int Cycles { get; set; }
        [JsonPropertyName("durationSeconds")]
        public int DurationSeconds { get; set; }
    }

    public class StatsData
    {
        [JsonPropertyName("totalSessions")]
        public int TotalSessions { get; set; }
        [JsonPropertyName("totalCycles")]
        public int TotalCycles { get; set; }
        [JsonPropertyName("totalTimeSeconds")]
        public int TotalTimeSeconds { get; set; }
        [JsonPropertyName("sessions")]
        public List<Session> Sessions { get; set; } = new();
        [JsonPropertyName("customTiming")]
        public Timing CustomTiming { get; set; } = new();
    }

    // ─── Config ────────────────────────────────────────────────────────────

    private static readonly string DataDir = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
        ".breath_trainer"
    );
    private static readonly string DataFile = Path.Combine(DataDir, "stats.json");

    // ─── Breath Trainer ──────────────────────────────────────────────────

    private readonly StatsData stats = new();
    private int inhale, hold, exhale;
    private bool soundOn = true;

    public BreathTrainer()
    {
        Directory.CreateDirectory(DataDir);
        Load();
        inhale = stats.CustomTiming.Inhale;
        hold = stats.CustomTiming.Hold;
        exhale = stats.CustomTiming.Exhale;
    }

    private void Load()
    {
        if (!File.Exists(DataFile)) return;
        try
        {
            string json = File.ReadAllText(DataFile);
            var loaded = JsonSerializer.Deserialize<StatsData>(json);
            if (loaded != null)
            {
                stats.TotalSessions = loaded.TotalSessions;
                stats.TotalCycles = loaded.TotalCycles;
                stats.TotalTimeSeconds = loaded.TotalTimeSeconds;
                stats.Sessions = loaded.Sessions ?? new List<Session>();
                stats.CustomTiming = loaded.CustomTiming ?? new Timing();
            }
        }
        catch { /* ignore */ }
    }

    private void Save()
    {
        string json = JsonSerializer.Serialize(stats, new JsonSerializerOptions { WriteIndented = true });
        File.WriteAllText(DataFile, json);
    }

    private void SleepMs(int ms)
    {
        Thread.Sleep(ms);
    }

    private void ShowStage(string stage, int duration, string color, string emoji)
    {
        int steps = duration * 2;
        for (int i = 0; i <= steps; i++)
        {
            double pct = (double)i / steps * 100;
            int filled = (int)((double)i / steps * 30);
            string bar = new string('█', filled) + new string('░', 30 - filled);
            Console.Write($"\r  {emoji} {C(stage, color)} {C(bar, color)} {pct,3:F0}%");
            SleepMs(500);
        }
        Console.WriteLine();
    }

    private double BreathingCycle()
    {
        var start = DateTime.Now;
        if (soundOn) Beep();
        ShowStage("Breathe In...", inhale, Blue, "🌬️");
        if (soundOn) Beep();
        ShowStage("Hold...", hold, Yellow, "⏸️");
        if (soundOn) Beep();
        ShowStage("Exhale...", exhale, Green, "🌊");
        return (DateTime.Now - start).TotalSeconds;
    }

    private void RunSession(int cycles)
    {
        Console.WriteLine(C($"\n🌬️ 4-7-8 Breathing Session – {cycles} cycles", Bright + Cyan));
        if (soundOn) Beep();
        Console.WriteLine(C("Get ready...", Dim));
        SleepMs(1000);

        double totalDuration = 0;
        for (int i = 0; i < cycles; i++)
        {
            Console.WriteLine(C($"\nCycle {i+1}/{cycles}", Magenta));
            totalDuration += BreathingCycle();
        }

        if (soundOn) { Beep(); Beep(); Beep(); }
        Console.WriteLine(C("\n✨ Session complete! ✨", Green));
        Console.WriteLine($"  Cycles: {cycles}");
        Console.WriteLine($"  Duration: {totalDuration:F1} seconds");

        stats.TotalSessions++;
        stats.TotalCycles += cycles;
        stats.TotalTimeSeconds += (int)totalDuration;
        stats.Sessions.Add(new Session
        {
            Date = DateTime.Now.ToString("o"),
            Cycles = cycles,
            DurationSeconds = (int)totalDuration
        });
        if (stats.Sessions.Count > 50) stats.Sessions.RemoveAt(0);
        Save();
    }

    private void ShowStats()
    {
        Console.WriteLine("\n📊 STATISTICS");
        Console.WriteLine(C(new string('─', 30), Dim));
        Console.WriteLine($"  Total Sessions: {stats.TotalSessions}");
        Console.WriteLine($"  Total Cycles:   {stats.TotalCycles}");
        int mins = stats.TotalTimeSeconds / 60;
        int secs = stats.TotalTimeSeconds % 60;
        Console.WriteLine($"  Total Time:     {mins}m {secs}s");
        if (stats.Sessions.Count > 0)
        {
            var last = stats.Sessions.Last();
            Console.WriteLine($"  Last Session:   {last.Cycles} cycles ({last.DurationSeconds}s)");
            Console.WriteLine("\n📅 Recent Sessions:");
            var recent = stats.Sessions.Skip(Math.Max(0, stats.Sessions.Count - 5));
            foreach (var s in recent)
            {
                string d = s.Date[..16].Replace('T', ' ');
                Console.WriteLine($"  {d}  {s.Cycles} cycles  {s.DurationSeconds}s");
            }
        }
    }

    private void SetTiming()
    {
        int inh = AskInt($"Inhale duration (default {inhale}): ");
        int h = AskInt($"Hold duration (default {hold}): ");
        int exh = AskInt($"Exhale duration (default {exhale}): ");
        if (inh > 0) inhale = inh;
        if (h > 0) hold = h;
        if (exh > 0) exhale = exh;
        stats.CustomTiming = new Timing { Inhale = inhale, Hold = hold, Exhale = exhale };
        Save();
        Console.WriteLine(C($"✅ Timing set: {inhale}s inhale, {hold}s hold, {exhale}s exhale", Green));
    }

    private void ToggleSound()
    {
        soundOn = !soundOn;
        Console.WriteLine(C($"🔊 Sound {(soundOn ? "on" : "off")}", Cyan));
    }

    private void ResetStats()
    {
        Console.Write("⚠️  Reset all statistics? (yes/no): ");
        string ans = Console.ReadLine()?.Trim() ?? "";
        if (ans.Equals("yes", StringComparison.OrdinalIgnoreCase))
        {
            stats.TotalSessions = 0;
            stats.TotalCycles = 0;
            stats.TotalTimeSeconds = 0;
            stats.Sessions.Clear();
            Save();
            Console.WriteLine(C("🗑️  Statistics reset.", Yellow));
        }
    }

    // ─── Menu ──────────────────────────────────────────────────────────────

    private string Ask(string prompt)
    {
        Console.Write(prompt);
        return Console.ReadLine()?.Trim() ?? "";
    }

    private int AskInt(string prompt)
    {
        while (true)
        {
            string ans = Ask(prompt);
            if (string.IsNullOrEmpty(ans)) return 0;
            if (int.TryParse(ans, out int val)) return val;
            Console.WriteLine(C("❌ Please enter a number.", Red));
        }
    }

    private void ShowMenu()
    {
        Console.WriteLine("\n" + C(new string('═', 50), Cyan));
        Console.WriteLine(C("🌬️ BREATH TRAINER", Bright + Cyan));
        Console.WriteLine(C(new string('═', 50), Cyan));
        Console.WriteLine($"  4-7-8: Inhale {inhale}s, Hold {hold}s, Exhale {exhale}s");
        Console.WriteLine($"  Sound: {(soundOn ? "🔊 On" : "🔇 Off")}");
        Console.WriteLine(C(new string('═', 50), Cyan));
        Console.WriteLine("  1. 🌬️ Start Session (4 cycles)");
        Console.WriteLine("  2. 🌬️ Custom Session (choose cycles)");
        Console.WriteLine("  3. 📊 Statistics");
        Console.WriteLine("  4. ⏱️ Set Timings");
        Console.WriteLine("  5. 🔇 Toggle Sound");
        Console.WriteLine("  6. 🗑️ Reset Statistics");
        Console.WriteLine("  0. 🚪 Exit");
        Console.WriteLine(C(new string('═', 50), Cyan));
    }

    public void Run()
    {
        Console.Clear();
        Console.WriteLine(C("\n🌬️ Breath Trainer – 4-7-8 Relaxation", Bright + Cyan));
        Console.WriteLine(C("Breathe in peace, hold calm, exhale stress.", Dim));

        while (true)
        {
            ShowMenu();
            string choice = Ask("Your choice: ");
            switch (choice)
            {
                case "1":
                    RunSession(4);
                    break;
                case "2":
                    int cycles = AskInt("Number of cycles (default 4): ");
                    if (cycles <= 0) cycles = 4;
                    RunSession(cycles);
                    break;
                case "3":
                    ShowStats();
                    break;
                case "4":
                    SetTiming();
                    break;
                case "5":
                    ToggleSound();
                    break;
                case "6":
                    ResetStats();
                    break;
                case "0":
                    Console.WriteLine(C("👋 Breathe deeply! Goodbye!", Cyan));
                    return;
                default:
                    Console.WriteLine(C("❌ Invalid choice.", Red));
                    break;
            }
            if (choice != "0")
            {
                Console.Write("\nPress Enter to continue...");
                Console.ReadLine();
            }
        }
    }

    public static void Main()
    {
        try
        {
            new BreathTrainer().Run();
        }
        catch (Exception ex)
        {
            Console.WriteLine(C($"❌ Unexpected error: {ex.Message}", Red));
            Environment.Exit(1);
        }
    }
}
