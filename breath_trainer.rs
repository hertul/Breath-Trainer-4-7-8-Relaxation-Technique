# breath_trainer.rs
/**
 * 🌬️ Breath Trainer – 4-7-8 Relaxation Technique (Rust Edition)
 * Features: guided breathing, progress bars, stats, custom timing, sound
 * Dependencies: serde, serde_json, chrono, colored, crossterm
 */

use chrono::Utc;
use colored::*;
use crossterm::terminal;
use serde::{Deserialize, Serialize};
use std::fs;
use std::io::{self, Write, BufRead};
use std::path::PathBuf;
use std::thread;
use std::time::Duration;

// ─── Types ──────────────────────────────────────────────────────────────────

#[derive(Debug, Serialize, Deserialize, Clone)]
struct Timing {
    inhale: u32,
    hold: u32,
    exhale: u32,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
struct Session {
    date: String,
    cycles: u32,
    duration_seconds: u32,
}

#[derive(Debug, Serialize, Deserialize)]
struct Stats {
    total_sessions: u32,
    total_cycles: u32,
    total_time_seconds: u32,
    sessions: Vec<Session>,
    custom_timing: Timing,
}

// ─── Colors ──────────────────────────────────────────────────────────────────

fn c(text: &str, color: &str) -> String {
    match color {
        "green" => text.green().to_string(),
        "red" => text.red().to_string(),
        "yellow" => text.yellow().to_string(),
        "cyan" => text.cyan().to_string(),
        "magenta" => text.magenta().to_string(),
        "blue" => text.blue().to_string(),
        "bright" => text.bright().to_string(),
        "dim" => text.dimmed().to_string(),
        _ => text.to_string(),
    }
}

// ─── Sound ──────────────────────────────────────────────────────────────────

fn beep() {
    print!("\x07");
    io::stdout().flush().unwrap();
}

// ─── Config ──────────────────────────────────────────────────────────────────

const DEFAULT_INHALE: u32 = 4;
const DEFAULT_HOLD: u32 = 7;
const DEFAULT_EXHALE: u32 = 8;

// ─── Stats Manager ──────────────────────────────────────────────────────────

struct StatsManager {
    file_path: PathBuf,
    pub total_sessions: u32,
    pub total_cycles: u32,
    pub total_time_seconds: u32,
    pub sessions: Vec<Session>,
    pub custom_timing: Timing,
}

impl StatsManager {
    fn new() -> Self {
        let home = std::env::var("HOME").or_else(|_| std::env::var("USERPROFILE")).unwrap_or_else(|_| ".".to_string());
        let dir = PathBuf::from(home).join(".breath_trainer");
        fs::create_dir_all(&dir).unwrap();
        let file_path = dir.join("stats.json");
        let mut s = StatsManager {
            file_path,
            total_sessions: 0,
            total_cycles: 0,
            total_time_seconds: 0,
            sessions: Vec::new(),
            custom_timing: Timing { inhale: DEFAULT_INHALE, hold: DEFAULT_HOLD, exhale: DEFAULT_EXHALE },
        };
        s.load();
        s
    }

    fn load(&mut self) {
        if let Ok(raw) = fs::read_to_string(&self.file_path) {
            if let Ok(data) = serde_json::from_str::<Stats>(&raw) {
                self.total_sessions = data.total_sessions;
                self.total_cycles = data.total_cycles;
                self.total_time_seconds = data.total_time_seconds;
                self.sessions = data.sessions;
                self.custom_timing = data.custom_timing;
                if self.custom_timing.inhale == 0 {
                    self.custom_timing = Timing { inhale: DEFAULT_INHALE, hold: DEFAULT_HOLD, exhale: DEFAULT_EXHALE };
                }
                return;
            }
        }
        self.custom_timing = Timing { inhale: DEFAULT_INHALE, hold: DEFAULT_HOLD, exhale: DEFAULT_EXHALE };
    }

    fn save(&self) {
        let data = Stats {
            total_sessions: self.total_sessions,
            total_cycles: self.total_cycles,
            total_time_seconds: self.total_time_seconds,
            sessions: self.sessions.clone(),
            custom_timing: self.custom_timing.clone(),
        };
        let raw = serde_json::to_string_pretty(&data).unwrap();
        let _ = fs::write(&self.file_path, raw);
    }

    fn record_session(&mut self, cycles: u32, duration: f64) {
        self.total_sessions += 1;
        self.total_cycles += cycles;
        self.total_time_seconds += duration as u32;
        self.sessions.push(Session {
            date: Utc::now().to_rfc3339(),
            cycles,
            duration_seconds: duration as u32,
        });
        if self.sessions.len() > 50 {
            self.sessions.drain(0..self.sessions.len()-50);
        }
        self.save();
    }

    fn set_timing(&mut self, inhale: u32, hold: u32, exhale: u32) {
        self.custom_timing = Timing { inhale, hold, exhale };
        self.save();
    }
}

// ─── Breath Trainer ──────────────────────────────────────────────────────

struct BreathTrainer {
    stats: StatsManager,
    inhale: u32,
    hold: u32,
    exhale: u32,
    sound_on: bool,
}

impl BreathTrainer {
    fn new() -> Self {
        let stats = StatsManager::new();
        let inhale = stats.custom_timing.inhale;
        let hold = stats.custom_timing.hold;
        let exhale = stats.custom_timing.exhale;
        BreathTrainer { stats, inhale, hold, exhale, sound_on: true }
    }

    fn sleep_ms(&self, ms: u64) {
        thread::sleep(Duration::from_millis(ms));
    }

    fn show_stage(&self, stage: &str, duration: u32, color: &str, emoji: &str) {
        let steps = duration * 2;
        for i in 0..=steps {
            let pct = i as f64 / steps as f64 * 100.0;
            let filled = (i as f64 / steps as f64 * 30.0) as usize;
            let bar = "█".repeat(filled) + &"░".repeat(30 - filled);
            print!("\r  {} {} {} {:3.0}%", emoji, c(stage, color), c(&bar, color), pct);
            io::stdout().flush().unwrap();
            self.sleep_ms(500);
        }
        println!();
    }

    fn breathing_cycle(&self) -> f64 {
        let start = std::time::Instant::now();
        if self.sound_on { beep(); }
        self.show_stage("Breathe In...", self.inhale, "blue", "🌬️");
        if self.sound_on { beep(); }
        self.show_stage("Hold...", self.hold, "yellow", "⏸️");
        if self.sound_on { beep(); }
        self.show_stage("Exhale...", self.exhale, "green", "🌊");
        start.elapsed().as_secs_f64()
    }

    fn run_session(&mut self, cycles: u32) {
        println!("{}", c(&format!("\n🌬️ 4-7-8 Breathing Session – {} cycles", cycles), "bright cyan"));
        if self.sound_on { beep(); }
        println!("{}", c("Get ready...", "dim"));
        self.sleep_ms(1000);

        let mut total_duration = 0.0;
        for i in 0..cycles {
            println!("{}", c(&format!("\nCycle {}/{}", i+1, cycles), "magenta"));
            total_duration += self.breathing_cycle();
        }

        if self.sound_on { beep(); beep(); beep(); }
        println!("{}", c("\n✨ Session complete! ✨", "green"));
        println!("  Cycles: {}", cycles);
        println!("  Duration: {:.1} seconds", total_duration);
        self.stats.record_session(cycles, total_duration);
    }

    fn show_stats(&self) {
        println!("\n📊 STATISTICS");
        println!("{}", "─".repeat(30).dimmed());
        println!("  Total Sessions: {}", self.stats.total_sessions);
        println!("  Total Cycles:   {}", self.stats.total_cycles);
        let mins = self.stats.total_time_seconds / 60;
        let secs = self.stats.total_time_seconds % 60;
        println!("  Total Time:     {}m {}s", mins, secs);
        if !self.stats.sessions.is_empty() {
            let last = &self.stats.sessions[self.stats.sessions.len()-1];
            println!("  Last Session:   {} cycles ({}s)", last.cycles, last.duration_seconds);
            println!("\n📅 Recent Sessions:");
            let start = if self.stats.sessions.len() > 5 { self.stats.sessions.len() - 5 } else { 0 };
            for s in &self.stats.sessions[start..] {
                let d = s.date[..16].replace("T", " ");
                println!("  {}  {} cycles  {}s", d, s.cycles, s.duration_seconds);
            }
        }
    }

    fn set_timing(&mut self) {
        let inhale = self.ask_u32(&format!("Inhale duration (default {}): ", self.inhale));
        let hold = self.ask_u32(&format!("Hold duration (default {}): ", self.hold));
        let exhale = self.ask_u32(&format!("Exhale duration (default {}): ", self.exhale));
        if inhale > 0 { self.inhale = inhale; }
        if hold > 0 { self.hold = hold; }
        if exhale > 0 { self.exhale = exhale; }
        self.stats.set_timing(self.inhale, self.hold, self.exhale);
        println!("{}", c(&format!("✅ Timing set: {}s inhale, {}s hold, {}s exhale", self.inhale, self.hold, self.exhale), "green"));
    }

    fn toggle_sound(&mut self) {
        self.sound_on = !self.sound_on;
        let state = if self.sound_on { "on" } else { "off" };
        println!("{}", c(&format!("🔊 Sound {}", state), "cyan"));
    }

    fn reset_stats(&mut self) {
        print!("⚠️  Reset all statistics? (yes/no): ");
        io::stdout().flush().unwrap();
        let mut ans = String::new();
        io::stdin().read_line(&mut ans).unwrap();
        if ans.trim().to_lowercase() == "yes" {
            self.stats.total_sessions = 0;
            self.stats.total_cycles = 0;
            self.stats.total_time_seconds = 0;
            self.stats.sessions = Vec::new();
            self.stats.save();
            println!("{}", c("🗑️  Statistics reset.", "yellow"));
        }
    }

    // ─── Helpers ──────────────────────────────────────────────────────────

    fn ask(&self, prompt: &str) -> String {
        print!("{}", prompt);
        io::stdout().flush().unwrap();
        let mut line = String::new();
        io::stdin().read_line(&mut line).unwrap();
        line.trim().to_string()
    }

    fn ask_u32(&self, prompt: &str) -> u32 {
        loop {
            let ans = self.ask(prompt);
            if ans.is_empty() { return 0; }
            if let Ok(val) = ans.parse::<u32>() {
                return val;
            }
            println!("{}", c("❌ Please enter a number.", "red"));
        }
    }

    fn show_menu(&self) {
        let state = if self.sound_on { "🔊 On" } else { "🔇 Off" };
        println!("\n{}", "═".repeat(50).cyan());
        println!("{}", c("🌬️ BREATH TRAINER", "bright cyan"));
        println!("{}", "═".repeat(50).cyan());
        println!("  4-7-8: Inhale {}s, Hold {}s, Exhale {}s", self.inhale, self.hold, self.exhale);
        println!("  Sound: {}", state);
        println!("{}", "═".repeat(50).cyan());
        println!("  1. 🌬️ Start Session (4 cycles)");
        println!("  2. 🌬️ Custom Session (choose cycles)");
        println!("  3. 📊 Statistics");
        println!("  4. ⏱️ Set Timings");
        println!("  5. 🔇 Toggle Sound");
        println!("  6. 🗑️ Reset Statistics");
        println!("  0. 🚪 Exit");
        println!("{}", "═".repeat(50).cyan());
    }

    fn run(&mut self) {
        println!("{}", "\n🌬️ Breath Trainer – 4-7-8 Relaxation".bright().cyan());
        println!("{}", "Breathe in peace, hold calm, exhale stress.".dimmed());

        loop {
            self.show_menu();
            let choice = self.ask("Your choice: ");
            match choice.as_str() {
                "1" => self.run_session(4),
                "2" => {
                    let cycles = self.ask_u32("Number of cycles (default 4): ");
                    let cycles = if cycles == 0 { 4 } else { cycles };
                    self.run_session(cycles);
                }
                "3" => self.show_stats(),
                "4" => self.set_timing(),
                "5" => self.toggle_sound(),
                "6" => self.reset_stats(),
                "0" => {
                    println!("{}", c("👋 Breathe deeply! Goodbye!", "cyan"));
                    return;
                }
                _ => println!("{}", c("❌ Invalid choice.", "red")),
            }
            if choice != "0" {
                print!("\nPress Enter to continue...");
                io::stdout().flush().unwrap();
                let mut _dummy = String::new();
                io::stdin().read_line(&mut _dummy).unwrap();
            }
        }
    }
}

// ─── Main ────────────────────────────────────────────────────────────────────

fn main() {
    let mut app = BreathTrainer::new();
    app.run();
}
