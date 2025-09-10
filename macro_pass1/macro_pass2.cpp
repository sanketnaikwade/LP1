#include <bits/stdc++.h>
using namespace std;

static inline string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
static inline vector<string> split_commas(const string &s) {
    vector<string> out; string cur; stringstream ss(s);
    while (getline(ss, cur, ',')) {
        size_t a = cur.find_first_not_of(" \t\r\n");
        if (a==string::npos) out.push_back("");
        else {
            size_t b = cur.find_last_not_of(" \t\r\n");
            out.push_back(cur.substr(a, b-a+1));
        }
    }
    return out;
}

struct MNTEntry { string name; int mdtIndex; };
vector<MNTEntry> MNT;
vector<string> MDT;
map<string, vector<string>> PNTABs;
map<string, map<string,string>> KPDTAB;

const int MAX_DEPTH = 30;

void expandMacro(const string &macroName,
                 const vector<string> &actuals,
                 ofstream &out,
                 vector<string> &stack,
                 unordered_set<string> &active)
{
    if (active.count(macroName)) return; // skip recursive call
    if ((int)stack.size() >= MAX_DEPTH) return; // prevent infinite nesting

    if (!PNTABs.count(macroName)) return;

    vector<string> formals = PNTABs[macroName];
    vector<string> ala(formals.size());
    for (size_t i=0; i<formals.size(); ++i) {
        if (KPDTAB[macroName].count(formals[i]))
            ala[i] = KPDTAB[macroName][formals[i]];
        else
            ala[i] = formals[i];
    }
    for (size_t i=0; i<actuals.size() && i<ala.size(); ++i)
        if (!actuals[i].empty())
            ala[i] = actuals[i];

    int start = -1;
    for (auto &e : MNT) if (e.name == macroName) { start = e.mdtIndex-1; break; }
    if (start == -1) return;

    active.insert(macroName);
    stack.push_back(macroName);

    // Start expansion: skip first line (parameter list), stop at MEND
    for (int i = start+1; i<(int)MDT.size(); ++i) {
        string line = trim(MDT[i]);
        if (line == "MEND") break;

        // replace #1,#2,...
        for (size_t j=0; j<formals.size(); ++j) {
            string key = "#" + to_string(j+1);
            size_t pos;
            while ((pos = line.find(key)) != string::npos)
                line.replace(pos, key.size(), ala[j]);
        }

        // check for nested macro call
        bool nested = false;
        for (auto &m : MNT) {
            size_t pos = line.find(m.name);
            if (pos != string::npos) {
                string paramsStr = trim(line.substr(pos+m.name.size()));
                while (!paramsStr.empty() && (paramsStr[0]==',' || isspace((unsigned char)paramsStr[0])))
                    paramsStr.erase(paramsStr.begin());
                vector<string> nestedArgs = split_commas(paramsStr);
                expandMacro(m.name, nestedArgs, out, stack, active);
                nested = true;
                break;
            }
        }

        if (!nested && !line.empty())
            out << line << "\n";
    }

    stack.pop_back();
    active.erase(macroName);
}

int main() {
    ifstream mntFile("MNT.txt"), mdtFile("MDT.txt"), pntFile("PNTAB.txt"), kpdFile("KPDTAB.txt"), icFile("IC.txt");
    ofstream out("output.txt");

    // Load MNT (index name ... mdtIndex)
    string line;
    getline(mntFile, line);
    if (line.find("Index") == string::npos) {
        mntFile.clear(); mntFile.seekg(0);
    }
    while (getline(mntFile, line)) {
        string name; int idx, pnt, kpd, mdt;
        stringstream ss(line);
        if (ss >> idx >> name >> pnt >> kpd >> mdt)
            MNT.push_back({name, mdt});
    }

    // Load MDT
    while (getline(mdtFile, line)) {
        size_t pos = line.find('\t');
        if (pos != string::npos)
            MDT.push_back(trim(line.substr(pos+1)));
        else
            MDT.push_back(trim(line));
    }

    // Load KPDTAB
    while (getline(kpdFile, line)) {
        int idx; string mac, key, val;
        stringstream ss(line);
        if (ss >> idx >> mac >> key >> val)
            KPDTAB[mac][key] = val;
    }

    // Load PNTAB
    string current;
    while (getline(pntFile, line)) {
        line = trim(line);
        if (line.empty()) continue;
        size_t c = line.find(':');
        if (c != string::npos) {
            current = trim(line.substr(0,c));
            PNTABs[current] = {};
        } else if (!current.empty()) {
            stringstream ss(line);
            int idx; string param;
            ss >> idx >> param;
            PNTABs[current].push_back(param);
        }
    }

    // Process IC
    while (getline(icFile, line)) {
        string op;
        stringstream ss(line);
        ss >> op;
        bool macroCall = false;
        for (auto &m : MNT) {
            if (m.name == op) {
                string paramsStr = trim(line.substr(op.size()));
                while (!paramsStr.empty() && (paramsStr[0]==',' || isspace((unsigned char)paramsStr[0])))
                    paramsStr.erase(paramsStr.begin());
                vector<string> args = split_commas(paramsStr);
                vector<string> stack;
                unordered_set<string> active;
                expandMacro(op, args, out, stack, active);
                macroCall = true;
                break;
            }
        }
        if (!macroCall)
            out << line << "\n";
    }

    cout << "Pass 2 clean done. See output.txt\n";
    return 0;
}