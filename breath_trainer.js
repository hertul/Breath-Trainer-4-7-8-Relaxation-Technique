# breath_trainer.js
/**
 * 🌬️ Breath Trainer – 4-7-8 Relaxation Technique (Node.js Edition)
 * Features: guided breathing, visual progress bars, session stats, custom timing, sound alerts
 */

const fs = require('fs');
const path = require('path');
const os = require('os');
const readline = require('readline');

// ─── Colors ──────────────────────────────────────────────────────────────────

const colors = {
    reset: '\x1b[0m',
    bright: '\x1b[1m',
    dim: '\x1b[2m',
    red: '\x1b[31m',
    green: '\x1b[32m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    magenta: '\x1b[35m',
    cyan: '\x1b[36m',
};

const c = (str, color) => `${color}${str}${colors.reset}`;

// ─── Sound ──────────────────────────────────────────────────────────────────

function beep() {
    // Use system bell
    process.stdout.write('\x07');
}

// ─── Config ──────────────────────────────────────────────────────────────────

const CONFIG = {
    dataDir: path.join(os.homedir(), '.breath_trainer'),
    dataFile: 'stats.json',
    defaultInhale: 4,
    defaultHold: 7,
    defaultExhale: 8,
};

// ─── Stats Manager ──────────────────────────────────────────────────────────

class StatsManager {
    constructor() {
        this.dataDir = CONFIG.dataDir;
        this.dataFile = path.join(this.dataDir, CONFIG.dataFile);
        this.totalSessions = 0;
        this.totalCycles = 0;
        this.totalTimeSeconds = 0;
        this.sessions = [];
        this.customTiming = { inhale: CONFIG.defaultInhale, hold: CONFIG.defaultHold, exhale: CONFIG.defaultExhale };
        if (!fs.existsSync(this.dataDir)) fs.mkdirSync(this.dataDir, { recursive: true });
        this._load();
    }

    _load() {
        if (fs.existsSync(this.dataFile)) {
            try {
                const data = JSON.parse(fs.readFileSync(this.dataFile, 'utf8'));
                this.totalSessions = data.totalSessions || 0;
                this.totalCycles = data.totalCycles || 0;
                this.totalTimeSeconds = data.totalTimeSeconds || 0;
                this.sessions = data.sessions || [];
                this.customTiming = data.customTiming || { inhale: CONFIG.defaultInhale, hold: CONFIG.defaultHold, exhale: CONFIG.defaultExhale };
            } catch (_) {}
        }
    }

    save() {
        fs.writeFileSync(this.dataFile, JSON.stringify({
            totalSessions: this.totalSessions,
            totalCycles: this.totalCycles,
            totalTimeSeconds: this.totalTimeSeconds,
            sessions: this.sessions,
            customTiming: this.customTiming
        }, null, 2));
    }

    recordSession(cycles, duration) {
        this.totalSessions++;
        this.totalCycles += cycles;
        this.totalTimeSeconds += Math.floor(duration);
        this.sessions.push({
            date: new Date().toISOString(),
            cycles,
            durationSeconds: Math.floor(duration)
        });
        if (this.sessions.length > 50) this.sessions = this.sessions.slice(-50);
        this.save();
    }

    setTiming(inhale, hold, exhale) {
        this.customTiming = { inhale, hold, exhale };
        this.save();
    }
}

// ─── Breath Trainer ──────────────────────────────────────────────────────

class BreathTrainer {
    constructor() {
        this.rl = readline.createInterface({ input: process.stdin, output: process.stdout });
        this.stats = new StatsManager();
        this.inhale = this.stats.customTiming.inhale;
        this.hold = this.stats.customTiming.hold;
        this.exhale = this.stats.customTiming.exhale;
        this.soundOn = true;
    }

    _sleep(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }

    _showStage(stage, duration, color, emoji) {
        return new Promise(async (resolve) => {
            const steps = Math.floor(duration * 2);
            for (let i = 0; i <= steps; i++) {
                const pct = (i / steps * 100);
                const filled = Math.floor(i / steps * 30);
                const bar = '█'.repeat(filled) + '░'.repeat(30 - filled);
                process.stdout.write(`\r  ${emoji} ${c(stage, color)} ${c(bar, color)} ${Math.floor(pct)}%`);
                await this._sleep(500);
            }
            console.log();
            resolve();
        });
    }

    async _breathingCycle() {
        const start = Date.now();
        if (this.soundOn) beep();
        await this._showStage('Breathe In...', this.inhale, 'blue', '🌬️');
        if (this.soundOn) beep();
        await this._showStage('Hold...', this.hold, 'yellow', '⏸️');
        if (this.soundOn) beep();
        await this._showStage('Exhale...', this.exhale, 'green', '🌊');
        return (Date.now() - start) / 1000;
    }

    async runSession(cycles = 4) {
        console.log(c(`\n🌬️ 4-7-8 Breathing Session – ${cycles} cycles`, colors.bright + colors.cyan));
        if (this.soundOn) beep();
        console.log(c('Get ready...', colors.dim));
        await this._sleep(1000);

        let totalDuration = 0;
        for (let i = 0; i < cycles; i++) {
            console.log(c(`\nCycle ${i+1}/${cycles}`, colors.magenta));
            totalDuration += await this._breathingCycle();
        }

        if (this.soundOn) { beep(); beep(); beep(); }
        console.log(c('\n✨ Session complete! ✨', colors.green));
        console.log(`  Cycles: ${cycles}`);
        console.log(`  Duration: ${totalDuration.toFixed(1)} seconds`);
        this.stats.recordSession(cycles, totalDuration);
    }

    showStats() {
        console.log('\n📊 STATISTICS');
        console.log(c('─'.repeat(30), colors.dim));
        console.log(`  Total Sessions: ${this.stats.totalSessions}`);
        console.log(`  Total Cycles:   ${this.stats.totalCycles}`);
        const mins = Math.floor(this.stats.totalTimeSeconds / 60);
        const secs = this.stats.totalTimeSeconds % 60;
        console.log(`  Total Time:     ${mins}m ${secs}s`);
        if (this.stats.sessions.length) {
            const last = this.stats.sessions[this.stats.sessions.length-1];
            console.log(`  Last Session:   ${last.cycles} cycles (${last.durationSeconds}s)`);
            console.log('\n📅 Recent Sessions:');
            this.stats.sessions.slice(-5).forEach(s => {
                const d = s.date.slice(0,16).replace('T', ' ');
                console.log(`  ${d}  ${s.cycles} cycles  ${s.durationSeconds}s`);
            });
        }
    }

    async setTiming() {
        const inhale = await this._askInt(`Inhale duration (default ${this.inhale}): `) || this.inhale;
        const hold = await this._askInt(`Hold duration (default ${this.hold}): `) || this.hold;
        const exhale = await this._askInt(`Exhale duration (default ${this.exhale}): `) || this.exhale;
        this.inhale = inhale;
        this.hold = hold;
        this.exhale = exhale;
        this.stats.setTiming(inhale, hold, exhale);
        console.log(c(`✅ Timing set: ${inhale}s inhale, ${hold}s hold, ${exhale}s exhale`, colors.green));
    }

    toggleSound() {
        this.soundOn = !this.soundOn;
        console.log(c(`🔊 Sound ${this.soundOn ? 'on' : 'off'}`, colors.cyan));
    }

    async resetStats() {
        const ans = await this._ask('⚠️  Reset all statistics? (yes/no): ');
        if (ans.toLowerCase() === 'yes') {
            this.stats.totalSessions = 0;
            this.stats.totalCycles = 0;
            this.stats.totalTimeSeconds = 0;
            this.stats.sessions = [];
            this.stats.save();
            console.log(c('🗑️  Statistics reset.', colors.yellow));
        }
    }

    // ─── Helpers ──────────────────────────────────────────────────────────

    _ask(prompt) {
        return new Promise(resolve => this.rl.question(prompt, resolve));
    }

    async _askInt(prompt) {
        const ans = await this._ask(prompt);
        const num = parseInt(ans.trim());
        return isNaN(num) ? null : num;
    }

    async _showMenu() {
        console.log('\n' + c('═'.repeat(50), colors.cyan));
        console.log(c('🌬️ BREATH TRAINER', colors.bright + colors.cyan));
        console.log(c('═'.repeat(50), colors.cyan));
        console.log(`  4-7-8: Inhale ${this.inhale}s, Hold ${this.hold}s, Exhale ${this.exhale}s`);
        console.log(`  Sound: ${this.soundOn ? '🔊 On' : '🔇 Off'}`);
        console.log(c('═'.repeat(50), colors.cyan));
        console.log('  1. 🌬️ Start Session (4 cycles)');
        console.log('  2. 🌬️ Custom Session (choose cycles)');
        console.log('  3. 📊 Statistics');
        console.log('  4. ⏱️ Set Timings');
        console.log('  5. 🔇 Toggle Sound');
        console.log('  6. 🗑️ Reset Statistics');
        console.log('  0. 🚪 Exit');
        console.log(c('═'.repeat(50), colors.cyan));
    }

    async run() {
        console.clear();
        console.log(c('\n🌬️ Breath Trainer – 4-7-8 Relaxation', colors.bright + colors.cyan));
        console.log(c('Breathe in peace, hold calm, exhale stress.', colors.dim));

        while (true) {
            await this._showMenu();
            const choice = await this._ask('Your choice: ');
            switch (choice.trim()) {
                case '1':
                    await this.runSession(4);
                    break;
                case '2': {
                    const cycles = await this._askInt('Number of cycles (default 4): ') || 4;
                    await this.runSession(Math.max(1, cycles));
                    break;
                }
                case '3':
                    this.showStats();
                    break;
                case '4':
                    await this.setTiming();
                    break;
                case '5':
                    this.toggleSound();
                    break;
                case '6':
                    await this.resetStats();
                    break;
                case '0':
                    console.log(c('👋 Breathe deeply! Goodbye!', colors.cyan));
                    this.rl.close();
                    return;
                default:
                    console.log(c('❌ Invalid choice.', colors.red));
            }
            if (choice !== '0') {
                console.log('\nPress Enter to continue...');
                await this._ask('');
            }
        }
    }
}

// ─── Main ────────────────────────────────────────────────────────────────────

const main = async () => {
    try {
        const app = new BreathTrainer();
        await app.run();
    } catch (e) {
        console.error(c(`❌ Unexpected error: ${e.message}`, colors.red));
        process.exit(1);
    }
};

main();
