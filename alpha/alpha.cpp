#include <iostream>
#include <string>
#include <fstream>
#include <limits>

using namespace std;

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

class partLinkedList {
private:
    partNode* head;

public:
    partLinkedList() : head(nullptr) {}

    ~partLinkedList() {
        partNode* current = head;
        while (current) {
            partNode* nextNode = current->next;
            delete current;
            current = nextNode;
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

    bool isValidString(const string& str) {
        return !str.empty() && str.find_first_not_of(' ') != string::npos;
    }

    void addPart(int part_id, const string& part_name, const string& position,
        const string& company_name, const string& model,
        int made_year, const string& chassis_number) {
        if (!isUniqueID(part_id)) {
            cout << "\033[33mError: Part ID already exists.\033[0m\n";
            return;
        }

        partNode* newNode = new partNode{ part_id, part_name, position, company_name, model, made_year, chassis_number, nullptr };

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

                cout << "\033[32mPart updated successfully.\033[0m\n";
                return;
            }
            current = current->next;
        }

        cout << "\033[33mPart not found.\033[0m\n";
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
};

int main() {
    partLinkedList parts;
    parts.loadFromFile("parts_data.txt");

    int choice;
    do {
        //system("cls");
        cout << "\033[31m \t\tParts sales store\033[0m";
        cout << "\n\t_____________________________________\n";
        cout << "\n1. Add Part\n2. Delete Part\n3. Update Part\n4. Search Part\n5. Display All\n6. Save\n7. Exit\n";
        cout << "\033[94mEnter your choice: \033[0m";
        while (!(cin >> choice) || choice < 1 || choice > 7) {
            cout << "\033[33mInvalid input. Enter a number between 1-7.\033[0m\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        cin.ignore();

        string part_name, position, company, model, chassis;
        int id, year;

        switch (choice) {
        case 1:
           // system("cls");
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
            //system("pause");
            break;
        case 2:
            //system("cls");
            cout << "\033[31m \t\t Add part information\033[0m";
            cout << "\n\t_____________________________________\n";
            cout << "Enter Part ID or Name to delete: ";
            getline(cin, part_name);
            parts.deletePart(part_name);
            //system("pause");
            break;
        case 3:
            //system("cls");
            cout << "\033[31m \t\t Add part information\033[0m";
            cout << "\n\t_____________________________________\n";
            cout << "Enter Part ID or Name to update: ";
            getline(cin, part_name);
            parts.updatePart(part_name);
            //system("pause");
            break;
        case 4:
            //system("cls");
            cout << "\033[31m \t\t Add part information\033[0m";
            cout << "\n\t_____________________________________\n";
            cout << "Enter Part ID or Name to search: ";
            getline(cin, part_name);
            parts.searchPart(part_name);
            break;
        case 5:
            //system("cls");
            cout << "\033[31m \t\t All parts information\033[0m";
            cout << "\n\t_____________________________________\n";
            parts.displayAll();
            //system("pause");
            break;
        case 6:
            parts.saveToFile("parts_data.txt");
            //system("pause");
            break;
        case 7:
            cout << "\033[31mExiting program...\033[0m\n";
            break;
        }
    } while (choice != 7);

    return 0;
}