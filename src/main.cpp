#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

static const int   ARRAY_SIZE_MIN        = 1;
static const int   THREAD_COUNT_MIN      = 1;
static const int   THREAD_COUNT_MAX      = 60;
static const DWORD MARKER_SLEEP_MS       = 5;
static const DWORD WAIT_INFINITE_TIMEOUT = INFINITE;

struct SharedData {
    int*             arr;
    int              arrSize;
    int              threadCount;
    CRITICAL_SECTION cs;
    HANDLE*          cannotContinueEvents;
    HANDLE*          resumeEvents;
    HANDLE*          terminateEvents;
    HANDLE           startEvent;
};

struct MarkerParam {
    SharedData* data;
    int         threadIndex;
};

static std::vector<HANDLE> buildActiveEventList(const SharedData& data,
                                                 const std::vector<bool>& terminated) {
    std::vector<HANDLE> handles;
    for (int i = 0; i < data.threadCount; ++i) {
        if (!terminated[static_cast<size_t>(i)]) {
            handles.push_back(data.cannotContinueEvents[i]);
        }
    }
    return handles;
}

static void printArray(const SharedData& data) {
    std::cout << "Array: [ ";
    for (int i = 0; i < data.arrSize; ++i) {
        std::cout << data.arr[i];
        if (i < data.arrSize - 1) { std::cout << ", "; }
    }
    std::cout << " ]\n";
}

static void resumeAllActive(const SharedData& data, const std::vector<bool>& terminated) {
    for (int i = 0; i < data.threadCount; ++i) {
        if (!terminated[static_cast<size_t>(i)]) {
            SetEvent(data.resumeEvents[i]);
        }
    }
}

static DWORD WINAPI markerThread(LPVOID param) {
    MarkerParam* p    = static_cast<MarkerParam*>(param);
    SharedData*  data = p->data;
    const int    idx  = p->threadIndex;

    WaitForSingleObject(data->startEvent, WAIT_INFINITE_TIMEOUT);
    srand(static_cast<unsigned int>(idx));

    int markedCount = 0;

    while (true) {
        int  cellIndex = rand() % data->arrSize;
        bool cellEmpty = false;

        EnterCriticalSection(&data->cs);
        cellEmpty = (data->arr[cellIndex] == 0);
        LeaveCriticalSection(&data->cs);

        if (cellEmpty) {
            Sleep(MARKER_SLEEP_MS);

            EnterCriticalSection(&data->cs);
            if (data->arr[cellIndex] == 0) {
                data->arr[cellIndex] = idx;
                ++markedCount;
            }
            LeaveCriticalSection(&data->cs);

            Sleep(MARKER_SLEEP_MS);
        }
        else {
            std::cout << "[Thread " << idx << "] blocked:"
                      << "  marked=" << markedCount
                      << "  blocked_at_index=" << cellIndex << "\n";

            SetEvent(data->cannotContinueEvents[idx - 1]);

            HANDLE waitSet[2] = {
                data->resumeEvents[idx - 1],
                data->terminateEvents[idx - 1]
            };
            DWORD waitResult = WaitForMultipleObjects(2, waitSet, FALSE, WAIT_INFINITE_TIMEOUT);

            if (WAIT_OBJECT_0 == waitResult) {
                continue;
            }
            else {
                EnterCriticalSection(&data->cs);
                for (int i = 0; i < data->arrSize; ++i) {
                    if (data->arr[i] == idx) { data->arr[i] = 0; }
                }
                LeaveCriticalSection(&data->cs);

                std::cout << "[Thread " << idx << "] terminated,"
                          << " cleared " << markedCount << " cell(s).\n";
                break;
            }
        }
    }

    return 0;
}

static void releaseSharedData(SharedData& data, std::vector<HANDLE>& threads) {
    for (int i = static_cast<int>(threads.size()) - 1; i >= 0; --i) {
        if (nullptr != threads[static_cast<size_t>(i)]) {
            CloseHandle(threads[static_cast<size_t>(i)]);
            threads[static_cast<size_t>(i)] = nullptr;
        }
    }

    if (nullptr != data.terminateEvents) {
        for (int i = data.threadCount - 1; i >= 0; --i) {
            if (nullptr != data.terminateEvents[i]) { CloseHandle(data.terminateEvents[i]); }
        }
        delete[] data.terminateEvents;
        data.terminateEvents = nullptr;
    }

    if (nullptr != data.resumeEvents) {
        for (int i = data.threadCount - 1; i >= 0; --i) {
            if (nullptr != data.resumeEvents[i]) { CloseHandle(data.resumeEvents[i]); }
        }
        delete[] data.resumeEvents;
        data.resumeEvents = nullptr;
    }

    if (nullptr != data.cannotContinueEvents) {
        for (int i = data.threadCount - 1; i >= 0; --i) {
            if (nullptr != data.cannotContinueEvents[i]) { CloseHandle(data.cannotContinueEvents[i]); }
        }
        delete[] data.cannotContinueEvents;
        data.cannotContinueEvents = nullptr;
    }

    if (nullptr != data.startEvent) {
        CloseHandle(data.startEvent);
        data.startEvent = nullptr;
    }

    DeleteCriticalSection(&data.cs);
    delete[] data.arr;
    data.arr = nullptr;
}

int main() {
    SharedData               data      = {};
    std::vector<HANDLE>      threads;
    std::vector<MarkerParam> params;

    try {
        int arrSize = 0;
        std::cout << "Enter array size (min " << ARRAY_SIZE_MIN << "): ";
        std::cin  >> arrSize;

        if (std::cin.fail() || arrSize < ARRAY_SIZE_MIN) {
            throw std::runtime_error(
                "Array size must be at least " + std::to_string(ARRAY_SIZE_MIN) + "."
            );
        }

        int threadCount = 0;
        std::cout << "Enter number of marker threads (1-" << THREAD_COUNT_MAX << "): ";
        std::cin  >> threadCount;

        if (std::cin.fail() || threadCount < THREAD_COUNT_MIN || threadCount > THREAD_COUNT_MAX) {
            throw std::runtime_error(
                "Thread count must be between " +
                std::to_string(THREAD_COUNT_MIN) + " and " +
                std::to_string(THREAD_COUNT_MAX) + "."
            );
        }

        data.arr         = new int[arrSize]();
        data.arrSize     = arrSize;
        data.threadCount = threadCount;

        InitializeCriticalSection(&data.cs);

        data.startEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (nullptr == data.startEvent) {
            throw std::runtime_error(
                "Failed to create start event. Error: " + std::to_string(GetLastError())
            );
        }

        data.cannotContinueEvents = new HANDLE[threadCount]();
        data.resumeEvents         = new HANDLE[threadCount]();
        data.terminateEvents      = new HANDLE[threadCount]();

        for (int i = 0; i < threadCount; ++i) {
            data.cannotContinueEvents[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            data.resumeEvents[i]         = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            data.terminateEvents[i]      = CreateEvent(nullptr, FALSE, FALSE, nullptr);

            if (nullptr == data.cannotContinueEvents[i] ||
                nullptr == data.resumeEvents[i]         ||
                nullptr == data.terminateEvents[i]) {
                throw std::runtime_error(
                    "Failed to create events for thread " + std::to_string(i + 1) +
                    ". Error: " + std::to_string(GetLastError())
                );
            }
        }

        params.resize(static_cast<size_t>(threadCount));
        threads.resize(static_cast<size_t>(threadCount), nullptr);

        for (int i = 0; i < threadCount; ++i) {
            params[static_cast<size_t>(i)].data        = &data;
            params[static_cast<size_t>(i)].threadIndex = i + 1;

            threads[static_cast<size_t>(i)] = CreateThread(
                nullptr, 0, markerThread,
                static_cast<LPVOID>(&params[static_cast<size_t>(i)]),
                0, nullptr
            );

            if (nullptr == threads[static_cast<size_t>(i)]) {
                throw std::runtime_error(
                    "Failed to create marker thread " + std::to_string(i + 1) +
                    ". Error: " + std::to_string(GetLastError())
                );
            }
        }

        SetEvent(data.startEvent);

        std::vector<bool> terminated(static_cast<size_t>(threadCount), false);
        int               remainingThreads = threadCount;

        while (remainingThreads > 0) {
            std::vector<HANDLE> activeEvents = buildActiveEventList(data, terminated);
            WaitForMultipleObjects(
                static_cast<DWORD>(activeEvents.size()),
                activeEvents.data(), TRUE, WAIT_INFINITE_TIMEOUT
            );

            std::cout << "\nAll active marker threads are blocked.\n";
            printArray(data);

            int choice = 0;
            std::cout << "Enter thread number to terminate (1-" << threadCount << "): ";
            std::cin  >> choice;

            if (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(1024, '\n');
                std::cerr << "Invalid input – resuming all active threads.\n";
                resumeAllActive(data, terminated);
                continue;
            }

            if (choice < 1 || choice > threadCount) {
                std::cerr << "Thread index out of range – resuming all.\n";
                resumeAllActive(data, terminated);
                continue;
            }

            const size_t choiceIdx = static_cast<size_t>(choice - 1);

            if (terminated[choiceIdx]) {
                std::cerr << "Thread " << choice << " is already terminated – resuming all.\n";
                resumeAllActive(data, terminated);
                continue;
            }

            SetEvent(data.terminateEvents[choiceIdx]);
            WaitForSingleObject(threads[choiceIdx], WAIT_INFINITE_TIMEOUT);

            CloseHandle(threads[choiceIdx]);
            threads[choiceIdx] = nullptr;

            terminated[choiceIdx] = true;
            --remainingThreads;

            std::cout << "\nArray after thread " << choice << " terminated:\n";
            printArray(data);

            resumeAllActive(data, terminated);
        }

        std::cout << "\nAll marker threads have been terminated. Done.\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        releaseSharedData(data, threads);
        return 1;
    }

    releaseSharedData(data, threads);
    return 0;
}
