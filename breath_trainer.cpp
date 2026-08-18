# breath_trainer.cpp
/**
 * 🌬️ Breath Trainer – 4-7-8 Relaxation Technique (C++ Edition)
 * Features: guided breathing, progress bars, stats, custom timing, sound
 * Uses only STL, no external libraries.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <thread>
#include <chrono>
#include <cctype>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#endif

// ─── Colors ──────────────────────────────────────────────────────────────────

#ifdef _WIN32
HANDLE hConsole;
void setColor(int color) { SetConsoleTextAttribute(hConsole, color); }
#define RESET_COLOR setColor(7)
#define COLOR_RED setColor(12)
#define COLOR_GREEN setColor(10)
#define COLOR_YELLOW setColor(14)
#define COLOR_BLUE setColor(9)
#define COLOR_MAGENTA setColor(13)
#define COLOR_CYAN setColor(11)
#define COLOR_BRIGHT setColor(15)
#define COLOR_DIM setColor(8)
#else
#define RESET_COLOR std::cout << "\x1b[0m"
#define COLOR_RED std::cout << "\x1b[31m"
#define COLOR_GREEN std::cout << "\x1b[32m"
#define COLOR_YELLOW std::cout << "\x1b[33m"
#define COLOR_BLUE std::cout << "\x1b[34m"
#define COLOR_MAGENTA std::cout << "\x1b[35m"
#define COLOR_CYAN std::cout << "\x1b[36m"
#define COLOR_BRIGHT std::cout << "\x1b[1m"
#define COLOR_DIM std::cout << "\x1b[2m"
#endif

#define C(str, color) color << str << RESET_COLOR

// ─── Sound ──────────────────────────────────────────────────────────────────

void beep() {
#ifdef _WIN32
    Beep(800, 200);
#else
    std::cout << '\a' << std::flush;
#endif
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string get_timestamp() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

std::string get_home_dir() {
#ifdef _WIN32
    const char* h = std::getenv("USERPROFILE");
#else
    const char* h = std::getenv("HOME");
#endif
    return h ? std::string(h) : ".";
}

// ─── Data Model ─────────────────────────────────────────────────────────────

struct Timing {
    int inhale = 4;
    int hold = 7;
    int exhale = 8;
};

struct Session {
    std::string date;
    int cycles;
    int durationSeconds;
};

struct Stats {
    int totalSessions = 0;
    int totalCycles = 0;
    int totalTimeSeconds = 0;
    std::vector<Session> sessions;
    Timing customTiming;
};

// ─── JSON (simplified) ─────────────────────────────────────────────────────

std::string escape_json(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

std::string serialize_stats(const Stats& stats) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"totalSessions\": " << stats.totalSessions << ",\n";
    json << "  \"totalCycles\": " << stats.totalCycles << ",\n";
    json << "  \"totalTimeSeconds\": " << stats.totalTimeSeconds << ",\n";
    json << "  \"sessions\": [\n";
    for (size_t i = 0; i < stats.sessions.size(); ++i) {
        const auto& s = stats.sessions[i];
        json << "    { \"date\": \"" << escape_json(s.date) << "\", \"cycles\": " << s.cycles << ", \"durationSeconds\": " << s.durationSeconds << " }";
        if (i + 1 < stats.sessions.size()) json << ",";
        json << "\n";
    }
    json << "  ],\n";
    json << "  \"customTiming\": { \"inhale\": " << stats.customTiming.inhale << ", \"hold\": " << stats.customTiming.hold << ", \"exhale\": " << stats.customTiming.exhale << " }\n";
    json << "}";
    return json.str();
}

bool deserialize_stats(const std::string& json_str, Stats& stats) {
    stats = Stats();
    auto extract_int = [&](const std::string& key) -> int {
        size_t pos = json_str.find("\"" + key + "\":");
        if (pos == std::string::npos) return 0;
        pos = json_str.find(":", pos) + 1;
        while (pos < json_str.length() && (json_str[pos] == ' ' || json_str[pos] == '\n' || json_str[pos] == '\r')) pos++;
        size_t end = json_str.find_first_of(",}\n\r", pos);
        if (end == std::string::npos) return 0;
        return std::stoi(json_str.substr(pos, end - pos));
    };
    stats.totalSessions = extract_int("totalSessions");
    stats.totalCycles = extract_int("totalCycles");
    stats.totalTimeSeconds = extract_int("totalTimeSeconds");
    stats.customTiming.inhale = extract_int("inhale");
    if (stats.customTiming.inhale <= 0) stats.customTiming.inhale = 4;
    stats.customTiming.hold = extract_int("hold");
    if (stats.customTiming.hold <= 0) stats.customTiming.hold = 7;
    stats.customTiming.exhale = extract_int("exhale");
    if (stats.customTiming.exhale <= 0) stats.customTiming.exhale = 8;
    return true;
}

// ─── Breath Trainer ──────────────────────────────────────────────────────

class BreathTrainer {
public:
    BreathTrainer() {
        home = get_home_dir();
        data_dir = home + "/.breath_trainer";
        std::filesystem::create_directories(data_dir);
        data_file = data_dir + "/stats.json";
        load();
        inhale = stats.customTiming.inhale;
        hold = stats.customTiming.hold;
        exhale = stats.customTiming.exhale;
    }

    void load() {
        std::ifstream file(data_file);
        if (!file.is_open()) {
            stats = Stats();
            stats.customTiming = Timing{4, 7, 8};
            return;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        if (!deserialize_stats(buffer.str(), stats)) {
            stats = Stats();
            stats.customTiming = Timing{4, 7, 8};
        }
    }

    void save() {
        std::string json = serialize_stats(stats);
        std::string temp = data_file + ".tmp";
        std::ofstream out(temp);
        if (out.is_open()) {
            out << json;
            out.close();
            std::filesystem::rename(temp, data_file);
        }
    }

    void sleep_ms(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    void show_stage(const std::string& stage, int duration, const std::string& color, const std::string& emoji) {
        int steps = duration * 2;
        for (int i = 0; i <= steps; ++i) {
            double pct = static_cast<double>(i) / steps * 100.0;
            int filled = static_cast<int>(static_cast<double>(i) / steps * 30);
            std::string bar = std::string(filled, '█') + std::string(30 - filled, '░');
            std::string color_code;
            if (color == "blue") color_code = COLOR_BLUE;
            else if (color == "yellow") color_code = COLOR_YELLOW;
            else if (color == "green") color_code = COLOR_GREEN;
            else color_code = COLOR_CYAN;
            std::cout << "\r  " << emoji << " " << C(stage, color_code) << " " << C(bar, color_code) << " " << std::setw(3) << static_cast<int>(pct) << "%" << std::flush;
            sleep_ms(500);
        }
        std::cout << std::endl;
    }

    double breathing_cycle() {
        auto start = std::chrono::steady_clock::now();
        if (sound_on) beep();
        show_stage("Breathe In...", inhale, "blue", "🌬️");
        if (sound_on) beep();
        show_stage("Hold...", hold, "yellow", "⏸️");
        if (sound_on) beep();
        show_stage("Exhale...", exhale, "green", "🌊");
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }

    void run_session(int cycles) {
        std::cout << C("\n🌬️ 4-7-8 Breathing Session – " + std::to_string(cycles) + " cycles", COLOR_BRIGHT) << C("", COLOR_CYAN) << std::endl;
        if (sound_on) beep();
        std::cout << C("Get ready...", COLOR_DIM) << std::endl;
        sleep_ms(1000);

        double total_duration = 0.0;
        for (int i = 0; i < cycles; ++i) {
            std::cout << C("\nCycle " + std::to_string(i+1) + "/" + std::to_string(cycles), COLOR_MAGENTA) << std::endl;
            total_duration += breathing_cycle();
        }

        if (sound_on) { beep(); beep(); beep(); }
        std::cout << C("\n✨ Session complete! ✨", COLOR_GREEN) << std::endl;
        std::cout << "  Cycles: " << cycles << std::endl;
        std::cout << "  Duration: " << std::fixed << std::setprecision(1) << total_duration << " seconds" << std::endl;
        stats.totalSessions++;
        stats.totalCycles += cycles;
        stats.totalTimeSeconds += static_cast<int>(total_duration);
        stats.sessions.push_back({get_timestamp(), cycles, static_cast<int>(total_duration)});
        if (stats.sessions.size() > 50) stats.sessions.erase(stats.sessions.begin());
        save();
    }

    void show_stats() {
        std::cout << "\n📊 STATISTICS" << std::endl;
        std::cout << C(std::string(30, '─'), COLOR_DIM) << std::endl;
        std::cout << "  Total Sessions: " << stats.totalSessions << std::endl;
        std::cout << "  Total Cycles:   " << stats.totalCycles << std::endl;
        int mins = stats.totalTimeSeconds / 60;
        int secs = stats.totalTimeSeconds % 60;
        std::cout << "  Total Time:     " << mins << "m " << secs << "s" << std::endl;
        if (!stats.sessions.empty()) {
            auto& last = stats.sessions.back();
            std::cout << "  Last Session:   " << last.cycles << " cycles (" << last.durationSeconds << "s)" << std::endl;
            std::cout << "\n📅 Recent Sessions:" << std::endl;
            int start = 0;
            if (stats.sessions.size() > 5) start = stats.sessions.size() - 5;
            for (size_t i = start; i < stats.sessions.size(); ++i) {
                auto& s = stats.sessions[i];
                std::string d = s.date.substr(0, 16);
                std::replace(d.begin(), d.end(), 'T', ' ');
                std::cout << "  " << d << "  " << s.cycles << " cycles  " << s.durationSeconds << "s" << std::endl;
            }
        }
    }

    void set_timing() {
        int inh = ask_int("Inhale duration (default " + std::to_string(inhale) + "): ");
        int h = ask_int("Hold duration (default " + std::to_string(hold) + "): ");
        int exh = ask_int("Exhale duration (default " + std::to_string(exhale) + "): ");
        if (inh > 0) inhale = inh;
        if (h > 0) hold = h;
        if (exh > 0) exhale = exh;
        stats.customTiming = Timing{inhale, hold, exhale};
        save();
        std::cout << C("✅ Timing set: " + std::to_string(inhale) + "s inhale, " + std::to_string(hold) + "s hold, " + std::to_string(exhale) + "s exhale", COLOR_GREEN) << std::endl;
    }

    void toggle_sound() {
        sound_on = !sound_on;
        std::cout << C("🔊 Sound " + std::string(sound_on ? "on" : "off"), COLOR_CYAN) << std::endl;
    }

    void reset_stats() {
        std::cout << "⚠️  Reset all statistics? (yes/no): ";
        std::string ans;
        std::getline(std::cin, ans);
        if (toLower(trim(ans)) == "yes") {
            stats.totalSessions = 0;
            stats.totalCycles = 0;
            stats.totalTimeSeconds = 0;
            stats.sessions.clear();
            save();
            std::cout << C("🗑️  Statistics reset.", COLOR_YELLOW) << std::endl;
        }
    }

    // ─── Menu ──────────────────────────────────────────────────────────────

    std::string ask(const std::string& prompt) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        return trim(line);
    }

    int ask_int(const std::string& prompt) {
        while (true) {
            std::string ans = ask(prompt);
            if (ans.empty()) return 0;
            try {
                return std::stoi(ans);
            } catch (...) {
                std::cout << C("❌ Please enter a number.", COLOR_RED) << std::endl;
            }
        }
    }

    void show_menu() {
        std::cout << "\n" << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
        std::cout << C("🌬️ BREATH TRAINER", COLOR_BRIGHT) << C("", COLOR_CYAN) << std::endl;
        std::cout << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
        std::cout << "  4-7-8: Inhale " << inhale << "s, Hold " << hold << "s, Exhale " << exhale << "s" << std::endl;
        std::cout << "  Sound: " << (sound_on ? "🔊 On" : "🔇 Off") << std::endl;
        std::cout << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
        std::cout << "  1. 🌬️ Start Session (4 cycles)" << std::endl;
        std::cout << "  2. 🌬️ Custom Session (choose cycles)" << std::endl;
        std::cout << "  3. 📊 Statistics" << std::endl;
        std::cout << "  4. ⏱️ Set Timings" << std::endl;
        std::cout << "  5. 🔇 Toggle Sound" << std::endl;
        std::cout << "  6. 🗑️ Reset Statistics" << std::endl;
        std::cout << "  0. 🚪 Exit" << std::endl;
        std::cout << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
    }

    void run() {
        std::cout << "\033[2J\033[1;1H";
        std::cout << C("\n🌬️ Breath Trainer – 4-7-8 Relaxation", COLOR_BRIGHT) << C("", COLOR_CYAN) << std::endl;
        std::cout << C("Breathe in peace, hold calm, exhale stress.", COLOR_DIM) << std::endl;

        while (true) {
            show_menu();
            std::string choice = ask("Your choice: ");
            if (choice == "1") {
                run_session(4);
            } else if (choice == "2") {
                int cycles = ask_int("Number of cycles (default 4): ");
                if (cycles <= 0) cycles = 4;
                run_session(cycles);
            } else if (choice == "3") {
                show_stats();
            } else if (choice == "4") {
                set_timing();
            } else if (choice == "5") {
                toggle_sound();
            } else if (choice == "6") {
                reset_stats();
            } else if (choice == "0") {
                std::cout << C("👋 Breathe deeply! Goodbye!", COLOR_CYAN) << std::endl;
                break;
            } else {
                std::cout << C("❌ Invalid choice.", COLOR_RED) << std::endl;
            }
            if (choice != "0") {
                std::cout << "\nPress Enter to continue...";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.get();
            }
        }
    }

private:
    std::string home, data_dir, data_file;
    Stats stats;
    int inhale, hold, exhale;
    bool sound_on = true;
};

int main() {
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    try {
        BreathTrainer app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << C("❌ Unexpected error: ", COLOR_RED) << e.what() << std::endl;
        return 1;
    }
    return 0;
}
