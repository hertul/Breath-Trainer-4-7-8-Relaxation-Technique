# BreathTrainer.java
/**
 * 🌬️ Breath Trainer – 4-7-8 Relaxation Technique (Java Edition)
 * Features: guided breathing, progress bars, stats, custom timing, sound
 * Requires: Java 17+
 */

import java.io.*;
import java.nio.file.*;
import java.time.*;
import java.time.format.*;
import java.util.*;
import java.util.concurrent.*;

public class BreathTrainer {
    // ─── Colors ────────────────────────────────────────────────────────────

    private static final String RESET = "\u001B[0m";
    private static final String BRIGHT = "\u001B[1m";
    private static final String DIM = "\u001B[2m";
    private static final String RED = "\u001B[31m";
    private static final String GREEN = "\u001B[32m";
    private static final String YELLOW = "\u001B[33m";
    private static final String BLUE = "\u001B[34m";
    private static final String MAGENTA = "\u001B[35m";
    private static final String CYAN = "\u001B[36m";

    private static String c(String text, String color) { return color + text + RESET; }

    // ─── Sound ─────────────────────────────────────────────────────────────

    private static void beep() {
        try {
            java.awt.Toolkit.getDefaultToolkit().beep();
        } catch (Exception e) {
            // Fallback: terminal bell
            System.out.print('\u0007');
            System.out.flush();
        }
    }

    // ─── Data Model ──────────────────────────────────────────────────────

    private static class Timing {
        int inhale, hold, exhale;
        Timing(int inhale, int hold, int exhale) {
            this.inhale = inhale;
            this.hold = hold;
            this.exhale = exhale;
        }
    }

    private static class Session {
        String date;
        int cycles;
        int durationSeconds;
        Session(String date, int cycles, int durationSeconds) {
            this.date = date;
            this.cycles = cycles;
            this.durationSeconds = durationSeconds;
        }
    }

    private static class Stats {
        int totalSessions = 0;
        int totalCycles = 0;
        int totalTimeSeconds = 0;
        List<Session> sessions = new ArrayList<>();
        Timing timing = new Timing(4, 7, 8);
    }

    // ─── Config ────────────────────────────────────────────────────────────

    private static final String DATA_DIR = System.getProperty("user.home") + "/.breath_trainer";
    private static final String DATA_FILE = DATA_DIR + "/stats.json";

    // ─── Breath Trainer ──────────────────────────────────────────────────

    private final Scanner scanner;
    private Stats stats;
    private int inhale, hold, exhale;
    private boolean soundOn = true;

    public BreathTrainer() throws IOException {
        scanner = new Scanner(System.in);
        Files.createDirectories(Paths.get(DATA_DIR));
        stats = new Stats();
        load();
        inhale = stats.timing.inhale;
        hold = stats.timing.hold;
        exhale = stats.timing.exhale;
    }

    private void load() {
        Path path = Paths.get(DATA_FILE);
        if (!Files.exists(path)) return;
        try {
            String json = Files.readString(path);
            // Simple manual parse
            stats.totalSessions = extractInt(json, "totalSessions");
            stats.totalCycles = extractInt(json, "totalCycles");
            stats.totalTimeSeconds = extractInt(json, "totalTimeSeconds");
            int i = extractInt(json, "inhale");
            if (i > 0) stats.timing.inhale = i;
            int h = extractInt(json, "hold");
            if (h > 0) stats.timing.hold = h;
            int e = extractInt(json, "exhale");
            if (e > 0) stats.timing.exhale = e;
        } catch (Exception e) { /* ignore */ }
    }

    private int extractInt(String json, String key) {
        String pattern = "\"" + key + "\"\\s*:\\s*(\\d+)";
        var m = java.util.regex.Pattern.compile(pattern).matcher(json);
        return m.find() ? Integer.parseInt(m.group(1)) : 0;
    }

    private void save() {
        try {
            StringBuilder sb = new StringBuilder();
            sb.append("{\n");
            sb.append("  \"totalSessions\": ").append(stats.totalSessions).append(",\n");
            sb.append("  \"totalCycles\": ").append(stats.totalCycles).append(",\n");
            sb.append("  \"totalTimeSeconds\": ").append(stats.totalTimeSeconds).append(",\n");
            sb.append("  \"sessions\": [\n");
            for (int i = 0; i < stats.sessions.size(); i++) {
                Session s = stats.sessions.get(i);
                sb.append("    { \"date\": \"").append(escapeJson(s.date)).append("\", \"cycles\": ").append(s.cycles).append(", \"durationSeconds\": ").append(s.durationSeconds).append(" }");
                if (i < stats.sessions.size() - 1) sb.append(",");
                sb.append("\n");
            }
            sb.append("  ],\n");
            sb.append("  \"customTiming\": { \"inhale\": ").append(stats.timing.inhale).append(", \"hold\": ").append(stats.timing.hold).append(", \"exhale\": ").append(stats.timing.exhale).append(" }\n");
            sb.append("}");
            Files.writeString(Paths.get(DATA_FILE), sb.toString());
        } catch (IOException e) { e.printStackTrace(); }
    }

    private String escapeJson(String s) {
        return s.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    private void sleepMs(int ms) {
        try { Thread.sleep(ms); } catch (InterruptedException e) { Thread.currentThread().interrupt(); }
    }

    private void showStage(String stage, int duration, String color, String emoji) {
        int steps = duration * 2;
        for (int i = 0; i <= steps; i++) {
            double pct = (double) i / steps * 100;
            int filled = (int) ((double) i / steps * 30);
            String bar = "█".repeat(filled) + "░".repeat(30 - filled);
            System.out.printf("\r  %s %s %s %3.0f%%", emoji, c(stage, color), c(bar, color), pct);
            System.out.flush();
            sleepMs(500);
        }
        System.out.println();
    }

    private double breathingCycle() {
        long start = System.currentTimeMillis();
        if (soundOn) beep();
        showStage("Breathe In...", inhale, BLUE, "🌬️");
        if (soundOn) beep();
        showStage("Hold...", hold, YELLOW, "⏸️");
        if (soundOn) beep();
        showStage("Exhale...", exhale, GREEN, "🌊");
        return (System.currentTimeMillis() - start) / 1000.0;
    }

    private void runSession(int cycles) {
        System.out.println(c("\n🌬️ 4-7-8 Breathing Session – " + cycles + " cycles", BRIGHT + CYAN));
        if (soundOn) beep();
        System.out.println(c("Get ready...", DIM));
        sleepMs(1000);

        double totalDuration = 0;
        for (int i = 0; i < cycles; i++) {
            System.out.println(c("\nCycle " + (i+1) + "/" + cycles, MAGENTA));
            totalDuration += breathingCycle();
        }

        if (soundOn) { beep(); beep(); beep(); }
        System.out.println(c("\n✨ Session complete! ✨", GREEN));
        System.out.println("  Cycles: " + cycles);
        System.out.printf("  Duration: %.1f seconds\n", totalDuration);

        stats.totalSessions++;
        stats.totalCycles += cycles;
        stats.totalTimeSeconds += (int) totalDuration;
        stats.sessions.add(new Session(
            Instant.now().toString(),
            cycles,
            (int) totalDuration
        ));
        if (stats.sessions.size() > 50) stats.sessions.remove(0);
        save();
    }

    private void showStats() {
        System.out.println("\n📊 STATISTICS");
        System.out.println(c("─".repeat(30), DIM));
        System.out.println("  Total Sessions: " + stats.totalSessions);
        System.out.println("  Total Cycles:   " + stats.totalCycles);
        int mins = stats.totalTimeSeconds / 60;
        int secs = stats.totalTimeSeconds % 60;
        System.out.println("  Total Time:     " + mins + "m " + secs + "s");
        if (!stats.sessions.isEmpty()) {
            Session last = stats.sessions.get(stats.sessions.size() - 1);
            System.out.println("  Last Session:   " + last.cycles + " cycles (" + last.durationSeconds + "s)");
            System.out.println("\n📅 Recent Sessions:");
            int start = Math.max(0, stats.sessions.size() - 5);
            for (int i = start; i < stats.sessions.size(); i++) {
                Session s = stats.sessions.get(i);
                String d = s.date.substring(0, 16).replace('T', ' ');
                System.out.println("  " + d + "  " + s.cycles + " cycles  " + s.durationSeconds + "s");
            }
        }
    }

    private void setTiming() {
        int inh = askInt("Inhale duration (default " + inhale + "): ");
        int h = askInt("Hold duration (default " + hold + "): ");
        int exh = askInt("Exhale duration (default " + exhale + "): ");
        if (inh > 0) inhale = inh;
        if (h > 0) hold = h;
        if (exh > 0) exhale = exh;
        stats.timing = new Timing(inhale, hold, exhale);
        save();
        System.out.println(c("✅ Timing set: " + inhale + "s inhale, " + hold + "s hold, " + exhale + "s exhale", GREEN));
    }

    private void toggleSound() {
        soundOn = !soundOn;
        System.out.println(c("🔊 Sound " + (soundOn ? "on" : "off"), CYAN));
    }

    private void resetStats() {
        System.out.print("⚠️  Reset all statistics? (yes/no): ");
        String ans = scanner.nextLine().trim();
        if (ans.equalsIgnoreCase("yes")) {
            stats.totalSessions = 0;
            stats.totalCycles = 0;
            stats.totalTimeSeconds = 0;
            stats.sessions.clear();
            save();
            System.out.println(c("🗑️  Statistics reset.", YELLOW));
        }
    }

    // ─── Menu ──────────────────────────────────────────────────────────────

    private String ask(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine().trim();
    }

    private int askInt(String prompt) {
        while (true) {
            try {
                String ans = ask(prompt);
                if (ans.isEmpty()) return 0;
                return Integer.parseInt(ans);
            } catch (NumberFormatException e) {
                System.out.println(c("❌ Please enter a number.", RED));
            }
        }
    }

    private void showMenu() {
        System.out.println("\n" + c("═".repeat(50), CYAN));
        System.out.println(c("🌬️ BREATH TRAINER", BRIGHT + CYAN));
        System.out.println(c("═".repeat(50), CYAN));
        System.out.println("  4-7-8: Inhale " + inhale + "s, Hold " + hold + "s, Exhale " + exhale + "s");
        System.out.println("  Sound: " + (soundOn ? "🔊 On" : "🔇 Off"));
        System.out.println(c("═".repeat(50), CYAN));
        System.out.println("  1. 🌬️ Start Session (4 cycles)");
        System.out.println("  2. 🌬️ Custom Session (choose cycles)");
        System.out.println("  3. 📊 Statistics");
        System.out.println("  4. ⏱️ Set Timings");
        System.out.println("  5. 🔇 Toggle Sound");
        System.out.println("  6. 🗑️ Reset Statistics");
        System.out.println("  0. 🚪 Exit");
        System.out.println(c("═".repeat(50), CYAN));
    }

    public void run() {
        System.out.print("\033[H\033[2J");
        System.out.flush();
        System.out.println(c("\n🌬️ Breath Trainer – 4-7-8 Relaxation", BRIGHT + CYAN));
        System.out.println(c("Breathe in peace, hold calm, exhale stress.", DIM));

        while (true) {
            showMenu();
            String choice = ask("Your choice: ");
            switch (choice) {
                case "1": runSession(4); break;
                case "2": {
                    int cycles = askInt("Number of cycles (default 4): ");
                    if (cycles <= 0) cycles = 4;
                    runSession(cycles);
                    break;
                }
                case "3": showStats(); break;
                case "4": setTiming(); break;
                case "5": toggleSound(); break;
                case "6": resetStats(); break;
                case "0":
                    System.out.println(c("👋 Breathe deeply! Goodbye!", CYAN));
                    return;
                default:
                    System.out.println(c("❌ Invalid choice.", RED));
            }
            if (!choice.equals("0")) {
                System.out.print("\nPress Enter to continue...");
                scanner.nextLine();
            }
        }
    }

    public static void main(String[] args) {
        try {
            new BreathTrainer().run();
        } catch (Exception e) {
            System.err.println(c("❌ Unexpected error: " + e.getMessage(), RED));
            e.printStackTrace();
            System.exit(1);
        }
    }
}
