#include <iostream>
#include <string>
#include <fstream>
#include <limits>
#define NOMINMAX
#include <Windows.h>
#include <sql.h>
#include <sqlext.h>
#include <vector>
#ifdef UNICODE
#define SQLGetDiagRec SQLGetDiagRecW
#else
#define SQLGetDiagRec SQLGetDiagRecA
#endif

void showError(SQLSMALLINT handleType, SQLHANDLE handle) {
#ifdef UNICODE
    SQLWCHAR sqlState[6], message[1024];
#else
    SQLCHAR sqlState[6], message[1024];
#endif
    SQLINTEGER nativeError;
    SQLSMALLINT textLength;
    SQLRETURN ret;
    int i = 1;
    while ((ret = SQLGetDiagRec(handleType, handle, i,
        sqlState, &nativeError, message, sizeof(message) / sizeof(message[0]), &textLength)) == SQL_SUCCESS) {
#ifdef UNICODE
        std::wcout << L"State: " << sqlState << L", Message: " << message << L" (Native: " << nativeError << L")" << std::endl;
#else
        std::cout << "State: " << sqlState << ", Message: " << message << " (Native: " << nativeError << ")" << std::endl;
#endif
        i++;
    }
}
using namespace std;
//----------------------------------------------------
struct partNode {
    int part_id;
    string part_name;
    string position;
    string company_name;
    string model;
    int made_year;
    string chassis_number;
    partNode* next;
};
//-----------------------------------------------
class DatabaseManager {
private:
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    bool connected;

public:
    DatabaseManager() : env(nullptr), dbc(nullptr), stmt(nullptr), connected(false) {}

    ~DatabaseManager() {
        disconnect();
    }

    bool connect(const string&, const string&, const string&, const string&) {
        if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env) != SQL_SUCCESS) {
            return false;
            
        }
         if (SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0) != SQL_SUCCESS) {
            SQLFreeHandle(SQL_HANDLE_ENV, env);
            return false;
            
        }
         if (SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc) != SQL_SUCCESS) {
            SQLFreeHandle(SQL_HANDLE_ENV, env);
            return false;
            
        }
         string connStr = "DSN=dd;UID=ADEL;PWD=121212;";
        SQLTCHAR retConnStr[1024];
        SQLSMALLINT retConnStrLen;
        #ifdef UNICODE
             std::wstring wconnStr(connStr.begin(), connStr.end());
        SQLRETURN ret = SQLDriverConnect(dbc, NULL, (SQLWCHAR*)wconnStr.c_str(), SQL_NTS,
            retConnStr, sizeof(retConnStr) / sizeof(SQLWCHAR), &retConnStrLen, SQL_DRIVER_NOPROMPT);
        #else
             SQLRETURN ret = SQLDriverConnect(dbc, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS,
                retConnStr, sizeof(retConnStr), &retConnStrLen, SQL_DRIVER_NOPROMPT);
        #endif
             if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            showError(SQL_HANDLE_DBC, dbc);
            disconnect();
            return false;
            
        }
         connected = true;
        return true;
        }

    void disconnect() {
        if (stmt) {
            SQLFreeHandle(SQL_HANDLE_STMT, stmt);
            stmt = nullptr;
        }
        if (dbc) {
            SQLDisconnect(dbc);
            SQLFreeHandle(SQL_HANDLE_DBC, dbc);
            dbc = nullptr;
        }
        if (env) {
            SQLFreeHandle(SQL_HANDLE_ENV, env);
            env = nullptr;
        }
        connected = false;
    }

    bool isConnected() const {
        return connected;
    }

    void showError(SQLSMALLINT handleType, SQLHANDLE handle) {
        SQLTCHAR sqlState[6], message[1024];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLRETURN ret;

        cout << "\033[31mODBC Error:\033[0m" << endl;

        int i = 1;
        while ((ret = SQLGetDiagRec(handleType, handle, i, sqlState, &nativeError,
            message, sizeof(message), &textLength)) == SQL_SUCCESS ||
            ret == SQL_SUCCESS_WITH_INFO) {
            cout << "  State: " << sqlState << ", Error: " << message << " (Native: " << nativeError << ")" << endl;
            i++;
        }
    }

    bool executeQuery(const string& query) {
        if (!connected) return false;
        if (SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt) != SQL_SUCCESS) {
            showError(SQL_HANDLE_DBC, dbc);
            return false;
            
        }
         #ifdef UNICODE
             std::wstring wquery(query.begin(), query.end());
        if (SQLExecDirect(stmt, (SQLWCHAR*)wquery.c_str(), SQL_NTS) != SQL_SUCCESS) {
            #else
                 if (SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS) != SQL_SUCCESS) {
                #endif
                    showError(SQL_HANDLE_STMT, stmt);
                SQLFreeHandle(SQL_HANDLE_STMT, stmt);
                stmt = nullptr;
                return false;
                
            }
             return true;
            
        }


    bool fetchResults(vector<vector<string>>& results) {
        if (!stmt) return false;

        SQLSMALLINT numCols;
        SQLNumResultCols(stmt, &numCols);

        while (SQLFetch(stmt) == SQL_SUCCESS) {
            vector<string> row;
            for (SQLSMALLINT i = 1; i <= numCols; i++) {
                SQLTCHAR colData[256];
                SQLLEN indicator;

                if (SQLGetData(stmt, i, SQL_C_CHAR, colData, sizeof(colData), &indicator) == SQL_SUCCESS) {
                    if (indicator == SQL_NULL_DATA) {
                        row.push_back("NULL");
                    }
                    else {
                        row.push_back(string((char*)colData));
                    }
                }
                else {
                    row.push_back("");
                }
            }
            results.push_back(row);
        }

        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        stmt = nullptr;

        return !results.empty();
    }

    bool createPartsTable() {
        string query = "IF NOT EXISTS (SELECT * FROM sysobjects WHERE name='Parts' AND xtype='U') "
            "CREATE TABLE Parts ("
            "PartID INT PRIMARY KEY, "
            "PartName NVARCHAR(100) NOT NULL, "
            "Position NVARCHAR(100), "
            "CompanyName NVARCHAR(100), "
            "Model NVARCHAR(50), "
            "MadeYear INT, "
            "ChassisNumber NVARCHAR(50)"
            ")";
        return executeQuery(query);
    }
};
//-----------------------------------------------
class partLinkedList {
private:
    partNode* head;
    DatabaseManager db;

public:
    partLinkedList() : head(nullptr) {
        if (!db.connect("localhost", "parts2", "ADEL", "121212")) {
            cout << "\033[33mWarning: Could not connect to database. Using local storage only.\033[0m\n";
            
        }
         else {
            db.createPartsTable();
            loadFromDatabase();
            
        }
        
    }

    ~partLinkedList() {
        partNode* current = head;
        while (current) {
            partNode* nextNode = current->next;
            delete current;
            current = nextNode;
        }
    }
    bool isDatabaseConnected() {
        return db.isConnected();
    }
    void loadFromDatabase() {
        if (!db.isConnected()) return;

        string query = "SELECT PartID, PartName, Position, CompanyName, Model, MadeYear, ChassisNumber FROM Parts";
        if (db.executeQuery(query)) {
            vector<vector<string>> results;
            if (db.fetchResults(results)) {
                for (const auto& row : results) {
                    addPart(stoi(row[0]), row[1], row[2], row[3], row[4],
                        stoi(row[5]), row[6], false); // false = don't save to DB yet
                }
            }
        }
    }
    bool isUniqueID(int id) {
        partNode* current = head;
        while (current) {
            if (current->part_id == id) return false;
            current = current->next;
        }
        return true;
    }
    void addPart(int part_id, const string& part_name, const string& position,
        const string& company_name, const string& model,
        int made_year, const string& chassis_number, bool saveToDB = true) {
        if (!isUniqueID(part_id)) {
            cout << "\033[33mError: Part ID already exists.\033[0m\n";
            return;
        }

        partNode* newNode = new partNode{ part_id, part_name, position, company_name,
                                         model, made_year, chassis_number, nullptr };

        if (!head) {
            head = newNode;
        }
        else {
            partNode* current = head;
            while (current->next) {
                current = current->next;
            }
            current->next = newNode;
        }

        if (saveToDB && db.isConnected()) {
            string query = "INSERT INTO Parts VALUES (" +
                to_string(part_id) + ", '" +
                escapeSQL(part_name) + "', '" +
                escapeSQL(position) + "', '" +
                escapeSQL(company_name) + "', '" +
                escapeSQL(model) + "', " +
                to_string(made_year) + ", '" +
                escapeSQL(chassis_number) + "')";

            if (!db.executeQuery(query)) {
                cout << "\033[33mWarning: Part added locally but failed to save to database.\033[0m\n";
            }
        }

        cout << "\033[32mPart added successfully.\033[0m\n";
    }

    void deletePart(const string& searchTerm) {
        partNode* current = head, * previous = nullptr;
        int id;

        try { id = stoi(searchTerm); }
        catch (...) { id = -1; }

        while (current) {
            if ((id != -1 && current->part_id == id) || (id == -1 && current->part_name == searchTerm)) {
                if (!previous) head = current->next;
                else previous->next = current->next;

                if (db.isConnected()) {
                    string query = "DELETE FROM Parts WHERE " +
                        (id != -1 ? "PartID = " + to_string(id) :
                            "PartName = '" + escapeSQL(searchTerm) + "'");

                    if (!db.executeQuery(query)) {
                        cout << "\033[33mWarning: Part deleted locally but failed to delete from database.\033[0m\n";
                    }
                }

                delete current;
                cout << "\033[31mPart deleted successfully.\033[0m\n";
                return;
            }
            previous = current;
            current = current->next;
        }

        cout << "\033[33mPart not found.\033[0m\n";
    }

    void updatePart(const string& searchTerm) {
        partNode* current = head;
        int id;

        try { id = stoi(searchTerm); }
        catch (...) { id = -1; }

        while (current) {
            if ((id != -1 && current->part_id == id) || (id == -1 && current->part_name == searchTerm)) {

                cout << "1. Position in car\n2. Company Name\n3. Model\n4. Made Year\n5. Chassis Number\n6. Modify All\n7. Cancel\n";
                int choice;
                while (!(cin >> choice) || choice < 1 || choice > 7) {
                    cout << "\033[33mInvalid input. Choose 1-7.\033[0m\n";
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
                cin.ignore();

                switch (choice) {
                case 1:
                    cout << "Enter new position in car: ";
                    getline(cin, current->position);
                    break;
                case 2:
                    cout << "Enter new company name: ";
                    getline(cin, current->company_name);
                    break;
                case 3:
                    cout << "Enter new model: ";
                    getline(cin, current->model);
                    break;
                case 4:
                    cout << "Enter new made year: ";
                    cin >> current->made_year;
                    break;
                case 5:
                    cout << "Enter new chassis number: ";
                    cin >> current->chassis_number;
                    break;
                case 6:
                    cout << "Enter new position in car: ";
                    getline(cin, current->position);
                    cout << "Enter new company name: ";
                    getline(cin, current->company_name);
                    cout << "Enter new model: ";
                    getline(cin, current->model);
                    cout << "Enter new made year: ";
                    cin >> current->made_year;
                    cin.ignore();
                    cout << "Enter new chassis number: ";
                    getline(cin, current->chassis_number);
                    break;
                case 7:
                    cout << "\033[33mUpdate canceled.\033[0m\n";
                    return;
                }
                if (db.isConnected()) {
                    string query = string("UPDATE Parts SET ") +
                        "PartName = '" + escapeSQL(current->part_name) + "', " +
                        "Position = '" + escapeSQL(current->position) + "', " +
                        "CompanyName = '" + escapeSQL(current->company_name) + "', " +
                        "Model = '" + escapeSQL(current->model) + "', " +
                        "MadeYear = " + to_string(current->made_year) + ", " +
                        "ChassisNumber = '" + escapeSQL(current->chassis_number) + "' " +
                        "WHERE PartID = " + to_string(current->part_id);

                    if (!db.executeQuery(query)) {
                        cout << "\033[33mWarning: Part updated locally but failed to update in database.\033[0m\n";
                    }
                }

                cout << "\033[32mPart updated successfully.\033[0m\n";
                return;
            }
            current = current->next;
        }

        cout << "\033[33mPart not found.\033[0m\n";
    }

    string escapeSQL(const string& input) {
        string output;
        for (char c : input) {
            if (c == '\'') output += '\'';
            output += c;
        }
        return output;
    }

    void searchPart(const string& searchTerm) {
        partNode* current = head;
        int id;

        try { id = stoi(searchTerm); }
        catch (...) { id = -1; }

        while (current) {
            if ((id != -1 && current->part_id == id) || (id == -1 && current->part_name == searchTerm)) {
                printPart(current);
                return;
            }
            current = current->next;
        }

        cout << "\033[33mPart not found.\033[0m\n";
    }

    void printPart(partNode* p) {
        cout << "ID: " << p->part_id
            << ", Name: " << p->part_name
            << ", Position in Car: " << p->position
            << ", Company: " << p->company_name
            << ", Model: " << p->model
            << ", Year: " << p->made_year
            << ", Chassis No: " << p->chassis_number << endl;
    }

    void displayAll() {
        if (!head) {
            cout << "\033[33mNo parts available.\033[0m\n";
            return;
        }

        partNode* current = head;
        while (current) {
            printPart(current);
            current = current->next;
        }
    }

    void saveToFile(const string& filename) {
        ofstream out(filename);
        if (!out) {
            cout << "\033[31mError opening file for writing.\033[0m\n";
            return;
        }

        partNode* current = head;
        while (current) {
            out << current->part_id << "," << current->part_name << "," << current->position << ","
                << current->company_name << "," << current->model << "," << current->made_year << ","
                << current->chassis_number << endl;
            current = current->next;
        }

        out.close();
        cout << "\033[32mData saved to file.\033[0m\n";
    }

    void loadFromFile(const string& filename) {
        ifstream in(filename);
        if (!in) {
            cout << "\033[31mFile not found. Starting with an empty list.\033[0m\n";
            return;
        }

        string line;
        while (getline(in, line)) {
            partNode* newNode = new partNode;
            size_t pos = 0;
            int field = 0;

            for (int i = 0; i < 6; ++i) {
                pos = line.find(',');
                string token = line.substr(0, pos);
                switch (i) {
                case 0: newNode->part_id = stoi(token); break;
                case 1: newNode->part_name = token; break;
                case 2: newNode->position = token; break;
                case 3: newNode->company_name = token; break;
                case 4: newNode->model = token; break;
                case 5: newNode->made_year = stoi(token); break;
                }
                line.erase(0, pos + 1);
            }
            newNode->chassis_number = line;
            newNode->next = nullptr;

            if (!head) head = newNode;
            else {
                partNode* current = head;
                while (current->next) current = current->next;
                current->next = newNode;
            }
        }

        in.close();
        cout << "\033[32mData loaded from file.\033[0m\n";
    }
    //----------------------------------------------------------------------------------------------------------------------
}; int main() {
    partLinkedList parts;

    if (!parts.isDatabaseConnected()) {
        cout << "\033[33mWarning: Could not connect to the database. Using local storage only.\033[0m\n";
        
    }

    int choice;
    do {
        system("cls");
        cout << "\033[31m \t\tParts sales store\033[0m";
        cout << "\n\t_____________________________________\n";
        cout << "\n1. Add Part\n2. Delete Part\n3. Update Part\n4. Search Part\n5. Display All\n6. Save\n7. Reload Database\n8. Exit\n";
        cout << "\033[94mEnter your choice: \033[0m";
        while (!(cin >> choice) || choice < 1 || choice > 8) {
            cout << "\033[33mInvalid input. Enter a number between 1-8.\033[0m\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        cin.ignore();

        string part_name, position, company, model, chassis;
        int id, year;

        switch (choice) {
        case 1:
            system("cls");
            cout << "\033[31m \t\t Add part information\033[0m";
            cout << "\n\t_____________________________________\n";
            cout << "Enter Part ID: ";
            cin >> id; cin.ignore();
            cout << "Enter Part Name: ";
            getline(cin, part_name);
            cout << "Enter Position in Car: ";
            getline(cin, position);
            cout << "Enter Company Name: ";
            getline(cin, company);
            cout << "Enter Model: ";
            getline(cin, model);
            cout << "Enter Made Year: ";
            cin >> year; cin.ignore();
            cout << "Enter Chassis Number: ";
            getline(cin, chassis);

            parts.addPart(id, part_name, position, company, model, year, chassis);
            system("pause");
            break;

        case 2:
            system("cls");
            cout << "\033[31m \t\t Delete part information\033[0m";
            cout << "\n\t_____________________________________\n";
            cout << "Enter Part ID or Name to delete: ";
            getline(cin, part_name);
            parts.deletePart(part_name);
            system("pause");
            break;

        case 3:
            system("cls");
            cout << "\033[31m \t\t Update part information\033[0m";
            cout << "\n\t_____________________________________\n";
            cout << "Enter Part ID or Name to update: ";
            getline(cin, part_name);
            parts.updatePart(part_name);
            system("pause");
            break;

        case 4:
            system("cls");
            cout << "\033[31m \t\t Search part information\033[0m";
            cout << "\n\t_____________________________________\n";
            cout << "Enter Part ID or Name to search: ";
            getline(cin, part_name);
            parts.searchPart(part_name);
            system("pause");
            break;

        case 5:
            system("cls");
            cout << "\033[31m \t\t All parts information\033[0m";
            cout << "\n\t_____________________________________\n";
            parts.displayAll();
            system("pause");
            break;

        case 6:
            parts.saveToFile("parts_data.txt");
            cout << "\033[32mData saved successfully.\033[0m\n";
            system("pause");
            break;

        case 7:
            // Reload data from the database
            if (parts.isDatabaseConnected()) {
                parts.loadFromDatabase();
                cout << "\033[32mData reloaded from the database.\033[0m\n";
            }
            else {
                cout << "\033[33mDatabase connection is not available.\033[0m\n";
            }
            system("pause");
            break;

        case 8:
            cout << "\033[31mExiting program...\033[0m\n";
            parts.isDatabaseConnected();  // Explicitly disconnect from the database
            break;
        }
    } while (choice != 8);

    return 0;
}
