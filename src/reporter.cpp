#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <iomanip>
#include "employee.h"

static const int    REPORT_COL_WIDTH = 15;
static const double HOURLY_RATE_MIN  = 0.01;

static std::string getLastErrorMessage(DWORD errorCode) {
    return "Windows error code: " + std::to_string(errorCode);
}

static bool compareByEmployeeId(const Employee& a, const Employee& b) {
    return a.num < b.num;
}

static double computeSalary(const Employee& emp, double hourlyRate) {
    return emp.hours * hourlyRate;
}

static std::vector<Employee> readAllEmployees(const char* filename) {
    HANDLE hFile = CreateFileA(
        filename, GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
    );

    if (INVALID_HANDLE_VALUE == hFile) {
        throw std::runtime_error(
            "Failed to open '" + std::string(filename) + "'. " +
            getLastErrorMessage(GetLastError())
        );
    }

    std::vector<Employee> employees;

    try {
        Employee emp       = {};
        DWORD    bytesRead = 0;

        while (true) {
            BOOL result = ReadFile(hFile, &emp, sizeof(Employee), &bytesRead, nullptr);

            if (FALSE == result) {
                throw std::runtime_error(
                    "Read error. " + getLastErrorMessage(GetLastError())
                );
            }
            if (0 == bytesRead) { break; }
            if (bytesRead != sizeof(Employee)) {
                throw std::runtime_error("Corrupted file: incomplete record.");
            }

            employees.push_back(emp);
        }
    }
    catch (...) {
        CloseHandle(hFile);
        throw;
    }

    CloseHandle(hFile);
    return employees;
}

static void writeReportHeader(std::ofstream& out, const char* sourceFilename) {
    out << "Report for file '" << sourceFilename << "'\n"
        << std::string(60, '=') << "\n"
        << std::left
        << std::setw(REPORT_COL_WIDTH) << "ID"
        << std::setw(REPORT_COL_WIDTH) << "Name"
        << std::setw(REPORT_COL_WIDTH) << "Hours"
        << std::setw(REPORT_COL_WIDTH) << "Salary"
        << "\n"
        << std::string(60, '-') << "\n";
}

static void writeEmployeeLine(std::ofstream& out, const Employee& emp, double hourlyRate) {
    out << std::left
        << std::setw(REPORT_COL_WIDTH) << emp.num
        << std::setw(REPORT_COL_WIDTH) << emp.name
        << std::setw(REPORT_COL_WIDTH) << std::fixed << std::setprecision(2) << emp.hours
        << std::setw(REPORT_COL_WIDTH) << computeSalary(emp, hourlyRate)
        << "\n";
}

static void generateReport(const char* reportFilename, const char* sourceFilename,
                            std::vector<Employee>& employees, double hourlyRate) {
    std::ofstream report(reportFilename);
    if (!report.is_open()) {
        throw std::runtime_error(
            "Failed to create report file '" + std::string(reportFilename) + "'."
        );
    }

    std::sort(employees.begin(), employees.end(), compareByEmployeeId);
    writeReportHeader(report, sourceFilename);

    for (const Employee& emp : employees) {
        writeEmployeeLine(report, emp, hourlyRate);
    }

    report << std::string(60, '=') << "\n"
           << "Total employees: " << employees.size() << "\n";

    if (!report.good()) {
        throw std::runtime_error("Error while writing report (disk full?).");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: Reporter.exe <binary_file> <report_file> <hourly_rate>\n";
        return 1;
    }

    const char* sourceFile = argv[1];
    const char* reportFile = argv[2];
    double      hourlyRate = 0.0;

    try {
        hourlyRate = std::stod(argv[3]);
    }
    catch (const std::exception&) {
        std::cerr << "Error: <hourly_rate> must be a number.\n";
        return 1;
    }

    if (hourlyRate < HOURLY_RATE_MIN) {
        std::cerr << "Error: hourly rate must be at least " << HOURLY_RATE_MIN << ".\n";
        return 1;
    }

    try {
        std::vector<Employee> employees = readAllEmployees(sourceFile);
        if (employees.empty()) {
            std::cerr << "Warning: source file contains no records.\n";
        }
        generateReport(reportFile, sourceFile, employees, hourlyRate);
        std::cout << "Report written to '" << reportFile << "'.\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Reporter error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
