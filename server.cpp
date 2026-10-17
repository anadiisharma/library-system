// ============================================================
//  Library Management System  -  C++ backend + web frontend
//  Build: see README.md   |   Run, then open http://localhost:8080
//  Your original classes (Book, BookItem, Catalog) are kept;
//  a tiny built-in web server was added so a browser can use them.
// ============================================================
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define CLOSESOCK closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  typedef int sock_t;
  #define CLOSESOCK close
#endif

using namespace std;

// Settings can be changed with environment variables (used by Docker / Kubernetes).
// Defaults keep the original behaviour: port 8080, reachable only from this computer.
static int envInt(const char* k, int d) { const char* v = getenv(k); return (v && *v) ? atoi(v) : d; }
static string envStr(const char* k, const string& d) { const char* v = getenv(k); return (v && *v) ? string(v) : d; }
const int    LOAN_SECS   = 30;   // DEMO MODE: due after 30 seconds (as in your original)
const int    FINE_PER_S  = 5;    // Rs. 5 per second late
const string CSV_FILE    = "books.csv";
const string WEB_DIR     = "web";
string APP_VERSION       = "dev";   // replaced at start-up from the APP_VERSION variable

// ---------- small helpers ----------
string jsonEscape(const string& s) {
    string o;
    for (unsigned char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else if (c == '\r') {}
        else if (c < 0x20) o += ' ';
        else o += (char)c;
    }
    return o;
}
string trim(string s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
string urlDecode(const string& s) {
    string o;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '+') o += ' ';
        else if (s[i] == '%' && i + 2 < s.size()) { o += (char)strtol(s.substr(i + 1, 2).c_str(), nullptr, 16); i += 2; }
        else o += s[i];
    }
    return o;
}
// split one CSV line, honouring "quoted,fields"
vector<string> splitCSV(const string& line) {
    vector<string> out; string cur; bool q = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (q) {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; i++; }
            else if (c == '"') q = false;
            else cur += c;
        } else if (c == '"') q = true;
        else if (c == ',') { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}
string csvQuote(const string& s) {
    if (s.find_first_of(",\"\n") == string::npos) return s;
    string o = "\"";
    for (char c : s) { if (c == '"') o += "\"\""; else o += c; }
    return o + "\"";
}

// ==========================================
// 1. ENTITY: Book (Metadata) & BookItem (Physical)
// ==========================================
class Book {
protected:
    string title, author, category;
public:
    Book(string t, string a, string c) : title(t), author(a), category(c) {}
};

class BookItem : public Book {
private:
    int bid;
    string status;
    time_t issueDate, dueDate;
public:
    BookItem(int id, string t, string a, string cat, string s)
        : Book(t, a, cat), bid(id), status(s), issueDate(0), dueDate(0) {}

    int getBid() const { return bid; }
    string getTitle() const { return title; }
    string getAuthor() const { return author; }
    string getCategory() const { return category; }
    string getStatus() const { return status; }
    time_t getDueDate() const { return dueDate; }

    void setIssueRecord(time_t now) { status = "issued"; issueDate = now; dueDate = now + LOAN_SECS; }
    void setReturnRecord() { status = "available"; issueDate = 0; dueDate = 0; }

    string toJson() const {
        ostringstream o;
        o << "{\"bid\":" << bid << ",\"title\":\"" << jsonEscape(title) << "\",\"author\":\"" << jsonEscape(author)
          << "\",\"category\":\"" << jsonEscape(category) << "\",\"status\":\"" << status
          << "\",\"issueTime\":" << (long long)issueDate << ",\"dueTime\":" << (long long)dueDate << "}";
        return o.str();
    }
};

// ==========================================
// 2. CATALOG ENGINE (CSV + issue/return/fines)
// ==========================================
class Catalog {
    vector<BookItem> inventory;
public:
    void loadFromCSV(const string& filename) {
        ifstream file(filename);
        if (!file) { cerr << "Could not open " << filename << "\n"; return; }
        string line;
        getline(file, line); // header
        while (getline(file, line)) {
            if (trim(line).empty()) continue;
            vector<string> f = splitCSV(line);
            if (f.size() < 5) continue;
            try { inventory.push_back(BookItem(stoi(trim(f[0])), trim(f[1]), trim(f[2]), trim(f[3]), trim(f[4]))); }
            catch (...) {}
        }
    }
    void saveToCSV(const string& filename) {
        ofstream f(filename);
        f << "bid,title,author,category,status\n";
        for (auto& b : inventory)
            f << b.getBid() << "," << csvQuote(b.getTitle()) << "," << csvQuote(b.getAuthor()) << ","
              << csvQuote(b.getCategory()) << "," << b.getStatus() << "\n";
    }
    string allJson() const {
        string o = "{\"now\":" + to_string((long long)time(0)) + ",\"loanSeconds\":" + to_string(LOAN_SECS) + ",\"books\":[";
        for (size_t i = 0; i < inventory.size(); i++) { if (i) o += ","; o += inventory[i].toJson(); }
        return o + "]}";
    }
    string issueBook(int id) {
        for (auto& item : inventory) if (item.getBid() == id) {
            if (item.getStatus() != "available")
                return "{\"ok\":false,\"message\":\"This book is already issued.\"}";
            item.setIssueRecord(time(0));
            saveToCSV(CSV_FILE);
            return "{\"ok\":true,\"message\":\"Book issued. Due in " + to_string(LOAN_SECS) + " seconds!\",\"dueTime\":" + to_string((long long)item.getDueDate()) + "}";
        }
        return "{\"ok\":false,\"message\":\"Invalid book ID.\"}";
    }
    string returnBook(int id) {
        for (auto& item : inventory) if (item.getBid() == id) {
            if (item.getStatus() != "issued")
                return "{\"ok\":false,\"message\":\"This book was not issued.\"}";
            time_t now = time(0);
            long late = 0;
            // dueDate==0 means it was already issued in the CSV before this run: no timeline, so no fine
            if (item.getDueDate() != 0 && now > item.getDueDate()) late = (long)difftime(now, item.getDueDate());
            item.setReturnRecord();
            saveToCSV(CSV_FILE);
            return "{\"ok\":true,\"message\":\"Book returned.\",\"lateSeconds\":" + to_string(late) +
                   ",\"fine\":" + to_string(late * FINE_PER_S) + "}";
        }
        return "{\"ok\":false,\"message\":\"Invalid book ID.\"}";
    }
    string addBook(const string& t, const string& a, const string& c) {
        if (t.empty()) return "{\"ok\":false,\"message\":\"Title is required.\"}";
        int next = 1;
        for (auto& b : inventory) if (b.getBid() >= next) next = b.getBid() + 1;
        inventory.push_back(BookItem(next, t, a.empty() ? "Unknown" : a, c.empty() ? "General" : c, "available"));
        saveToCSV(CSV_FILE);
        return "{\"ok\":true,\"message\":\"Book added with ID " + to_string(next) + ".\"}";
    }
};

// ==========================================
// 3. TINY WEB SERVER
// ==========================================
string readFile(const string& path, bool& ok) {
    ifstream f(path, ios::binary);
    if (!f) { ok = false; return ""; }
    ostringstream ss; ss << f.rdbuf(); ok = true; return ss.str();
}
string mimeOf(const string& p) {
    auto ends = [&](const char* e) { size_t n = strlen(e); return p.size() >= n && p.compare(p.size() - n, n, e) == 0; };
    if (ends(".html")) return "text/html; charset=utf-8";
    if (ends(".css")) return "text/css";
    if (ends(".js")) return "application/javascript";
    if (ends(".svg")) return "image/svg+xml";
    return "application/octet-stream";
}
void sendAll(sock_t s, const string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        int n = send(s, data.data() + sent, (int)(data.size() - sent), 0);
        if (n <= 0) return;
        sent += n;
    }
}
void respond(sock_t s, int code, const string& type, const string& body) {
    string st = code == 200 ? "OK" : code == 404 ? "Not Found" : code == 405 ? "Method Not Allowed" : "Bad Request";
    sendAll(s, "HTTP/1.1 " + to_string(code) + " " + st + "\r\nContent-Type: " + type +
               "\r\nContent-Length: " + to_string(body.size()) + "\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n" + body);
}
string queryParam(const string& query, const string& key) {
    size_t pos = 0;
    while (pos <= query.size()) {
        size_t amp = query.find('&', pos);
        string pair = query.substr(pos, amp == string::npos ? string::npos : amp - pos);
        size_t eq = pair.find('=');
        if (eq != string::npos && pair.substr(0, eq) == key) return urlDecode(pair.substr(eq + 1));
        if (amp == string::npos) break;
        pos = amp + 1;
    }
    return "";
}

void handle(sock_t c, Catalog& lib) {
    string req; char buf[4096];
    while (req.find("\r\n\r\n") == string::npos && req.size() < 65536) {
        int n = recv(c, buf, sizeof(buf), 0);
        if (n <= 0) return;
        req.append(buf, n);
    }
    istringstream first(req.substr(0, req.find("\r\n")));
    string method, target, ver; first >> method >> target >> ver;
    string path = target, query;
    size_t q = target.find('?');
    if (q != string::npos) { path = target.substr(0, q); query = target.substr(q + 1); }

    const string J = "application/json; charset=utf-8";
    if (path == "/healthz" && method == "GET") { respond(c, 200, "text/plain", "ok"); return; }
    if (path == "/api/version" && method == "GET") { respond(c, 200, J, "{\"version\":\"" + jsonEscape(APP_VERSION) + "\"}"); return; }
    if (path == "/api/books" && method == "GET") { respond(c, 200, J, lib.allJson()); return; }
    if (path == "/api/issue" && method == "POST") { respond(c, 200, J, lib.issueBook(atoi(queryParam(query, "bid").c_str()))); return; }
    if (path == "/api/return" && method == "POST") { respond(c, 200, J, lib.returnBook(atoi(queryParam(query, "bid").c_str()))); return; }
    if (path == "/api/add" && method == "POST") {
        respond(c, 200, J, lib.addBook(trim(queryParam(query, "title")), trim(queryParam(query, "author")), trim(queryParam(query, "category")))); return;
    }
    if (path.rfind("/api/", 0) == 0) { respond(c, 404, J, "{\"ok\":false,\"message\":\"Unknown API\"}"); return; }

    if (method != "GET") { respond(c, 405, "text/plain", "Method not allowed"); return; }
    if (path == "/") path = "/index.html";
    if (path.find("..") != string::npos) { respond(c, 400, "text/plain", "Bad path"); return; }
    bool ok; string body = readFile(WEB_DIR + path, ok);
    if (!ok) { respond(c, 404, "text/plain", "404 - file not found"); return; }
    respond(c, 200, mimeOf(path), body);
}

int main() {
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    const int PORT = envInt("PORT", 8080);
    const string BIND = envStr("BIND_ADDR", "127.0.0.1");   // Docker sets 0.0.0.0
    APP_VERSION = envStr("APP_VERSION", "dev");
    Catalog lib;
    lib.loadFromCSV(CSV_FILE);

    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
    sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = (BIND == "0.0.0.0") ? htonl(INADDR_ANY) : htonl(INADDR_LOOPBACK);   // 127.0.0.1 = only this computer
    addr.sin_port = htons(PORT);
    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) < 0) { cerr << "Port " << PORT << " is busy. Close the other copy of this program.\n"; return 1; }
    listen(srv, 16);
    cout << "Library server running (version " << APP_VERSION << ").\nOpen this in your browser:  http://localhost:" << PORT << "\nPress Ctrl+C to stop." << endl;
    while (true) {
        sock_t c = accept(srv, nullptr, nullptr);
        if (c < 0) continue;
        handle(c, lib);
        CLOSESOCK(c);
    }
}
