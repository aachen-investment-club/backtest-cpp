#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#define popen _popen
#define pclose _pclose
#endif

// ANSI Terminal colors
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string WHITE = "\033[0m";

std::string getCurrentDateTime() {
    std::time_t t = std::time(nullptr);
    char buf[32];
#if defined(_MSC_VER)
    struct std::tm buf_tm;
    localtime_s(&buf_tm, &t);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d:%H:%M", &buf_tm);
#else
    std::strftime(buf, sizeof(buf), "%Y-%m-%d:%H:%M", std::localtime(&t));
#endif
    return std::string(buf);
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Exception-safe string to double
double safeStod(const std::string& s) {
    try {
        return std::stod(s);
    } catch (...) {
        return 0.0;  // Handles "<not supported>" safely
    }
}

void printComparison(const std::string& name, double current, double previous,
                     bool higher_is_better) {
    std::cout << std::left << std::setw(20) << name << ": " << std::setw(10) << current;
    if (previous == 0.0) {
        std::cout << " (No previous data)\n";
        return;
    }

    double percent_change = ((current - previous) / previous) * 100.0;

    bool improved = higher_is_better ? (percent_change > 5.0) : (percent_change < -5.0);
    bool worsened = higher_is_better ? (percent_change < -5.0) : (percent_change > 5.0);

    if (improved)
        std::cout << GREEN;
    else if (worsened)
        std::cout << RED;
    else
        std::cout << WHITE;

    std::cout << "(" << (percent_change > 0 ? "+" : "") << std::fixed << std::setprecision(2)
              << percent_change << "% vs " << previous << ")" << WHITE << "\n";
}

int main(int argc, char* argv[]) {
    // TODO: Needs refactoring in the future, the plan is for the backtester to pull in all data
    // from a specified folder
    std::string dataset = (argc > 1) ? argv[1] : "./data/NQ_sample.csv";
    std::string datetime = getCurrentDateTime();

    // MAIN PERFORMANCE COMMAND
    std::string cmd =
        "perf stat -x, -e "
        "instructions,cycles,L1-dcache-loads,L1-dcache-load-misses,branches,branch-misses "
        "./build/release/src/backtest " +
        dataset + " 2>&1";

    std::cout << "Running backtest and profiling...\n\n";

    double throughput = 0.0, max_rss = 0.0;
    double instructions = 0, cycles = 0;
    double l1_loads = 0, l1_misses = 0;
    double branches = 0, branch_misses = 0;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run command.\n";
        return 1;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);

        if (line.find("Throughput:") != std::string::npos && line.find("M") != std::string::npos) {
            size_t pos = line.find("Throughput:") + 11;
            throughput = safeStod(line.substr(pos));
        } else if (line.find("Max Resident Size:") == 0) {
            max_rss = safeStod(line.substr(18));
        }

        auto cols = split(line, ',');
        if (cols.size() > 2) {
            std::string val_str = cols[0];
            std::string event = cols[2];

            // Filter out Intel E-Cores (cpu_atom). We only care about P-Cores (cpu_core)
            if (event.find("atom") != std::string::npos) continue;

            if (event.find("instructions") != std::string::npos) {
                instructions = safeStod(val_str);
            } else if (event.find("cycles") != std::string::npos) {
                cycles = safeStod(val_str);
            } else if (event.find("L1-dcache-load-misses") != std::string::npos) {
                l1_misses = safeStod(val_str);
            } else if (event.find("L1-dcache-loads") != std::string::npos) {
                l1_loads = safeStod(val_str);
            } else if (event.find("branch-misses") != std::string::npos) {
                branch_misses = safeStod(val_str);
            } else if (event.find("branches") != std::string::npos) {
                branches = safeStod(val_str);
            }
        }
    }
    pclose(pipe);

    double ipc = cycles > 0 ? (instructions / cycles) : 0;
    double l1_miss_pct = l1_loads > 0 ? ((l1_misses / l1_loads) * 100.0) : 0;
    double branch_miss_pct = branches > 0 ? ((branch_misses / branches) * 100.0) : 0;

    double old_throughput = 0.0, old_l1_miss = 0.0, old_branch_miss = 0.0, old_ipc = 0.0,
           old_rss = 0.0;
    std::ifstream file_in("performance/PERFORMANCE.md");
    std::string last_line;
    if (file_in.is_open()) {
        std::string temp;
        while (std::getline(file_in, temp)) {
            if (!temp.empty() && temp[0] == '|') {
                last_line = temp;
            }
        }
        file_in.close();

        if (!last_line.empty() && last_line.find("Throughput") == std::string::npos &&
            last_line.find("---") == std::string::npos) {
            auto cols = split(last_line, '|');
            if (cols.size() >= 8) {
                old_throughput = safeStod(cols[3]);
                old_l1_miss = safeStod(cols[4]);
                old_branch_miss = safeStod(cols[5]);
                old_ipc = safeStod(cols[6]);
                old_rss = safeStod(cols[7]);
            }
        }
    }

    std::cout << "=== PERFORMANCE RESULTS ===\n";
    printComparison("Throughput (M/s)", throughput, old_throughput, true);
    printComparison("L1d Miss %", l1_miss_pct, old_l1_miss, false);
    printComparison("Branch Miss %", branch_miss_pct, old_branch_miss, false);
    printComparison("IPC", ipc, old_ipc, true);
    printComparison("Max RSS (MB)", max_rss, old_rss, false);
    std::cout << "===========================\n\n";

    std::cout << "Notes for this run: ";
    std::string notes;
    std::getline(std::cin, notes);

    std::cout << "Write to PERFORMANCE.md? (Y/n): ";
    std::string ans;
    std::getline(std::cin, ans);

    if (ans.empty() || ans == "Y" || ans == "y") {
        std::ofstream file_out("performance/PERFORMANCE.md", std::ios::app);
        if (file_out.is_open()) {
            file_out << "|" << datetime << "|" << dataset << "|" << std::fixed
                     << std::setprecision(4) << throughput << "|" << std::setprecision(2)
                     << l1_miss_pct << "|" << branch_miss_pct << "|" << ipc << "|" << max_rss << "|"
                     << notes << "|\n";
            std::cout << "Successfully appended to PERFORMANCE.md.\n";
        }
    } else {
        std::cout << "Discarded.\n";
    }

    return 0;
}