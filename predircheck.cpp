#include <iostream>
#include <filesystem>
#include <regex>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <unistd.h>

namespace fs = std::filesystem;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

const string VER = "1.5";

const string SKIPSECTIONS = "/REQUESTS|/GROUPS|/PRIVATE|/_SPEED";

const string SKIPUSERS = "";
const string SKIPGROUPS = "";
const string SKIPFLAGS = "";

const string NUKE_PREFIX = "";

const string DIRLOGLIST_GL = "/bin/dirloglist_gl";

const string SKIPDIRS = "^sample$|^Dis[ck].*|^vobsub.*|^extra.*|^cover.*|^sub.*|^bonus.*|^approved$|^allowed$|^ac3.*|^CD[0-9]|^Proof|^DVD[0-9]$|^DISC[0-9]$|^S[0-9]$";

const string ALLOWFILE = "/tmp/tur-predircheck.allow";

const bool DENYSAMENAMEDIRS = true;

const string DENYGROUPS = "/site:(-)(ANiHLS|ANiURL|AVTOMAT|BiPOLAR|CLASSiCO|SHiNiGAMi|SKYANiME|TVARCHiV|KAWAII|HOA)$";

const string DENYDIRS =
    "/site:[-._](SUBBED|BONUS|EXTRAS|SUBFRENCH|DUTCH|FLEMISH|FRENCH|ITALIAN)[-._]\n"
    "/site/incoming/MOVIES-FOREIGN:[-._](MULTI)[-._]\n"
    "/site/incoming/TV-FOREIGN:[-._](MULTI)[-._]";
	
const string ALLOWDIRS = "";
const bool ALLOWDIRS_OVERRULES_DENYGROUPS = false;
const bool ALLOWDIRS_OVERRULES_DENYDIRS = false;

const string GLLOG = "/ftp-data/logs/glftpd.log";

const string BOLD = "";

const bool DEBUG = false;

void proc_debug(const string &msg) {
    if (DEBUG)
        cout << "#0PreDirCheck: " << msg << endl;
}

void proc_announce(const string &message) {
    if (!GLLOG.empty()) {
        std::ofstream logFile(GLLOG, std::ios::app);
        if (!logFile) {
            proc_debug("Error. Can not write to " + GLLOG);
        } else {
            proc_debug("Sending to gllog: " + message);
            logFile << message << std::endl;
        }
    }
}

void proc_checkallow(const string &name) {
    if (!ALLOWFILE.empty() && fs::exists(ALLOWFILE)) {
        std::ifstream allowFile(ALLOWFILE);
        string line;
        while (std::getline(allowFile, line)) {
            if (line == name) {
                proc_debug("Stop & Allow: This file has been allowed");
                exit(0);
            }
        }
    }
}

void error_exit(const string &errorMsg, const string &announceMsg) {
    if (!announceMsg.empty())
        proc_announce(announceMsg);
    std::cerr << errorMsg << std::endl;
    exit(2);
}

string toLower(const string &s) {
    string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

string regex_escape(const string &s) {
    static const std::regex re("([.^$|()\\[\\]{}*+?\\\\])");
    return std::regex_replace(s, re, "\\$1");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        proc_debug("Stop & Allow: Didn't get 2 arguments.");
        return 0;
    }
    string dirName = argv[1];
    string parentPath = argv[2];
    string envUser = (std::getenv("USER") ? std::getenv("USER") : "");
    string envGroup = (std::getenv("GROUP") ? std::getenv("GROUP") : "");
    string envFlags = (std::getenv("FLAGS") ? std::getenv("FLAGS") : "");
    if (DEBUG && !DIRLOGLIST_GL.empty()) {
        if (access(DIRLOGLIST_GL.c_str(), X_OK) != 0) {
            proc_debug("ERROR: Can't execute DIRLOGLIST_GL: " + DIRLOGLIST_GL);
            return 1;
        }
        proc_debug("Testing dirloglist_gl binary. Should output 5 last lines in dirlog.");
        FILE *pipe = popen(DIRLOGLIST_GL.c_str(), "r");
        if (pipe) {
            char buffer[256];
            std::vector<string> lines;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                lines.push_back(string(buffer));
            }
            pclose(pipe);
            int start = std::max(0, static_cast<int>(lines.size()) - 5);
            for (int i = start; i < lines.size(); i++) {
                proc_debug(lines[i]);
            }
        }
    }
    if (!SKIPSECTIONS.empty()) {
        try {
            std::regex skipSectionsRegex(SKIPSECTIONS, std::regex_constants::icase);
            if (std::regex_search(parentPath, skipSectionsRegex)) {
                proc_debug("Stop & Allow: Excluded section in SKIPSECTIONS");
                return 0;
            }
        } catch (const std::regex_error &e) {
            proc_debug("Regex error in SKIPSECTIONS: " + string(e.what()));
        }
    }
    if (!SKIPUSERS.empty()) {
        try {
            std::regex skipUsersRegex(SKIPUSERS, std::regex_constants::icase);
            if (std::regex_search(envUser, skipUsersRegex)) {
                proc_debug("Stop & Allow: Excluded user in SKIPUSERS");
                return 0;
            }
        } catch (const std::regex_error &e) {
            proc_debug("Regex error in SKIPUSERS: " + string(e.what()));
        }
    }
    if (!SKIPGROUPS.empty()) {
        try {
            std::regex skipGroupsRegex(SKIPGROUPS, std::regex_constants::icase);
            if (std::regex_search(envGroup, skipGroupsRegex)) {
                proc_debug("Stop & Allow: Excluded group " + envGroup + " in SKIPGROUPS");
                return 0;
            }
        } catch (const std::regex_error &e) {
            proc_debug("Regex error in SKIPGROUPS: " + string(e.what()));
        }
    }
    if (!SKIPFLAGS.empty()) {
        try {
            std::regex skipFlagsRegex(SKIPFLAGS, std::regex_constants::icase);
            if (std::regex_search(envFlags, skipFlagsRegex)) {
                proc_debug("Stop & Allow: Excluded flag on " + envUser + ". (" + envFlags + ") in SKIPFLAGS");
                return 0;
            }
        } catch (const std::regex_error &e) {
            proc_debug("Regex error in SKIPFLAGS: " + string(e.what()));
        }
    }
    proc_checkallow(dirName);
    fs::path fullPath = fs::path(parentPath) / dirName;
    if (!fs::exists(fullPath)) {
        if (DENYSAMENAMEDIRS) {
            try {
                std::regex sameNameRegex("/" + regex_escape(dirName) + "(/|$)", std::regex_constants::icase);
                if (std::regex_search(parentPath, sameNameRegex)) {
                    proc_debug("Stop & Deny: This dir exists within the current path already.");
                    string ierror6 = BOLD + "-[Wanker]- " + envUser + BOLD + " tried to create " +
                                     BOLD + dirName + BOLD + " which is already present in a parent folder.";
                    string error6 = "Denying creation of " + dirName + ". Already exists a same name directory in parent.";
                    error_exit(error6, ierror6);
                }
            } catch (const std::regex_error &e) {
                proc_debug("Regex error in DENYSAMENAMEDIRS check: " + string(e.what()));
            }
        }
        if (fs::is_directory(parentPath)) {
            for (const auto &entry : fs::directory_iterator(parentPath)) {
                if (entry.is_directory()) {
                    string entryName = entry.path().filename().string();
                    if (toLower(entryName) == toLower(dirName) && entryName != dirName) {
                        proc_debug("Stop & Deny: This dir exists here with another case structure already.");
                        string ierror1 = BOLD + "-[Wanker]- " + envUser + BOLD + " tried to create " +
                                         BOLD + dirName + BOLD + " which already exists with a different case structure.";
                        string error1 = dirName + " already exists with a different case structure. Skipping.";
                        error_exit(error1, ierror1);
                    }
                }
            }
        }
        if (!NUKE_PREFIX.empty()) {
            fs::path nukePath = NUKE_PREFIX + dirName;
            if (fs::exists(nukePath) && fs::is_directory(nukePath)) {
                proc_checkallow(dirName);
                proc_debug("Stop & Deny: This dir has been nuked and found by NUKE_PREFIX.");
                string ierror2 = BOLD + "-[Wanker]- " + envUser + BOLD + " tried to create " +
                                 BOLD + dirName + BOLD + " which is already on site or was nuked.";
                string error2 = dirName + " is already on site or was nuked. Wanker.";
                error_exit(error2, ierror2);
            }
        }
        if (!SKIPDIRS.empty()) {
            try {
                std::regex skipDirsRegex(SKIPDIRS, std::regex_constants::icase);
                if (std::regex_search(dirName, skipDirsRegex)) {
                    proc_debug("Stop & Allow: This dirname is excluded in SKIPDIRS");
                    return 0;
                }
            } catch (const std::regex_error &e) {
                proc_debug("Regex error in SKIPDIRS: " + string(e.what()));
            }
        }
        bool allow_overrule_dir = false;
        bool allow_overrule_group = false;
        if (!ALLOWDIRS.empty()) {
            std::istringstream allowDirsStream(ALLOWDIRS);
            string rawdata;
            while (std::getline(allowDirsStream, rawdata)) {
                if (rawdata.empty()) continue;
                size_t pos = rawdata.find(':');
                if (pos == string::npos) continue;
                string section = rawdata.substr(0, pos);
                string allowed = rawdata.substr(pos + 1);
                try {
                    std::regex sectionRegex(section, std::regex_constants::icase);
                    if (std::regex_search(parentPath, sectionRegex)) {
                        std::regex allowedRegex(allowed, std::regex_constants::icase);
                        if (!std::regex_search(dirName, allowedRegex)) {
                            proc_checkallow(dirName);
                            proc_debug("Stop & Deny: This dir/group is not allowed.");
                            string ierror5 = BOLD + "-[Wanker]- " + envUser + BOLD + " tried to create " +
                                             BOLD + dirName + BOLD + " which isn't an allowed group or release.";
                            string error5 = "Denying creation of " + dirName + ". Not allowed group/release.";
                            error_exit(error5, ierror5);
                        } else {
                            if (ALLOWDIRS_OVERRULES_DENYDIRS)
                                allow_overrule_dir = true;
                            if (ALLOWDIRS_OVERRULES_DENYGROUPS)
                                allow_overrule_group = true;
                        }
                    }
                } catch (const std::regex_error &e) {
                    proc_debug("Regex error in ALLOWDIRS processing: " + string(e.what()));
                }
            }
        }
        if (!DENYGROUPS.empty() && !allow_overrule_group) {
            std::istringstream denyGroupsStream(DENYGROUPS);
            string rawdata;
            while (std::getline(denyGroupsStream, rawdata)) {
                if (rawdata.empty()) continue;
                size_t pos = rawdata.find(':');
                if (pos == string::npos) continue;
                string section = rawdata.substr(0, pos);
                string deniedGroup = rawdata.substr(pos + 1);
                try {
                    std::regex sectionRegex(section, std::regex_constants::icase);
                    if (std::regex_search(parentPath, sectionRegex)) {
                        std::regex deniedGroupRegex(deniedGroup, std::regex_constants::icase);
                        if (std::regex_search(dirName, deniedGroupRegex)) {
                            proc_checkallow(dirName);
                            proc_debug("Stop & Deny: This group is banned.");
                            string ierror3 = BOLD + "-[Wanker]- " + envUser + BOLD + " tried to create " +
                                             BOLD + dirName + BOLD + " which is from a BANNED GROUP.";
                            string error3 = "Denying creation of " + dirName + ". This group is BANNED!";
                            error_exit(error3, ierror3);
                        }
                    }
                } catch (const std::regex_error &e) {
                    proc_debug("Regex error in DENYGROUPS: " + string(e.what()));
                }
            }
        }
        if (!DENYDIRS.empty() && !allow_overrule_dir) {
            std::istringstream denyDirsStream(DENYDIRS);
            string rawdata;
            while (std::getline(denyDirsStream, rawdata)) {
                if (rawdata.empty()) continue;
                size_t pos = rawdata.find(':');
                if (pos == string::npos) continue;
                string section = rawdata.substr(0, pos);
                string denied = rawdata.substr(pos + 1);
                try {
                    std::regex sectionRegex(section, std::regex_constants::icase);
                    if (std::regex_search(parentPath, sectionRegex)) {
                        std::regex deniedRegex(denied, std::regex_constants::icase);
                        if (std::regex_search(dirName, deniedRegex)) {
                            proc_checkallow(dirName);
                            proc_debug("Stop & Deny: This dir seems denied in DENYDIRS");
                            string ierror4 = BOLD + "-[Wanker]- " + envUser + BOLD + " tried to create " +
                                             BOLD + dirName + BOLD + " but was denied.";
                            string error4 = "Denying creation of " + dirName + ". Watch what you're doing!";
                            error_exit(error4, ierror4);
                        }
                    }
                } catch (const std::regex_error &e) {
                    proc_debug("Regex error in DENYDIRS: " + string(e.what()));
                }
            }
        }
        if (!DIRLOGLIST_GL.empty()) {
            FILE *pipe = popen(DIRLOGLIST_GL.c_str(), "r");
            if (pipe) {
                char buffer[512];
                bool found = false;
                string pattern = "/" + regex_escape(dirName) + "$";
                try {
                    std::regex dirlogRegex(pattern, std::regex_constants::icase);
                    while (fgets(buffer, sizeof(buffer), pipe)) {
                        string line(buffer);
                        if (std::regex_search(line, dirlogRegex)) {
                            found = true;
                            break;
                        }
                    }
                } catch (const std::regex_error &e) {
                    proc_debug("Regex error in DIRLOGLIST_GL output processing: " + string(e.what()));
                }
                pclose(pipe);
                if (found) {
                    proc_checkallow(dirName);
                    proc_debug("Stop & Deny: It was found in dirlog, thus already upped before (NOPARENT CHECK).");
                    string ierror2 = BOLD + "-[Wanker]- " + envUser + BOLD + " tried to create " +
                                     BOLD + dirName + BOLD + " which is already on site or was nuked.";
                    string error2 = dirName + " is already on site or was nuked. Wanker.";
                    error_exit(error2, ierror2);
                }
            }
        }
    }
    proc_debug("Stop & Allow: This dir passed all checks.");
    return 0;
}
