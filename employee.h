#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include "employee.h"

static const DWORD  PROCESS_WAIT_MS = INFINITE;
static const int    RECORDS_MIN     = 1;
static const double HOURLY_RATE_MIN = 0.01;

static std::string getLastErrorMessage(DWORD errorCode) {
    return "Windows error code: " + std::to_string(errorCode);
}

static void launchAndWait(const std::string& commandLine) {
    STARTUPINFOA        si  = {};
    PROCESS_INFORMATION pi  = {};
    si.cb = sizeof(STARTUPINFOA);

    std::string cmd = commandLine;

    BOOL result = CreateProcessA(
        nullptr, cmd.data(), nullptr, nullptr,
        FALSE, 0, nullptr, nullptr, &si, &pi
    );

    if (FALSE == result) {
        throw std::runtime_error(
            "Failed to launch '" + commandLine + "'. " +
            getLastErrorMessage(GetLastError())
        );
    }

    WaitForSingleObject(pi.hProcess, PROCESS_WAIT_MS);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

static void printBinaryFile(const std::string& filename) {
    HANDLE hFile = CreateFileA(
        filename.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
    );

    if (INVALID_HANDLE_VALUE == hFile) {
        throw std::runtime_error(
            "Cannot open '" + filename + "'. " + getLastErrorMessage(GetLastError())
        );
    }

    std::cout << "\n=== Binary file: " << filename << " ===\n"
              << "ID\tName\t\tHours\n"
              << std::string(40, '-') << "\n";

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
            std::cout << emp.num << "\t" << emp.name << "\t\t" << emp.hours << "\n";
        }
    }
    catch (...) {
        CloseHandle(hFile);
        throw;
    }

    CloseHandle(hFile);
    std::cout << std::string(40, '=') << "\n\n";
}

static void printTextFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open report '" + filename + "'.");
    }

    std::cout << "\n=== Report: " << filename << " ===\n";
    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << "\n";
    }
    std::cout << std::string(40, '=') << "\n\n";
}

int main() {
    try {
        std::string binaryFilename;
        int         recordCount = 0;

        std::cout << "Enter binary file name: ";
        std::cin  >> binaryFilename;

        std::cout << "Enter number of records: ";
        std::cin  >> recordCount;

        if (std::cin.fail() || recordCount < RECORDS_MIN) {
            throw std::runtime_error(
                "Record count must be at least " + std::to_string(RECORDS_MIN) + "."
            );
        }

        launchAndWait("Creator.exe " + binaryFilename + " " + std::to_string(recordCount));
        printBinaryFile(binaryFilename);

        std::string reportFilename;
        double      hourlyRate = 0.0;

        std::cout << "Enter report file name: ";
        std::cin  >> reportFilename;

        std::cout << "Enter hourly rate: ";
        std::cin  >> hourlyRate;

        if (std::cin.fail() || hourlyRate < HOURLY_RATE_MIN) {
            throw std::runtime_error(
                "Hourly rate must be at least " + std::to_string(HOURLY_RATE_MIN) + "."
            );
        }

        launchAndWait(
            "Reporter.exe " + binaryFilename +
            " " + reportFilename +
            " " + std::to_string(hourlyRate)
        );
        printTextFile(reportFilename);
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
