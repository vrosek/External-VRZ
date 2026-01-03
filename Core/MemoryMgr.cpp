#include "MemoryMgr.h"
#include <TlHelp32.h>
#include <Psapi.h>

MemoryMgr::MemoryMgr()
{
    ProcessID = 0;
    kernelDriver = nullptr;
}

MemoryMgr::~MemoryMgr()
{
    DisconnectDriver();
    ProcessID = 0;
    kernelDriver = nullptr;
}

bool MemoryMgr::ConnectDriver(const LPCWSTR name)
{
    kernelDriver = CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (kernelDriver == INVALID_HANDLE_VALUE)
    {
        kernelDriver = nullptr;
        return false;
    }
    return true;
}

bool MemoryMgr::DisconnectDriver()
{
    if (kernelDriver != nullptr)
    {
        BOOL result = CloseHandle(kernelDriver);
        kernelDriver = nullptr;
        return result == TRUE;
    }
    return false;
}

bool MemoryMgr::Attach(const DWORD pid)
{
    ProcessID = pid;

    // Clear cache on reattach
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        readCache.clear();
    }

    // If kernel driver is available, use it
    if (kernelDriver != nullptr)
    {
        Request attachRequest;
        attachRequest.process_id = ULongToHandle(pid);
        attachRequest.target = nullptr;
        attachRequest.buffer = nullptr;
        attachRequest.size = 0;

        BOOL result = DeviceIoControl(kernelDriver,
            IOCTL_ATTACH,
            &attachRequest,
            sizeof(attachRequest),
            &attachRequest,
            sizeof(attachRequest),
            nullptr,
            nullptr);

        if (result == TRUE)
        {
            return true;
        }
    }

    // Fallback to user-mode process opening
    if (useUserModeOnly)
    {
        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (hProcess != nullptr)
        {
            ProcessID = pid;
            return true;
        }
    }

    return false;
}

DWORD MemoryMgr::GetProcessID(const wchar_t* processName)
{
    // If kernel driver is available, use it
    if (kernelDriver != nullptr)
    {
        PID_PACK PidPack;
        RtlZeroMemory(PidPack.name, 1024);
        wcsncpy(PidPack.name, processName, 1024);

        BOOL result = DeviceIoControl(kernelDriver,
            IOCTL_GET_PID,
            &PidPack,
            sizeof(PidPack),
            &PidPack,
            sizeof(PidPack),
            nullptr,
            nullptr);

        if (result == TRUE)
            return PidPack.pid;
    }

    // Fallback to user-mode process search using Windows API
    if (useUserModeOnly)
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
            return 0;

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32))
        {
            do {
                if (_wcsicmp(pe32.szExeFile, processName) == 0)
                {
                    CloseHandle(hSnapshot);
                    return pe32.th32ProcessID;
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);
        return 0;
    }

    return 0;
}

DWORD64 MemoryMgr::GetModuleBase(const wchar_t* moduleName)
{
    if (kernelDriver != nullptr && ProcessID != 0)
    {
        MODULE_PACK ModulePack;
        DWORD64 address = 0;
        ModulePack.pid = ProcessID;
        ModulePack.baseAddress = address;
        RtlZeroMemory(ModulePack.moduleName, 1024);
        wcsncpy(ModulePack.moduleName, moduleName, 1024);
        BOOL result = DeviceIoControl(kernelDriver,
            IOCTL_GET_MODULE_BASE,
            &ModulePack,
            sizeof(ModulePack),
            &ModulePack,
            sizeof(ModulePack),
            nullptr,
            nullptr);
        if (result == TRUE)
            return ModulePack.baseAddress;
    }

    // Fallback to user-mode module enumeration
    if (useUserModeOnly && hProcess != nullptr)
    {
        HMODULE hModules[1024];
        DWORD cbNeeded;

        if (EnumProcessModulesEx(hProcess, hModules, sizeof(hModules), &cbNeeded, LIST_MODULES_ALL))
        {
            DWORD cModules = cbNeeded / sizeof(HMODULE);

            for (DWORD i = 0; i < cModules; i++)
            {
                wchar_t szModName[MAX_PATH];
                if (GetModuleFileNameExW(hProcess, hModules[i], szModName, sizeof(szModName) / sizeof(wchar_t)))
                {
                    // Extract just the filename from the full path
                    wchar_t* pszFileName = wcsrchr(szModName, L'\\');
                    if (pszFileName == nullptr)
                        pszFileName = szModName;
                    else
                        pszFileName++; // Skip the backslash

                    if (_wcsicmp(pszFileName, moduleName) == 0)
                    {
                        return reinterpret_cast<DWORD64>(hModules[i]);
                    }
                }
            }
        }
    }

    return 0;
}

/*
DWORD64 MemoryMgr::TraceAddress(DWORD64 baseAddress, std::vector<DWORD> offsets)
{
    if (kernelDriver == nullptr || ProcessID == 0)
        return 0;

    if (baseAddress == 0 || baseAddress >= 0x7FFFFFFFFFFF)
        return 0;

    uint64_t address = baseAddress;
    if (offsets.empty())
        return baseAddress;

    uint64_t buffer = 0;
    if (!ReadMemory(address, buffer))
        return 0;

    for (size_t i = 0; i < offsets.size() - 1; i++)
    {
        if (buffer == 0 || buffer >= 0x7FFFFFFFFFFF)
            return 0;

        address = buffer + offsets[i];

        if (address < buffer)
            return 0;

        if (!ReadMemory(address, buffer))
            return 0;
    }

    if (buffer == 0 || buffer >= 0x7FFFFFFFFFFF)
        return 0;

    uint64_t finalAddress = buffer + offsets.back();
    return (finalAddress < buffer) ? 0 : finalAddress; // Check overflow
}
*/

bool MemoryMgr::BatchReadMemory(const std::vector<std::pair<DWORD64, SIZE_T>>& requests, void* output_buffer) {
    if (ProcessID == 0 || requests.empty() || output_buffer == nullptr) {
        return false;
    }

    // Kernel-mode path
    if (kernelDriver != nullptr)
    {
        // Calculate buffer size for output data only
        SIZE_T output_data_size = 0;
        for (const auto& req : requests) {
            output_data_size += req.second;
        }

        // Calculate total request structure size
        SIZE_T request_struct_size = sizeof(BatchReadHeader) +
            (requests.size() * sizeof(BatchReadRequest));

        // Total size includes both request structure and output buffer space
        SIZE_T total_buffer_size = request_struct_size + output_data_size;

        // Allocate buffer for the entire operation
        std::vector<BYTE> operation_buffer(total_buffer_size);

        BatchReadHeader* header = reinterpret_cast<BatchReadHeader*>(operation_buffer.data());
        BatchReadRequest* batch_requests = reinterpret_cast<BatchReadRequest*>(header + 1);

        // Fill header
        header->process_id = ULongToHandle(ProcessID);
        header->num_requests = static_cast<UINT32>(requests.size());
        header->total_buffer_size = output_data_size; // Size of output data only

        // Fill requests with correct offsets
        SIZE_T buffer_offset = 0;
        for (size_t i = 0; i < requests.size(); ++i) {
            batch_requests[i].address = requests[i].first;
            batch_requests[i].size = requests[i].second;
            batch_requests[i].offset_in_buffer = buffer_offset;
            buffer_offset += requests[i].second;
        }

        BOOL result = DeviceIoControl(
            kernelDriver,
            IOCTL_BATCH_READ,
            operation_buffer.data(),
            static_cast<DWORD>(total_buffer_size),
            operation_buffer.data(),
            static_cast<DWORD>(total_buffer_size),
            nullptr,
            nullptr
        );

        if (result) {
            // Copy output data (starts after the request structures)
            BYTE* output_start = operation_buffer.data() + request_struct_size;
            memcpy(output_buffer, output_start, output_data_size);
        }

        return result == TRUE;
    }

    // User-mode path (fallback)
    if (useUserModeOnly && hProcess != nullptr)
    {
        SIZE_T current_offset = 0;
        BYTE* output_buffer_bytes = reinterpret_cast<BYTE*>(output_buffer);

        for (const auto& req : requests)
        {
            if (req.second == 0) continue; // Skip zero-size requests

            SIZE_T bytesRead = 0;
            
            // Read in smaller chunks to avoid blocking the main thread
            const SIZE_T CHUNK_SIZE = 512;  // Read 512 bytes at a time
            SIZE_T bytes_remaining = req.second;
            DWORD64 current_addr = req.first;
            SIZE_T chunk_offset = 0;

            while (bytes_remaining > 0)
            {
                SIZE_T to_read = (bytes_remaining > CHUNK_SIZE) ? CHUNK_SIZE : bytes_remaining;
                
                if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(current_addr),
                                       output_buffer_bytes + current_offset + chunk_offset, 
                                       to_read, &bytesRead))
                {
                    return false; // Read failed
                }

                if (bytesRead == 0)
                {
                    return false; // No progress
                }

                bytes_remaining -= bytesRead;
                current_addr += bytesRead;
                chunk_offset += bytesRead;
                
                // Small sleep to yield to other threads - prevents stuttering
                if (bytes_remaining > 0)
                    Sleep(0);  // Yield to OS scheduler
            }

            current_offset += req.second;
        }
        return true;
    }

    return false;
}

// NEW: Priority reading for ESP data with optimizations
bool MemoryMgr::ReadESPData(DWORD64 address, float& outX, float& outY, float& outZ, DWORD64 entityId)
{
    if (address == 0 || hProcess == nullptr) {
        return false;
    }

    // Read 3 floats (12 bytes) as a single operation for speed
    struct { float x, y, z; } pos = { 0, 0, 0 };
    SIZE_T bytesRead = 0;

    // Use priority buffer - don't cache, always read fresh for accuracy
    if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), &pos, sizeof(pos), &bytesRead))
    {
        return false;
    }

    if (bytesRead != sizeof(pos))
    {
        return false;
    }

    // Update smoothing buffer immediately
    UpdatePositionSmoothing(entityId, pos.x, pos.y, pos.z);

    // Return smoothed position instead of raw
    GetSmoothedPosition(entityId, outX, outY, outZ);

    return true;
}

// NEW: Update position history for smoothing
void MemoryMgr::UpdatePositionSmoothing(DWORD64 entityId, float x, float y, float z)
{
    std::lock_guard<std::mutex> lock(entityCacheMutex);

    auto it = entityCache.find(entityId);
    if (it == entityCache.end())
    {
        // Create new entry
        EntityCache cache = {};
        cache.positions[0] = { x, y, z, GetTickCount64() };
        cache.frameIndex = 1;
        cache.smoothX = x;
        cache.smoothY = y;
        cache.smoothZ = z;
        entityCache[entityId] = cache;
    }
    else
    {
        EntityCache& cache = it->second;

        // Add to circular buffer
        cache.positions[cache.frameIndex] = { x, y, z, GetTickCount64() };
        cache.frameIndex = (cache.frameIndex + 1) % 10;

        // Calculate smoothed position (average of last 3 frames for stability)
        float avgX = 0, avgY = 0, avgZ = 0;
        int count = 0;
        for (int i = 0; i < 10; i++)
        {
            if (cache.positions[i].timestamp > 0 && (GetTickCount64() - cache.positions[i].timestamp) < 50) // Last 50ms
            {
                avgX += cache.positions[i].x;
                avgY += cache.positions[i].y;
                avgZ += cache.positions[i].z;
                count++;
            }
        }

        if (count > 0)
        {
            // Lerp smoothing for very fluid motion
            cache.smoothX = avgX / count;
            cache.smoothY = avgY / count;
            cache.smoothZ = avgZ / count;
        }
    }
}

// NEW: Get interpolated smooth position
void MemoryMgr::GetSmoothedPosition(DWORD64 entityId, float& outX, float& outY, float& outZ)
{
    std::lock_guard<std::mutex> lock(entityCacheMutex);

    auto it = entityCache.find(entityId);
    if (it != entityCache.end())
    {
        outX = it->second.smoothX;
        outY = it->second.smoothY;
        outZ = it->second.smoothZ;
    }
    else
    {
        outX = outY = outZ = 0.0f;
    }
}