#pragma once
#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <queue>

#define DRAGON_DEVICE 0x8000
#define IOCTL_ATTACH CTL_CODE(DRAGON_DEVICE, 0x4452, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_READ CTL_CODE(DRAGON_DEVICE, 0x4453, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_MODULE_BASE CTL_CODE(DRAGON_DEVICE, 0x4454, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_PID CTL_CODE(DRAGON_DEVICE, 0x4455, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_BATCH_READ CTL_CODE(DRAGON_DEVICE, 0x4456, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_WRITE CTL_CODE(DRAGON_DEVICE, 0x4457, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Cache structure for reducing memory reads
struct CacheEntry {
    std::vector<uint8_t> data;
    DWORD64 timestamp;
};

// Position tracking for smooth ESP
struct PositionFrame {
    float x, y, z;
    DWORD64 timestamp;
};

struct EntityCache {
    PositionFrame positions[10];  // History of last 10 frames
    int frameIndex = 0;
    float smoothX = 0.0f, smoothY = 0.0f, smoothZ = 0.0f;
};

class MemoryMgr
{
public:
    MemoryMgr();
    ~MemoryMgr();
    bool ConnectDriver(const LPCWSTR name);
    bool DisconnectDriver();
    bool Attach(const DWORD pid);
    DWORD64 GetModuleBase(const wchar_t* moduleName);
    DWORD GetProcessID(const wchar_t* processName);
    //DWORD64 TraceAddress(DWORD64 baseAddress, std::vector<DWORD> offsets);
    bool BatchReadMemory(const std::vector<std::pair<DWORD64, SIZE_T>>& requests, void* output_buffer);
    
    // NEW: Priority reading for ESP data (smooth positions)
    bool ReadESPData(DWORD64 address, float& outX, float& outY, float& outZ, DWORD64 entityId);
    
    // NEW: Smooth position interpolation
    void UpdatePositionSmoothing(DWORD64 entityId, float x, float y, float z);
    void GetSmoothedPosition(DWORD64 entityId, float& outX, float& outY, float& outZ);

    template <typename ReadType>
    bool ReadMemory(DWORD64 address, ReadType& value, SIZE_T size = sizeof(ReadType))
    {
        // Try kernel mode first
        if (kernelDriver != nullptr && ProcessID != 0)
        {
            if (address == 0 || address >= 0x7FFFFFFFFFFF || size == 0 || size > 0x1000) {
                return false;
            }

            if (address + size < address) {
                return false;
            }

            Request readRequest;
            readRequest.process_id = ULongToHandle(ProcessID);
            readRequest.target = reinterpret_cast<PVOID>(address);
            readRequest.buffer = &value;
            readRequest.size = size;

            BOOL result = DeviceIoControl(kernelDriver,
                IOCTL_READ,
                &readRequest,
                sizeof(readRequest),
                &readRequest,
                sizeof(readRequest),
                nullptr,
                nullptr);
            return result == TRUE;
        }
        // Fallback to user-mode reading with cache
        else if (useUserModeOnly && hProcess != nullptr && ProcessID != 0)
        {
            if (address == 0 || size == 0 || size > 0x1000) {
                return false;
            }
            
            // Try to get from cache first
            {
                std::lock_guard<std::mutex> lock(cacheMutex);
                auto it = readCache.find(address);
                if (it != readCache.end() && (GetTickCount64() - it->second.timestamp) < 5) // 5ms cache
                {
                    if (it->second.data.size() >= size)
                    {
                        memcpy(&value, it->second.data.data(), size);
                        return true;
                    }
                }
            }

            // Cache miss - read from process
            SIZE_T bytesRead = 0;
            BOOL result = ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), &value, size, &bytesRead);
            
            if (result && bytesRead == size)
            {
                // Update cache
                {
                    std::lock_guard<std::mutex> lock(cacheMutex);
                    CacheEntry entry;
                    entry.data.resize(size);
                    memcpy(entry.data.data(), &value, size);
                    entry.timestamp = GetTickCount64();
                    readCache[address] = entry;
                }
                return true;
            }
            return false;
        }
        return false;
    }

    template<typename T>
    bool BatchReadStructured(const std::vector<DWORD64>& addresses, std::vector<T>& results) {
        if (addresses.empty()) return false;

        std::vector<std::pair<DWORD64, SIZE_T>> requests;
        requests.reserve(addresses.size());

        for (DWORD64 addr : addresses) {
            requests.emplace_back(addr, sizeof(T));
        }

        results.resize(addresses.size());
        return BatchReadMemory(requests, results.data());
    }

    // Note: WriteMemory removed for external cheat compatibility
    // External cheats cannot write to target process memory

private:
    DWORD ProcessID = 0;
    HANDLE kernelDriver = nullptr;
    HANDLE hProcess = nullptr;
    bool useUserModeOnly = true;  // NEW: Default to user-mode only
    
    // Performance optimization: cache system
    std::unordered_map<DWORD64, CacheEntry> readCache;
    std::mutex cacheMutex;
    
    // NEW: Entity position cache for smooth ESP
    std::unordered_map<DWORD64, EntityCache> entityCache;
    std::mutex entityCacheMutex;
    
    // Performance optimization: batch reading to reduce lag
    static constexpr SIZE_T BATCH_SIZE = 1024 * 4;  // 4KB batches
    static constexpr int MAX_RETRIES = 1;  // Reduce retries for speed

    typedef struct _Request
    {
        HANDLE process_id;
        PVOID target;
        PVOID buffer;
        SIZE_T size;
    } Request, * PRequest;

    typedef struct _PID_PACK
    {
        UINT32 pid;
        WCHAR name[1024];
    } PID_PACK, * P_PID_PACK;

    typedef struct _MODULE_PACK {
        UINT32 pid;
        UINT64 baseAddress;
        SIZE_T size;
        WCHAR moduleName[1024];
    } MODULE_PACK, * P_MODULE_PACK;


    // Batch read structures
    struct BatchReadRequest {
        DWORD64 address;
        SIZE_T size;
        SIZE_T offset_in_buffer; // Offset where this read's data starts in the output buffer
    };

    struct BatchReadHeader {
        HANDLE process_id;
        UINT32 num_requests;
        SIZE_T total_buffer_size;
        // Followed by BatchReadRequest array, then output buffer
    };
};

inline MemoryMgr memoryManager;