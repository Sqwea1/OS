#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>
#include "employee.h"

static const int RECORD_COUNT_MIN = 1;
static const int NAME_MAX_CHARS   = 9;

static std::string getLastErrorMessage(DWORD errorCode) {
    return "Windows error code: " + std::to_string(errorCode);
}

static Employee readEmployeeFromConsole(int recordNumber) {
    Employee emp = {};

    std::cout << "\n--- Record " << recordNumber << " ---\n";

    std::cout << "  Employee ID: ";
    std::cin >> emp.num;
    if (std::cin.fail()) {
        throw std::runtime_error("Invalid employee ID input.");
    }

    std::cout << "  Name (max " << NAME_MAX_CHARS << " chars): ";
    std::cin >> emp.name;
    emp.name[NAME_MAX_CHARS] = '\0';

    std::cout << "  Hours worked: ";
    std::cin >> emp.hours;
    if (std::cin.fail()) {
        throw std::runtime_error("Invalid hours input.");
    }

    return emp;
}

static void writeEmployeeToFile(HANDLE hFile, const Employee& emp) {
    DWORD bytesWritten = 0;
    BOOL  result       = WriteFile(hFile, &emp, sizeof(Employee), &bytesWritten, nullptr);

    if (FALSE == result || bytesWritten != sizeof(Employee)) {
        throw std::runtime_error(
            "Failed to write record. " + getLastErrorMessage(GetLastError())
        );
    }
}

static void createBinaryFile(const char* filename, int recordCount) {
    HANDLE hFile = CreateFileA(
        filename, GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr
    );

    if (INVALID_HANDLE_VALUE == hFile) {
        throw std::runtime_error(
            "Failed to create file '" + std::string(filename) + "'. " +
            getLastErrorMessage(GetLastError())
        );
    }

    try {
        for (int i = 0; i < recordCount; ++i) {
            Employee emp = readEmployeeFromConsole(i + 1);
            writeEmployeeToFile(hFile, emp);
        }
    }
    catch (...) {
        CloseHandle(hFile);
        throw;
    }

    CloseHandle(hFile);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: Creator.exe <filename> <record_count>\n";
        return 1;
    }

    const char* filename    = argv[1];
    int         recordCount = 0;

    try {
        recordCount = std::stoi(argv[2]);
    }
    catch (const std::exception&) {
        std::cerr << "Error: <record_count> must be an integer.\n";
        return 1;
    }

    if (recordCount < RECORD_COUNT_MIN) {
        std::cerr << "Error: record count must be at least " << RECORD_COUNT_MIN << ".\n";
        return 1;
    }

    try {
        createBinaryFile(filename, recordCount);
        std::cout << "\nFile '" << filename << "' created successfully ("
                  << recordCount << " record(s)).\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Creator error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
