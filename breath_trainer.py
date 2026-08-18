# breath_trainer.py
#!/usr/bin/env python3
"""
🌬️ Breath Trainer – 4-7-8 Relaxation Technique (Python Edition)
Features: guided breathing, visual progress bars, session stats, custom timing, sound alerts
"""

import json
import os
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional

try:
    from rich.console import Console
    from rich.panel import Panel
    from rich.prompt import Prompt, IntPrompt, Confirm
    from rich.progress import Progress, BarColumn, TextColumn
    from rich import box
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False
    print("⚠️  Install 'rich' for enhanced UI: pip install rich")


# ─── Colors ──────────────────────────────────────────────────────────────────

def c(text: str, color: str) -> str:
    colors = {
        "reset": "\033[0m", "bright": "\033[1m", "dim": "\033[2m",
        "red": "\033[31m", "green": "\033[32m", "yellow": "\033[33m",
        "blue": "\033[34m", "magenta": "\033[35m", "cyan": "\033[36m"
    }
    return f"{colors.get(color, '')}{text}{colors['reset']}"


# ─── Sound ──────────────────────────────────────────────────────────────────

def beep(frequency: int = 800, duration: int = 200):
    """Cross‑platform beep."""
    try:
        if sys.platform == "win32":
            import winsound
            winsound.Beep(frequency, duration)
        else:
            # Use terminal bell as fallback
            print('\a', end='', flush=True)
    except Exception:
        pass


# ─── Data Manager ──────────────────────────────────────────────────────────

class StatsManager:
    DATA_DIR = Path.home() / ".breath_trainer"
    DATA_FILE = DATA_DIR / "stats.json"

    def __init__(self):
        self.total_sessions = 0
        self.total_cycles = 0
        self.total_time_seconds = 0
        self.sessions: List[Dict] = []
        self.custom_timing = {"inhale": 4, "hold": 7, "exhale": 8}
        self._load()

    def _load(self):
        if self.DATA_FILE.exists():
            try:
                with open(self.DATA_FILE, 'r') as f:
                    data = json.load(f)
                    self.total_sessions = data.get("total_sessions", 0)
                    self.total_cycles = data.get("total_cycles", 0)
                    self.total_time_seconds = data.get("total_time_seconds", 0)
                    self.sessions = data.get("sessions", [])
                    self.custom_timing = data.get("custom_timing", {"inhale": 4, "hold": 7, "exhale": 8})
            except Exception:
                pass

    def save(self):
        self.DATA_DIR.mkdir(parents=True, exist_ok=True)
        data = {
            "total_sessions": self.total_sessions,
            "total_cycles": self.total_cycles,
            "total_time_seconds": self.total_time_seconds,
            "sessions": self.sessions,
            "custom_timing": self.custom_timing,
        }
        with open(self.DATA_FILE, 'w') as f:
            json.dump(data, f, indent=2)

    def record_session(self, cycles: int, duration: float):
        self.total_sessions += 1
        self.total_cycles += cycles
        self.total_time_seconds += int(duration)
        self.sessions.append({
            "date": datetime.now().isoformat(),
            "cycles": cycles,
            "duration_seconds": int(duration),
        })
        if len(self.sessions) > 50:
            self.sessions = self.sessions[-50:]
        self.save()

    def set_timing(self, inhale: int, hold: int, exhale: int):
        self.custom_timing = {"inhale": inhale, "hold": hold, "exhale": exhale}
        self.save()


# ─── Breath Trainer ──────────────────────────────────────────────────────

class BreathTrainer:
    def __init__(self):
        self.console = Console() if RICH_AVAILABLE else None
        self.stats = StatsManager()
        self.inhale = self.stats.custom_timing["inhale"]
        self.hold = self.stats.custom_timing["hold"]
        self.exhale = self.stats.custom_timing["exhale"]
        self.sound_on = True

    def _show_stage(self, stage: str, duration: float, color: str, emoji: str):
        """Display a breathing stage with a progress bar."""
        if self.console:
            with Progress(
                TextColumn(f"[bold {color}]{emoji} {{task.description}}", justify="center"),
                BarColumn(bar_width=40, style=color, complete_style=color),
                TextColumn("[progress.percentage]{task.percentage:>3.0f}%", style=color),
                transient=True,
            ) as progress:
                task = progress.add_task(f"{stage}  ", total=duration * 10)
                for _ in range(int(duration * 10)):
                    time.sleep(0.1)
                    progress.update(task, advance=1)
        else:
            # Simple ASCII bar without rich
            total_steps = int(duration * 2)
            for i in range(total_steps + 1):
                pct = i / total_steps * 100
                filled = int(i / total_steps * 30)
                bar = "█" * filled + "░" * (30 - filled)
                sys.stdout.write(f"\r  {emoji} {stage} {bar} {pct:3.0f}%")
                sys.stdout.flush()
                time.sleep(0.5)
            print()

    def _breathing_cycle(self) -> float:
        """Run one 4-7-8 cycle and return the duration."""
        start = time.time()
        # Inhale
        beep(600, 100) if self.sound_on else None
        self._show_stage("Breathe In...", self.inhale, "blue", "🌬️")
        # Hold
        beep(800, 100) if self.sound_on else None
        self._show_stage("Hold...", self.hold, "yellow", "⏸️")
        # Exhale
        beep(400, 150) if self.sound_on else None
        self._show_stage("Exhale...", self.exhale, "green", "🌊")
        return time.time() - start

    def run_session(self, cycles: int = 4):
        """Run a full breathing session."""
        if self.console:
            self.console.print(Panel.fit(f"[bold cyan]🌬️ 4-7-8 Breathing Session[/bold cyan]\n{cycles} cycles", border_style="cyan"))
        else:
            print(c(f"\n🌬️ 4-7-8 Breathing Session – {cycles} cycles", "bright"))

        beep(1000, 200) if self.sound_on else None
        print(c("\nGet ready...", "dim"))
        time.sleep(1)

        total_duration = 0
        for i in range(cycles):
            if self.console:
                self.console.print(f"[bold magenta]Cycle {i+1}/{cycles}[/bold magenta]")
            else:
                print(c(f"\nCycle {i+1}/{cycles}", "magenta"))
            total_duration += self._breathing_cycle()

        beep(1200, 300) if self.sound_on else None
        if self.console:
            self.console.print("[bold green]✨ Session complete! ✨[/bold green]")
            self.console.print(f"  Cycles: {cycles}")
            self.console.print(f"  Duration: {total_duration:.1f} seconds")
        else:
            print(c("\n✨ Session complete! ✨", "green"))
            print(f"  Cycles: {cycles}")
            print(f"  Duration: {total_duration:.1f} seconds")

        self.stats.record_session(cycles, total_duration)

    def show_stats(self):
        """Display session statistics."""
        if self.console:
            from rich.table import Table
            table = Table(title="📊 Breathing Statistics", box=box.ROUNDED)
            table.add_column("Metric", style="cyan")
            table.add_column("Value", style="green")
            table.add_row("Total Sessions", str(self.stats.total_sessions))
            table.add_row("Total Cycles", str(self.stats.total_cycles))
            table.add_row("Total Time", f"{self.stats.total_time_seconds // 60}m {self.stats.total_time_seconds % 60}s")
            if self.stats.sessions:
                last = self.stats.sessions[-1]
                table.add_row("Last Session", f"{last['cycles']} cycles ({last['duration_seconds']}s)")
            self.console.print(table)

            if self.stats.sessions:
                hist_table = Table(title="📅 Recent Sessions", box=box.MINIMAL)
                hist_table.add_column("Date", style="dim")
                hist_table.add_column("Cycles", justify="right")
                hist_table.add_column("Duration", justify="right")
                for s in self.stats.sessions[-5:]:
                    date = s["date"][:16].replace("T", " ")
                    hist_table.add_row(date, str(s["cycles"]), f"{s['duration_seconds']}s")
                self.console.print(hist_table)
        else:
            print("\n📊 STATISTICS")
            print(c("─"*30, "dim"))
            print(f"  Total Sessions: {self.stats.total_sessions}")
            print(f"  Total Cycles:   {self.stats.total_cycles}")
            print(f"  Total Time:     {self.stats.total_time_seconds // 60}m {self.stats.total_time_seconds % 60}s")
            if self.stats.sessions:
                last = self.stats.sessions[-1]
                print(f"  Last Session:   {last['cycles']} cycles ({last['duration_seconds']}s)")
                print("\n📅 Recent Sessions:")
                for s in self.stats.sessions[-5:]:
                    date = s["date"][:16].replace("T", " ")
                    print(f"  {date}  {s['cycles']} cycles  {s['duration_seconds']}s")

    def set_timing(self):
        if self.console:
            inhale = IntPrompt.ask("Inhale duration (seconds)", default=self.inhale)
            hold = IntPrompt.ask("Hold duration (seconds)", default=self.hold)
            exhale = IntPrompt.ask("Exhale duration (seconds)", default=self.exhale)
        else:
            try:
                inhale = int(input(f"Inhale duration (default {self.inhale}): ") or self.inhale)
                hold = int(input(f"Hold duration (default {self.hold}): ") or self.hold)
                exhale = int(input(f"Exhale duration (default {self.exhale}): ") or self.exhale)
            except ValueError:
                print(c("Invalid input. Keeping current values.", "yellow"))
                return
        self.inhale = inhale
        self.hold = hold
        self.exhale = exhale
        self.stats.set_timing(inhale, hold, exhale)
        print(c(f"✅ Timing set: {inhale}s inhale, {hold}s hold, {exhale}s exhale", "green"))

    def toggle_sound(self):
        self.sound_on = not self.sound_on
        state = "on" if self.sound_on else "off"
        print(c(f"🔊 Sound {state}", "cyan"))

    def reset_stats(self):
        if self.console:
            if not Confirm.ask("⚠️  Reset all statistics?"):
                return
        else:
            if input("⚠️  Reset all statistics? (yes/no): ").strip().lower() != "yes":
                return
        self.stats.total_sessions = 0
        self.stats.total_cycles = 0
        self.stats.total_time_seconds = 0
        self.stats.sessions = []
        self.stats.save()
        print(c("🗑️  Statistics reset.", "yellow"))

    # ─── Menu ──────────────────────────────────────────────────────────────

    def _show_menu(self):
        if self.console:
            menu = f"""
[bold cyan]🌬️ Breath Trainer[/bold cyan]
  4-7-8 Technique: Inhale {self.inhale}s, Hold {self.hold}s, Exhale {self.exhale}s
  Sound: {"🔊 On" if self.sound_on else "🔇 Off"}

  [1] 🌬️ Start Session (4 cycles)
  [2] 🌬️ Custom Session (choose cycles)
  [3] 📊 Statistics
  [4] ⏱️ Set Timings
  [5] 🔇 Toggle Sound
  [6] 🗑️ Reset Statistics
  [0] 🚪 Exit
"""
            self.console.print(Panel(menu, border_style="blue"))
        else:
            print("\n" + "-"*50)
            print(f"🌬️ 4-7-8: Inhale {self.inhale}s, Hold {self.hold}s, Exhale {self.exhale}s")
            print(f"   Sound: {'🔊 On' if self.sound_on else '🔇 Off'}")
            print("-"*50)
            print("  1. 🌬️ Start Session (4 cycles)")
            print("  2. 🌬️ Custom Session (choose cycles)")
            print("  3. 📊 Statistics")
            print("  4. ⏱️ Set Timings")
            print("  5. 🔇 Toggle Sound")
            print("  6. 🗑️ Reset Statistics")
            print("  0. 🚪 Exit")
            print("-"*50)

    def _get_choice(self) -> str:
        if self.console:
            return Prompt.ask("Your choice", choices=["0","1","2","3","4","5","6"])
        return input("Your choice: ").strip()

    def run(self):
        if self.console:
            self.console.print(Panel.fit("[bold cyan]🌬️ Breath Trainer – 4-7-8 Relaxation[/bold cyan]", border_style="cyan"))
        else:
            print(c("\n🌬️ Breath Trainer – 4-7-8 Relaxation", "bright"))
            print(c("Breathe in peace, hold calm, exhale stress.", "dim"))

        while True:
            self._show_menu()
            choice = self._get_choice()
            if choice == "1":
                self.run_session(4)
            elif choice == "2":
                if self.console:
                    cycles = IntPrompt.ask("Number of cycles", default=4)
                else:
                    try:
                        cycles = int(input("Number of cycles (default 4): ") or 4)
                    except ValueError:
                        cycles = 4
                self.run_session(max(1, cycles))
            elif choice == "3":
                self.show_stats()
            elif choice == "4":
                self.set_timing()
            elif choice == "5":
                self.toggle_sound()
            elif choice == "6":
                self.reset_stats()
            elif choice == "0":
                print(c("👋 Breathe deeply! Goodbye!", "cyan"))
                break
            else:
                print(c("❌ Invalid choice.", "red"))

            if choice != "0":
                if self.console:
                    self.console.print("\n[dim]Press Enter to continue...[/dim]")
                    input()
                else:
                    input("\nPress Enter to continue...")


def main():
    try:
        app = BreathTrainer()
        app.run()
    except KeyboardInterrupt:
        print("\n👋 Breathe deeply!")
        sys.exit(0)
    except Exception as e:
        print(c(f"❌ Unexpected error: {e}", "red"))
        sys.exit(1)

if __name__ == "__main__":
    main()
