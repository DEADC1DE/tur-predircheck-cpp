#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <cerrno>

namespace fs = std::filesystem;

struct Config {
    std::string VER;
    std::regex SKIPSECTIONS;
    std::regex SKIPUSERS;
    std::regex SKIPGROUPS;
    std::regex SKIPFLAGS;
    std::string NUKE_PREFIX;
    std::string DIRLOGLIST_GL;
    std::regex SKIPDIRS;
    std::string ALLOWFILE;
    bool DENYSAMENAMEDIRS;
    std::vector<std::pair<std::regex, std::regex>> DENYGROUPS;
    std::vector<std::pair<std::regex, std::regex>> DENYDIRS;
    std::vector<std::pair<std::regex, std::regex>> ALLOWDIRS;
    bool ALLOWDIRS_OVERRULES_DENYGROUPS;
    bool ALLOWDIRS_OVERRULES_DENYDIRS;
    std::string ERROR1, ERROR2, ERROR3, ERROR4, ERROR5, ERROR6;
    bool DEBUG;
    std::string GLLOG;
    std::string BOLD = "\x02";
    std::string IERROR1, IERROR2, IERROR3, IERROR4, IERROR5, IERROR6;
};

static void processConfigEntry(Config& config, const std::string& key, const std::string& value);

static std::string replacePlaceholders(const std::string& input,
                                       const std::string& dirName,
                                       const std::string& user,
                                       const std::string& bold) {
    std::string result = input;
    size_t pos;
    while ((pos = result.find("{1}")) != std::string::npos)
        result.replace(pos, 3, dirName);
    while ((pos = result.find("{USER}")) != std::string::npos)
        result.replace(pos, 6, user);
    while ((pos = result.find("{BOLD}")) != std::string::npos)
        result.replace(pos, 6, bold);
    return result;
}

static Config readConfig(const fs::path& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file " << configFile
                  << " (" << strerror(errno) << ")" << std::endl;
        exit(1);
    }
    Config config;
    std::string line;
    bool inMultiline = false;
    std::string multilineKey;
    std::vector<std::string> multilineLines;
    std::string currentSection;

    while (std::getline(file, line)) {
        auto ltrim = [](std::string &s) { s.erase(0, s.find_first_not_of(" \t")); };
        auto rtrim = [](std::string &s) { s.erase(s.find_last_not_of(" \t\r\n") + 1); };
        ltrim(line);
        rtrim(line);
        if (line.empty() || line[0] == '#') continue;

        if (!inMultiline && line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            if (config.DEBUG) std::cout << "DEBUG: Entering section [" << currentSection << "]" << std::endl;
            continue;
        }
        if (!inMultiline && !currentSection.empty() && line.find('=') == std::string::npos) {
            if (config.DEBUG) std::cout << "DEBUG: Processing section entry: " << currentSection << " = " << line << std::endl;
            processConfigEntry(config, currentSection, line);
            continue;
        }
        currentSection.clear();

        if (!inMultiline) {
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) continue;
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            ltrim(value);
            rtrim(value);

            if (value.empty()) continue;

            if (value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2); // Entferne Anführungszeichen
                if (config.DEBUG) std::cout << "DEBUG: Processing single-line with quotes: " << key << " = " << value << std::endl;
                processConfigEntry(config, key, value);
            } else if (value.front() == '"') {
                inMultiline = true;
                multilineKey = key;
                multilineLines.clear();
                std::string first = value.substr(1);
                ltrim(first);
                rtrim(first);
                multilineLines.push_back(first);
                if (config.DEBUG) std::cout << "DEBUG: Starting multi-line for key: " << key << ", first line: " << first << std::endl;
            } else {
                if (config.DEBUG) std::cout << "DEBUG: Processing single-line: " << key << " = " << value << std::endl;
                processConfigEntry(config, key, value);
            }
        } else {
            bool endsHere = (!line.empty() && line.back() == '"');
            std::string chunk = endsHere ? line.substr(0, line.size() - 1) : line;
            ltrim(chunk);
            rtrim(chunk);
            multilineLines.push_back(chunk);
            if (config.DEBUG) std::cout << "DEBUG: Multi-line continuation: " << chunk << std::endl;
            if (endsHere) {
                if (multilineKey == "DENYDIRS" || multilineKey == "DENYGROUPS" || multilineKey == "ALLOWDIRS") {
                    for (auto &entry : multilineLines) {
                        if (config.DEBUG) std::cout << "DEBUG: Processing multi-line list entry: " << multilineKey << " = " << entry << std::endl;
                        processConfigEntry(config, multilineKey, entry);
                    }
                } else {
                    std::string joined;
                    for (size_t i = 0; i < multilineLines.size(); ++i) {
                        if (i) joined += '\n';
                        joined += multilineLines[i];
                    }
                    if (config.DEBUG) std::cout << "DEBUG: Processing multi-line joined: " << multilineKey << " = " << joined << std::endl;
                    processConfigEntry(config, multilineKey, joined);
                }
                inMultiline = false;
            }
        }
    }
    if (config.DEBUG) std::cout << "DEBUG: Finished reading config file: " << configFile << std::endl;
    return config;
}

static void processConfigEntry(Config& config, const std::string& key, const std::string& value) {
    if (config.DEBUG) std::cout << "DEBUG: Setting config: " << key << " = " << value << std::endl;
    if (key == "VER") config.VER = value;
    else if (key == "SKIPSECTIONS") config.SKIPSECTIONS = std::regex(value.empty() ? "a^" : value, std::regex::icase);
    else if (key == "SKIPUSERS") config.SKIPUSERS = std::regex(value.empty() ? "a^" : value, std::regex::icase);
    else if (key == "SKIPGROUPS") config.SKIPGROUPS = std::regex(value.empty() ? "a^" : value, std::regex::icase);
    else if (key == "SKIPFLAGS") config.SKIPFLAGS = std::regex(value.empty() ? "a^" : value, std::regex::icase);
    else if (key == "NUKE_PREFIX") config.NUKE_PREFIX = value;
    else if (key == "DIRLOGLIST_GL") config.DIRLOGLIST_GL = value;
    else if (key == "SKIPDIRS") config.SKIPDIRS = std::regex(value.empty() ? "a^" : value, std::regex::icase);
    else if (key == "ALLOWFILE") config.ALLOWFILE = value;
    else if (key == "DENYSAMENAMEDIRS") config.DENYSAMENAMEDIRS = (value == "TRUE");
    else if (key == "ALLOWDIRS_OVERRULES_DENYGROUPS") config.ALLOWDIRS_OVERRULES_DENYGROUPS = (value == "TRUE");
    else if (key == "ALLOWDIRS_OVERRULES_DENYDIRS") config.ALLOWDIRS_OVERRULES_DENYDIRS = (value == "TRUE");
    else if (key == "ERROR1") config.ERROR1 = value;
    else if (key == "ERROR2") config.ERROR2 = value;
    else if (key == "ERROR3") config.ERROR3 = value;
    else if (key == "ERROR4") config.ERROR4 = value;
    else if (key == "ERROR5") config.ERROR5 = value;
    else if (key == "ERROR6") config.ERROR6 = value;
    else if (key == "DEBUG") config.DEBUG = (value == "TRUE");
    else if (key == "GLLOG") config.GLLOG = value;
    else if (key == "IERROR1") config.IERROR1 = value;
    else if (key == "IERROR2") config.IERROR2 = value;
    else if (key == "IERROR3") config.IERROR3 = value;
    else if (key == "IERROR4") config.IERROR4 = value;
    else if (key == "IERROR5") config.IERROR5 = value;
    else if (key == "IERROR6") config.IERROR6 = value;
    else if (key == "DENYGROUPS" || key == "DENYDIRS" || key == "ALLOWDIRS") {
        auto& list = (key == "DENYGROUPS") ? config.DENYGROUPS
                      : (key == "DENYDIRS") ? config.DENYDIRS
                                               : config.ALLOWDIRS;
        size_t start = 0;
        while (start < value.size()) {
            size_t end = value.find(' ', start);
            std::string entry = value.substr(start, (end == std::string::npos ? value.size() : end) - start);
            size_t colon = entry.find(':');
            if (colon != std::string::npos) {
                list.emplace_back(
                    std::regex(entry.substr(0, colon), std::regex::icase),
                    std::regex(entry.substr(colon + 1), std::regex::icase)
                );
                if (config.DEBUG) std::cout << "DEBUG: Added to " << key << ": " << entry.substr(0, colon) << ":" << entry.substr(colon + 1) << std::endl;
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }
}

static void announce(const Config& c, const std::string& msg,
                     const std::string& user, const std::string& dir) {
    if (c.DEBUG) std::cout << "DEBUG: Announcing message: " << msg << " for user: " << user << ", dir: " << dir << std::endl;
    if (c.GLLOG.empty()) return;
    std::ofstream log(c.GLLOG, std::ios::app);
    if (!log.is_open()) {
        if (c.DEBUG) std::cout << "DEBUG: Failed to open GLLOG: " << c.GLLOG << std::endl;
        return;
    }
    char ts[100];
    std::time_t now = std::time(nullptr);
    std::strftime(ts, sizeof(ts), "%a %b %e %T %Y", std::localtime(&now));
    log << ts << " TURGEN: \"" << replacePlaceholders(msg, dir, user, c.BOLD) << "\"" << std::endl;
}

static bool checkAllow(const Config& c, const std::string& dir) {
    if (c.DEBUG) std::cout << "DEBUG: Checking ALLOWFILE for dir: " << dir << std::endl;
    if (c.ALLOWFILE.empty() || !fs::exists(c.ALLOWFILE)) {
        if (c.DEBUG) std::cout << "DEBUG: ALLOWFILE empty or does not exist: " << c.ALLOWFILE << std::endl;
        return false;
    }
    std::ifstream f(c.ALLOWFILE);
    std::string ln;
    while (std::getline(f, ln)) {
        if (c.DEBUG) std::cout << "DEBUG: Checking ALLOWFILE line: " << ln << std::endl;
        if (ln == dir) {
            if (c.DEBUG) std::cout << "DEBUG: Found match in ALLOWFILE for dir: " << dir << std::endl;
            return true;
        }
    }
    if (c.DEBUG) std::cout << "DEBUG: No match found in ALLOWFILE for dir: " << dir << std::endl;
    return false;
}

static bool endsWith(const std::string& s, const std::string& x) {
    bool result = s.size() >= x.size() && s.compare(s.size() - x.size(), x.size(), x) == 0;
    if (s.size() >= x.size() && s.compare(s.size() - x.size(), x.size(), x) == 0) {
        return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
    fs::path exe;
    try {
        exe = fs::canonical(fs::path(argv[0]));
    } catch (...) {
        exe = fs::current_path() / argv[0];
    }
    fs::path configPath = exe.parent_path() / "tur-predircheck.conf";

    std::string dirName, path;
    if (argc > 1) dirName = argv[1];
    if (argc > 2) path = argv[2];

    Config cfg = readConfig(configPath);
    if (cfg.DEBUG) {
        std::cout << "DEBUG: Config loaded from: " << configPath << std::endl;
        std::cout << "DEBUG: dirName: " << dirName << ", path: " << path << std::endl;
    }

    if (dirName.empty() || path.empty()) {
        if (cfg.DEBUG) std::cout << "DEBUG: Validating config only, no dirName or path provided" << std::endl;
        return 0;
    }
    if (path.front() != '/') path.insert(path.begin(), '/');
    if (cfg.DEBUG) std::cout << "DEBUG: Normalized path: " << path << std::endl;

    std::string USER = getenv("USER") ? getenv("USER") : "";
    std::string GROUP = getenv("GROUP") ? getenv("GROUP") : "";
    std::string FLAGS = getenv("FLAGS") ? getenv("FLAGS") : "";
    if (cfg.DEBUG) {
        std::cout << "DEBUG: USER: " << USER << ", GROUP: " << GROUP << ", FLAGS: " << FLAGS << std::endl;
    }

    if (std::regex_search(path, cfg.SKIPSECTIONS)) {
        if (cfg.DEBUG) std::cout << "DEBUG: Path matches SKIPSECTIONS, exiting" << std::endl;
        return 0;
    }
    if (std::regex_search(USER, cfg.SKIPUSERS)) {
        if (cfg.DEBUG) std::cout << "DEBUG: USER matches SKIPUSERS, exiting" << std::endl;
        return 0;
    }
    if (std::regex_search(GROUP, cfg.SKIPGROUPS)) {
        if (cfg.DEBUG) std::cout << "DEBUG: GROUP matches SKIPGROUPS, exiting" << std::endl;
        return 0;
    }
    if (std::regex_search(FLAGS, cfg.SKIPFLAGS)) {
        if (cfg.DEBUG) std::cout << "DEBUG: FLAGS matches SKIPFLAGS, exiting" << std::endl;
        return 0;
    }

    if (!fs::exists(path + "/" + dirName)) {
        if (cfg.DEBUG) std::cout << "DEBUG: Directory does not exist: " << path << "/" << dirName << std::endl;
        if (cfg.DENYSAMENAMEDIRS) {
            std::string pat = "/" + dirName + "/|/" + dirName + "$";
            if (std::regex_search(path, std::regex(pat, std::regex::icase))) {
                if (cfg.DEBUG) std::cout << "DEBUG: Path matches DENYSAMENAMEDIRS pattern: " << pat << std::endl;
                if (!checkAllow(cfg, dirName)) {
                    if (cfg.DEBUG) std::cout << "DEBUG: Not allowed, announcing IERROR6" << std::endl;
                    announce(cfg, cfg.IERROR6, USER, dirName);
                    std::cerr << replacePlaceholders(cfg.ERROR6, dirName, USER, cfg.BOLD) << std::endl;
                    return 2;
                }
                if (cfg.DEBUG) std::cout << "DEBUG: Allowed by ALLOWFILE, proceeding" << std::endl;
                return 0;
            }
        }
        if (fs::exists(path)) {
            if (cfg.DEBUG) std::cout << "DEBUG: Checking existing directories in: " << path << std::endl;
            for (auto &e : fs::directory_iterator(path)) {
                if (!e.is_directory()) continue;
                std::string nm = e.path().filename();
                if (std::equal(nm.begin(), nm.end(), dirName.begin(), dirName.end(),
                               [](char a, char b) { return tolower(a) == tolower(b); })) {
                    if (cfg.DEBUG) std::cout << "DEBUG: Found case-insensitive match: " << nm << std::endl;
                    if (!checkAllow(cfg, dirName)) {
                        if (cfg.DEBUG) std::cout << "DEBUG: Not allowed, announcing IERROR1" << std::endl;
                        announce(cfg, cfg.IERROR1, USER, dirName);
                        std::cerr << replacePlaceholders(cfg.ERROR1, dirName, USER, cfg.BOLD) << std::endl;
                        return 2;
                    }
                    if (cfg.DEBUG) std::cout << "DEBUG: Allowed by ALLOWFILE, proceeding" << std::endl;
                    return 0;
                }
            }
        }
        if (!cfg.NUKE_PREFIX.empty() && fs::exists(cfg.NUKE_PREFIX + dirName)) {
            if (cfg.DEBUG) std::cout << "DEBUG: Found directory in NUKE_PREFIX: " << cfg.NUKE_PREFIX + dirName << std::endl;
            if (!checkAllow(cfg, dirName)) {
                if (cfg.DEBUG) std::cout << "DEBUG: Not allowed, announcing IERROR2" << std::endl;
                announce(cfg, cfg.IERROR2, USER, dirName);
                std::cerr << replacePlaceholders(cfg.ERROR2, dirName, USER, cfg.BOLD) << std::endl;
                return 2;
            }
            if (cfg.DEBUG) std::cout << "DEBUG: Allowed by ALLOWFILE, proceeding" << std::endl;
            return 0;
        }
        if (std::regex_search(dirName, cfg.SKIPDIRS)) {
            if (cfg.DEBUG) std::cout << "DEBUG: dirName matches SKIPDIRS, exiting" << std::endl;
            return 0;
        }
        bool overG = false, overD = false;
        for (auto &a : cfg.ALLOWDIRS) {
            if (std::regex_search(path, a.first)) {
                if (cfg.DEBUG) std::cout << "DEBUG: Path matches ALLOWDIRS path regex" << std::endl;
                if (!std::regex_search(dirName, a.second)) {
                    if (cfg.DEBUG) std::cout << "DEBUG: dirName does not match ALLOWDIRS dir regex" << std::endl;
                    if (!checkAllow(cfg, dirName)) {
                        if (cfg.DEBUG) std::cout << "DEBUG: Not allowed, announcing IERROR5" << std::endl;
                        announce(cfg, cfg.IERROR5, USER, dirName);
                        std::cerr << replacePlaceholders(cfg.ERROR5, dirName, USER, cfg.BOLD) << std::endl;
                        return 2;
                    }
                    if (cfg.DEBUG) std::cout << "DEBUG: Allowed by ALLOWFILE, proceeding" << std::endl;
                    return 0;
                } else {
                    if (cfg.DEBUG) std::cout << "DEBUG: dirName matches ALLOWDIRS, setting overrules" << std::endl;
                    if (cfg.ALLOWDIRS_OVERRULES_DENYGROUPS) overG = true;
                    if (cfg.ALLOWDIRS_OVERRULES_DENYDIRS) overD = true;
                }
            }
        }
        if (!overG) {
            for (auto &d : cfg.DENYGROUPS) {
                if (std::regex_search(path, d.first) && std::regex_search(dirName, d.second)) {
                    if (cfg.DEBUG) std::cout << "DEBUG: Path and dirName match DENYGROUPS" << std::endl;
                    if (!checkAllow(cfg, dirName)) {
                        if (cfg.DEBUG) std::cout << "DEBUG: Not allowed, announcing IERROR3" << std::endl;
                        announce(cfg, cfg.IERROR3, USER, dirName);
                        std::cerr << replacePlaceholders(cfg.ERROR3, dirName, USER, cfg.BOLD) << std::endl;
                        return 2;
                    }
                    if (cfg.DEBUG) std::cout << "DEBUG: Allowed by ALLOWFILE, proceeding" << std::endl;
                    return 0;
                }
            }
        }
        if (!overD) {
            for (auto &d : cfg.DENYDIRS) {
                if (std::regex_search(path, d.first) && std::regex_search(dirName, d.second)) {
                    if (cfg.DEBUG) std::cout << "DEBUG: Path and dirName match DENYDIRS" << std::endl;
                    if (!checkAllow(cfg, dirName)) {
                        if (cfg.DEBUG) std::cout << "DEBUG: Not allowed, announcing IERROR4" << std::endl;
                        announce(cfg, cfg.IERROR4, USER, dirName);
                        std::cerr << replacePlaceholders(cfg.ERROR4, dirName, USER, cfg.BOLD) << std::endl;
                        return 2;
                    }
                    if (cfg.DEBUG) std::cout << "DEBUG: Allowed by ALLOWFILE, proceeding" << std::endl;
                    return 0;
                }
            }
        }
        if (!cfg.DIRLOGLIST_GL.empty()) {
            if (cfg.DEBUG) std::cout << "DEBUG: Executing DIRLOGLIST_GL command: " << cfg.DIRLOGLIST_GL << std::endl;
            FILE* pipe = popen(cfg.DIRLOGLIST_GL.c_str(), "r");
            if (pipe) {
                char buf[1024];
                while (fgets(buf, sizeof(buf), pipe)) {
                    std::string ln(buf);
                    if (ln.back() == '\n') ln.pop_back();
                    if (cfg.DEBUG) std::cout << "DEBUG: Checking DIRLOGLIST_GL line: " << ln << std::endl;
                    if (endsWith(ln, "/" + dirName)) {
                        if (cfg.DEBUG) std::cout << "DEBUG: Found match in DIRLOGLIST_GL: " << ln << std::endl;
                        if (!checkAllow(cfg, dirName)) {
                            if (cfg.DEBUG) std::cout << "DEBUG: Not allowed, announcing IERROR2" << std::endl;
                            announce(cfg, cfg.IERROR2, USER, dirName);
                            std::cerr << replacePlaceholders(cfg.ERROR2, dirName, USER, cfg.BOLD) << std::endl;
                            pclose(pipe);
                            return 2;
                        }
                        if (cfg.DEBUG) std::cout << "DEBUG: Allowed by ALLOWFILE, proceeding" << std::endl;
                        pclose(pipe);
                        return 0;
                    }
                }
                pclose(pipe);
            } else {
                if (cfg.DEBUG) std::cout << "DEBUG: Failed to execute DIRLOGLIST_GL command" << std::endl;
            }
        }
    }
    if (cfg.DEBUG) std::cout << "DEBUG: All checks passed, exiting with 0" << std::endl;
    return 0;
}