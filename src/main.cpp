#include <windows.h>
#include <iostream>
#include <string>
#include <stdexcept>

static const DWORD SLEEP_MIN_MAX_MS = 7;
static const DWORD SLEEP_AVERAGE_MS = 12;
static const int   ARRAY_SIZE_MIN   = 2;

struct ThreadData {
    int*   arr;
    int    size;
    int    minIndex;
    int    maxIndex;
    double average;
};

static DWORD WINAPI minMaxThread(LPVOID param) {
    ThreadData* data   = static_cast<ThreadData*>(param);
    int         minIdx = 0;
    int         maxIdx = 0;

    for (int i = 1; i < data->size; ++i) {
        Sleep(SLEEP_MIN_MAX_MS);
        if (data->arr[i] < data->arr[minIdx]) { minIdx = i; }
        if (data->arr[i] > data->arr[maxIdx]) { maxIdx = i; }
    }

    data->minIndex = minIdx;
    data->maxIndex = maxIdx;

    std::cout << "[min_max] min = " << data->arr[minIdx]
              << " (index " << minIdx << "),  max = " << data->arr[maxIdx]
              << " (index " << maxIdx << ")\n";
    return 0;
}

static DWORD WINAPI averageThread(LPVOID param) {
    ThreadData* data = static_cast<ThreadData*>(param);
    double      sum  = 0.0;

    for (int i = 0; i < data->size; ++i) {
        Sleep(SLEEP_AVERAGE_MS);
        sum += static_cast<double>(data->arr[i]);
    }

    data->average = sum / static_cast<double>(data->size);
    std::cout << "[average] average = " << data->average << "\n";
    return 0;
}

static void printArray(const char* label, const int* arr, int size) {
    std::cout << label << " [ ";
    for (int i = 0; i < size; ++i) {
        std::cout << arr[i];
        if (i < size - 1) { std::cout << ", "; }
    }
    std::cout << " ]\n";
}

int main() {
    int*   arr      = nullptr;
    HANDLE hMinMax  = nullptr;
    HANDLE hAverage = nullptr;

    try {
        int size = 0;
        std::cout << "Enter array size (min " << ARRAY_SIZE_MIN << "): ";
        std::cin >> size;

        if (std::cin.fail() || size < ARRAY_SIZE_MIN) {
            throw std::runtime_error(
                "Array size must be at least " + std::to_string(ARRAY_SIZE_MIN) + "."
            );
        }

        arr = new int[size];

        std::cout << "Enter " << size << " integers:\n";
        for (int i = 0; i < size; ++i) {
            std::cin >> arr[i];
            if (std::cin.fail()) {
                throw std::runtime_error(
                    "Invalid integer at position " + std::to_string(i + 1) + "."
                );
            }
        }

        printArray("Initial array:", arr, size);

        ThreadData data = {};
        data.arr      = arr;
        data.size     = size;
        data.minIndex = 0;
        data.maxIndex = 0;
        data.average  = 0.0;

        hMinMax = CreateThread(nullptr, 0, minMaxThread, static_cast<LPVOID>(&data), 0, nullptr);
        if (nullptr == hMinMax) {
            throw std::runtime_error(
                "Failed to create min_max thread. Error: " + std::to_string(GetLastError())
            );
        }

        hAverage = CreateThread(nullptr, 0, averageThread, static_cast<LPVOID>(&data), 0, nullptr);
        if (nullptr == hAverage) {
            throw std::runtime_error(
                "Failed to create average thread. Error: " + std::to_string(GetLastError())
            );
        }

        HANDLE handles[2] = { hMinMax, hAverage };
        WaitForMultipleObjects(2, handles, TRUE, INFINITE);

        CloseHandle(hAverage);
        hAverage = nullptr;
        CloseHandle(hMinMax);
        hMinMax = nullptr;

        int avgValue       = static_cast<int>(data.average);
        arr[data.minIndex] = avgValue;
        arr[data.maxIndex] = avgValue;

        std::cout << "\nReplaced min (index " << data.minIndex
                  << ") and max (index " << data.maxIndex
                  << ") with average = " << data.average
                  << " (truncated to int: " << avgValue << ").\n";

        printArray("Result array:  ", arr, size);
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        if (nullptr != hAverage) { CloseHandle(hAverage); }
        if (nullptr != hMinMax)  { CloseHandle(hMinMax);  }
        delete[] arr;
        return 1;
    }

    delete[] arr;
    arr = nullptr;
    return 0;
}
