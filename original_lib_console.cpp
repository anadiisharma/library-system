#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

using namespace std;

// Helper to show time in HH:MM:SS for the live demo
string formatTime(time_t rawTime) {
    if (rawTime == 0) return "N/A";
    struct tm *timeinfo = localtime(&rawTime);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return string(buffer);
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
    time_t issueDate;
    time_t dueDate;

public:
    BookItem(int id, string t, string a, string cat, string s) 
        : Book(t, a, cat), bid(id), status(s), issueDate(0), dueDate(0) {}

    int getBid() { return bid; }
    string getTitle() { return title; }
    string getStatus() { return status; }
    
    void setIssueRecord(time_t now) {
        status = "issued";
        issueDate = now;
        // DEMO MODE: Book is due in 30 seconds
        dueDate = now + 30; 
    }

    void setReturnRecord() {
        status = "available";
        issueDate = 0;
        dueDate = 0;
    }

    void display(bool detailed = false) {
        cout << left << setw(5) << bid 
             << setw(35) << (title.length() > 32 ? title.substr(0, 30) + ".." : title)
             << setw(15) << category
             << setw(12) << status;
        
        if (detailed && status == "issued") {
            cout << " [Issued: " << formatTime(issueDate) 
                 << " | Due: " << formatTime(dueDate) << "]";
        }
        cout << endl;
    }

    time_t getDueDate() { return dueDate; }
};

// ==========================================
// 2. ACCOUNT HIERARCHY (Abstraction)
// ==========================================
class Account {
protected:
    string name;
public:
    Account(string n) : name(n) {}
    virtual void showMenu() = 0; // Pure Virtual
};

class Librarian : public Account {
public:
    Librarian(string n) : Account(n) {}
    void showMenu() override {
        cout << "\n--- Librarian Dashboard: " << name << " ---\n";
        cout << "1. View Catalog (Check Timelines)\n2. Logout\nSelection: ";
    }
};

class Member : public Account {
public:
    Member(string n) : Account(n) {}
    void showMenu() override {
        cout << "\n--- Member Dashboard: " << name << " ---\n";
        cout << "1. View Catalog\n2. Search Book\n3. Issue Book (30s Limit)\n4. Return Book (Check Fine)\n5. Logout\nSelection: ";
    }
};

// ==========================================
// 3. CATALOG ENGINE (CSV Parsing & Fines)
// ==========================================
class Catalog {
private:
    vector<BookItem> inventory;

public:
    void loadFromCSV(string filename) {
        ifstream file(filename);
        if (!file) return;
        string line, header;
        getline(file, header); 
        while (getline(file, line)) {
            stringstream ss(line);
            string bidStr, title, author, category, status;
            if (getline(ss, bidStr, ',') && getline(ss, title, ',') && 
                getline(ss, author, ',') && getline(ss, category, ',') && getline(ss, status)) {
                inventory.push_back(BookItem(stoi(bidStr), title, author, category, status));
            }
        }
        file.close();
    }

    void showInventory(bool detailed = false) {
        cout << "\n" << string(100, '-') << endl;
        cout << left << setw(5) << "BID" << setw(35) << "TITLE" << setw(15) << "CATEGORY" << setw(12) << "STATUS" << "TIMELINE" << endl;
        cout << string(100, '-') << endl;
        for (auto& item : inventory) item.display(detailed);
    }

    void search(string query) {
        for (auto& item : inventory) {
            if (item.getTitle().find(query) != string::npos) item.display(true);
        }
    }

    void issueBook(int id) {
        for (auto& item : inventory) {
            if (item.getBid() == id && item.getStatus() == "available") {
                item.setIssueRecord(time(0));
                cout << "SUCCESS: Book Issued at " << formatTime(time(0)) 
                     << ". DUE IN 30 SECONDS!" << endl;
                return;
            }
        }
        cout << "FAILED: Invalid ID or Book Issued.\n";
    }

    void returnBook(int id) {
        for (auto& item : inventory) {
            if (item.getBid() == id && item.getStatus() == "issued") {
                time_t now = time(0);
                cout << "Returning at: " << formatTime(now) << endl;
                
                if (now > item.getDueDate()) {
                    int secondsLate = (int)difftime(now, item.getDueDate());
                    cout << "!!! OVERDUE BY " << secondsLate << " SECONDS !!!\n";
                    cout << "TOTAL FINE: Rs. " << secondsLate * 5 << " (Rs. 5 per second)\n";
                } else {
                    cout << "Returned on time. No fine.\n";
                }
                item.setReturnRecord();
                return;
            }
        }
        cout << "FAILED: This book was not issued.\n";
    }
};

// ==========================================
// 4. MAIN
// ==========================================
int main() {
    Catalog lib;
    lib.loadFromCSV("Books_data - Sheet1.csv");

    int role;
    cout << "--- DEMO MODE: 30 SECOND LIMIT ---\n";
    cout << "1. Librarian Login\n2. Member Login\nChoice: ";
    cin >> role;

    if (role == 1) {
        Librarian admin("Krish Gandhi");
        int ch;
        while(true) {
            admin.showMenu(); cin >> ch;
            if (ch == 1) lib.showInventory(true);
            else break;
        }
    } else {
        Member user("Anadi Sharma");
        int ch;
        while(true) {
            user.showMenu(); cin >> ch;
            if (ch == 1) lib.showInventory();
            else if (ch == 2) {
                string q; cout << "Search Title: "; cin.ignore(); getline(cin, q);
                lib.search(q);
            } else if (ch == 3) {
                int id; cout << "Enter BID to Issue: "; cin >> id;
                lib.issueBook(id);
            } else if (ch == 4) {
                int id; cout << "Enter BID to Return: "; cin >> id;
                lib.returnBook(id);
            } else break;
        }
    }
    return 0;
}