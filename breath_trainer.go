# breath_trainer.go
/**
 * 🌬️ Breath Trainer – 4-7-8 Relaxation Technique (Go Edition)
 * Features: guided breathing, progress bars, stats, custom timing, sound
 */

package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"
)

// ─── Types ──────────────────────────────────────────────────────────────────

type Timing struct {
	Inhale int `json:"inhale"`
	Hold   int `json:"hold"`
	Exhale int `json:"exhale"`
}

type Session struct {
	Date            string `json:"date"`
	Cycles          int    `json:"cycles"`
	DurationSeconds int    `json:"durationSeconds"`
}

type Stats struct {
	TotalSessions   int       `json:"totalSessions"`
	TotalCycles     int       `json:"totalCycles"`
	TotalTimeSeconds int      `json:"totalTimeSeconds"`
	Sessions        []Session `json:"sessions"`
	CustomTiming    Timing    `json:"customTiming"`
}

// ─── Colors ──────────────────────────────────────────────────────────────────

const (
	reset  = "\x1b[0m"
	bright = "\x1b[1m"
	dim    = "\x1b[2m"
	red    = "\x1b[31m"
	green  = "\x1b[32m"
	yellow = "\x1b[33m"
	blue   = "\x1b[34m"
	magenta = "\x1b[35m"
	cyan   = "\x1b[36m"
)

func c(str, color string) string {
	return color + str + reset
}

// ─── Sound ──────────────────────────────────────────────────────────────────

func beep() {
	fmt.Print("\x07")
}

// ─── Config ──────────────────────────────────────────────────────────────────

const (
	defaultInhale = 4
	defaultHold   = 7
	defaultExhale = 8
)

// ─── Stats Manager ──────────────────────────────────────────────────────────

type StatsManager struct {
	filePath        string
	TotalSessions   int
	TotalCycles     int
	TotalTimeSeconds int
	Sessions        []Session
	CustomTiming    Timing
}

func NewStatsManager() *StatsManager {
	home, _ := os.UserHomeDir()
	dir := filepath.Join(home, ".breath_trainer")
	os.MkdirAll(dir, 0755)
	filePath := filepath.Join(dir, "stats.json")
	s := &StatsManager{filePath: filePath}
	s.load()
	return s
}

func (s *StatsManager) load() {
	if _, err := os.Stat(s.filePath); os.IsNotExist(err) {
		s.CustomTiming = Timing{Inhale: defaultInhale, Hold: defaultHold, Exhale: defaultExhale}
		return
	}
	raw, err := os.ReadFile(s.filePath)
	if err != nil {
		s.CustomTiming = Timing{Inhale: defaultInhale, Hold: defaultHold, Exhale: defaultExhale}
		return
	}
	var data Stats
	if err := json.Unmarshal(raw, &data); err != nil {
		s.CustomTiming = Timing{Inhale: defaultInhale, Hold: defaultHold, Exhale: defaultExhale}
		return
	}
	s.TotalSessions = data.TotalSessions
	s.TotalCycles = data.TotalCycles
	s.TotalTimeSeconds = data.TotalTimeSeconds
	s.Sessions = data.Sessions
	s.CustomTiming = data.CustomTiming
	if s.CustomTiming.Inhale == 0 {
		s.CustomTiming = Timing{Inhale: defaultInhale, Hold: defaultHold, Exhale: defaultExhale}
	}
}

func (s *StatsManager) save() {
	data := Stats{
		TotalSessions:    s.TotalSessions,
		TotalCycles:      s.TotalCycles,
		TotalTimeSeconds: s.TotalTimeSeconds,
		Sessions:         s.Sessions,
		CustomTiming:     s.CustomTiming,
	}
	raw, _ := json.MarshalIndent(data, "", "  ")
	os.WriteFile(s.filePath, raw, 0644)
}

func (s *StatsManager) RecordSession(cycles int, duration float64) {
	s.TotalSessions++
	s.TotalCycles += cycles
	s.TotalTimeSeconds += int(duration)
	s.Sessions = append(s.Sessions, Session{
		Date:            time.Now().Format(time.RFC3339),
		Cycles:          cycles,
		DurationSeconds: int(duration),
	})
	if len(s.Sessions) > 50 {
		s.Sessions = s.Sessions[len(s.Sessions)-50:]
	}
	s.save()
}

func (s *StatsManager) SetTiming(inhale, hold, exhale int) {
	s.CustomTiming = Timing{Inhale: inhale, Hold: hold, Exhale: exhale}
	s.save()
}

// ─── Breath Trainer ──────────────────────────────────────────────────────

type BreathTrainer struct {
	reader   *bufio.Reader
	stats    *StatsManager
	inhale   int
	hold     int
	exhale   int
	soundOn  bool
}

func NewBreathTrainer() *BreathTrainer {
	s := NewStatsManager()
	return &BreathTrainer{
		reader:  bufio.NewReader(os.Stdin),
		stats:   s,
		inhale:  s.CustomTiming.Inhale,
		hold:    s.CustomTiming.Hold,
		exhale:  s.CustomTiming.Exhale,
		soundOn: true,
	}
}

func (b *BreathTrainer) sleep(ms int) {
	time.Sleep(time.Duration(ms) * time.Millisecond)
}

func (b *BreathTrainer) showStage(stage string, duration int, color, emoji string) {
	steps := duration * 2
	for i := 0; i <= steps; i++ {
		pct := float64(i) / float64(steps) * 100
		filled := int(float64(i) / float64(steps) * 30)
		bar := strings.Repeat("█", filled) + strings.Repeat("░", 30-filled)
		fmt.Printf("\r  %s %s %s %3.0f%%", emoji, c(stage, color), c(bar, color), pct)
		b.sleep(500)
	}
	fmt.Println()
}

func (b *BreathTrainer) breathingCycle() float64 {
	start := time.Now()
	if b.soundOn {
		beep()
	}
	b.showStage("Breathe In...", b.inhale, blue, "🌬️")
	if b.soundOn {
		beep()
	}
	b.showStage("Hold...", b.hold, yellow, "⏸️")
	if b.soundOn {
		beep()
	}
	b.showStage("Exhale...", b.exhale, green, "🌊")
	return time.Since(start).Seconds()
}

func (b *BreathTrainer) RunSession(cycles int) {
	fmt.Printf("%s\n", c(fmt.Sprintf("\n🌬️ 4-7-8 Breathing Session – %d cycles", cycles), bright+cyan))
	if b.soundOn {
		beep()
	}
	fmt.Println(c("Get ready...", dim))
	b.sleep(1000)

	var totalDuration float64
	for i := 0; i < cycles; i++ {
		fmt.Printf("%s\n", c(fmt.Sprintf("\nCycle %d/%d", i+1, cycles), magenta))
		totalDuration += b.breathingCycle()
	}

	if b.soundOn {
		beep()
		beep()
		beep()
	}
	fmt.Printf("%s\n", c("\n✨ Session complete! ✨", green))
	fmt.Printf("  Cycles: %d\n", cycles)
	fmt.Printf("  Duration: %.1f seconds\n", totalDuration)
	b.stats.RecordSession(cycles, totalDuration)
}

func (b *BreathTrainer) ShowStats() {
	fmt.Println("\n📊 STATISTICS")
	fmt.Println(c(strings.Repeat("─", 30), dim))
	fmt.Printf("  Total Sessions: %d\n", b.stats.TotalSessions)
	fmt.Printf("  Total Cycles:   %d\n", b.stats.TotalCycles)
	mins := b.stats.TotalTimeSeconds / 60
	secs := b.stats.TotalTimeSeconds % 60
	fmt.Printf("  Total Time:     %dm %ds\n", mins, secs)
	if len(b.stats.Sessions) > 0 {
		last := b.stats.Sessions[len(b.stats.Sessions)-1]
		fmt.Printf("  Last Session:   %d cycles (%ds)\n", last.Cycles, last.DurationSeconds)
		fmt.Println("\n📅 Recent Sessions:")
		start := 0
		if len(b.stats.Sessions) > 5 {
			start = len(b.stats.Sessions) - 5
		}
		for _, s := range b.stats.Sessions[start:] {
			d := s.Date[:16]
			d = strings.Replace(d, "T", " ", 1)
			fmt.Printf("  %s  %d cycles  %ds\n", d, s.Cycles, s.DurationSeconds)
		}
	}
}

func (b *BreathTrainer) SetTiming() {
	inhale := b.askInt(fmt.Sprintf("Inhale duration (default %d): ", b.inhale))
	hold := b.askInt(fmt.Sprintf("Hold duration (default %d): ", b.hold))
	exhale := b.askInt(fmt.Sprintf("Exhale duration (default %d): ", b.exhale))
	if inhale > 0 {
		b.inhale = inhale
	}
	if hold > 0 {
		b.hold = hold
	}
	if exhale > 0 {
		b.exhale = exhale
	}
	b.stats.SetTiming(b.inhale, b.hold, b.exhale)
	fmt.Printf("%s\n", c(fmt.Sprintf("✅ Timing set: %ds inhale, %ds hold, %ds exhale", b.inhale, b.hold, b.exhale), green))
}

func (b *BreathTrainer) ToggleSound() {
	b.soundOn = !b.soundOn
	state := "on"
	if !b.soundOn {
		state = "off"
	}
	fmt.Printf("%s\n", c(fmt.Sprintf("🔊 Sound %s", state), cyan))
}

func (b *BreathTrainer) ResetStats() {
	fmt.Print("⚠️  Reset all statistics? (yes/no): ")
	ans, _ := b.reader.ReadString('\n')
	ans = strings.TrimSpace(strings.ToLower(ans))
	if ans == "yes" {
		b.stats.TotalSessions = 0
		b.stats.TotalCycles = 0
		b.stats.TotalTimeSeconds = 0
		b.stats.Sessions = []Session{}
		b.stats.save()
		fmt.Printf("%s\n", c("🗑️  Statistics reset.", yellow))
	}
}

// ─── Helpers ──────────────────────────────────────────────────────────────

func (b *BreathTrainer) ask(prompt string) string {
	fmt.Print(prompt)
	line, _ := b.reader.ReadString('\n')
	return strings.TrimSpace(line)
}

func (b *BreathTrainer) askInt(prompt string) int {
	for {
		ans := b.ask(prompt)
		if ans == "" {
			return 0
		}
		if val, err := strconv.Atoi(ans); err == nil {
			return val
		}
		fmt.Println(c("❌ Please enter a number.", red))
	}
}

func (b *BreathTrainer) showMenu() {
	fmt.Println("\n" + c(strings.Repeat("═", 50), cyan))
	fmt.Println(c("🌬️ BREATH TRAINER", bright+cyan))
	fmt.Println(c(strings.Repeat("═", 50), cyan))
	fmt.Printf("  4-7-8: Inhale %ds, Hold %ds, Exhale %ds\n", b.inhale, b.hold, b.exhale)
	state := "🔊 On"
	if !b.soundOn {
		state = "🔇 Off"
	}
	fmt.Printf("  Sound: %s\n", state)
	fmt.Println(c(strings.Repeat("═", 50), cyan))
	fmt.Println("  1. 🌬️ Start Session (4 cycles)")
	fmt.Println("  2. 🌬️ Custom Session (choose cycles)")
	fmt.Println("  3. 📊 Statistics")
	fmt.Println("  4. ⏱️ Set Timings")
	fmt.Println("  5. 🔇 Toggle Sound")
	fmt.Println("  6. 🗑️ Reset Statistics")
	fmt.Println("  0. 🚪 Exit")
	fmt.Println(c(strings.Repeat("═", 50), cyan))
}

func (b *BreathTrainer) run() {
	fmt.Print("\033[H\033[2J")
	fmt.Printf("%s\n", c("\n🌬️ Breath Trainer – 4-7-8 Relaxation", bright+cyan))
	fmt.Printf("%s\n", c("Breathe in peace, hold calm, exhale stress.", dim))

	for {
		b.showMenu()
		choice := b.ask("Your choice: ")
		switch choice {
		case "1":
			b.RunSession(4)
		case "2":
			cycles := b.askInt("Number of cycles (default 4): ")
			if cycles <= 0 {
				cycles = 4
			}
			b.RunSession(cycles)
		case "3":
			b.ShowStats()
		case "4":
			b.SetTiming()
		case "5":
			b.ToggleSound()
		case "6":
			b.ResetStats()
		case "0":
			fmt.Printf("%s\n", c("👋 Breathe deeply! Goodbye!", cyan))
			return
		default:
			fmt.Println(c("❌ Invalid choice.", red))
		}
		if choice != "0" {
			fmt.Print("\nPress Enter to continue...")
			b.reader.ReadString('\n')
		}
	}
}

func main() {
	app := NewBreathTrainer()
	app.run()
}
