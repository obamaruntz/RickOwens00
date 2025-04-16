#include <Driver/Communication.h>

Communication::Communication() {
    DriverHandle = CreateFileA("\\\\.\\RickOwens00", GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (DriverHandle == INVALID_HANDLE_VALUE) {
        DriverHandle = nullptr;
    }
}

Communication::~Communication() {
    if (DriverHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(DriverHandle);
    }
}

bool Communication::IsConnected() {
    return (DriverHandle != INVALID_HANDLE_VALUE);
}

bool Communication::VAttach(i32 ProcessID) {
    _VRW Arguments;
    Arguments.ProcessHandle = reinterpret_cast<HANDLE>(ProcessID);

    return DeviceIoControl(DriverHandle, VRW_ATTACH_CODE, &Arguments, sizeof(Arguments), &Arguments, sizeof(Arguments), nullptr, nullptr);
}

std::string Communication::ReadStr(u64 Address) {
    u16 Length = PRead<u16>(Address + 0x10);
    if (Length == 0 || Length > 255) {
        return "Unknown";
    }

    if (Length <= 15) {
        char Buffer[16] = {};
        _VRW Arguments;
        Arguments.Address = reinterpret_cast<void*>(Address);
        Arguments.Buffer = Buffer;
        Arguments.Size = sizeof(Buffer) - 1;
        Arguments.Type = false;

        DeviceIoControl(DriverHandle, VRW_CODE, &Arguments, sizeof(Arguments), &Arguments, sizeof(Arguments), nullptr, nullptr);
        return std::string(Buffer, Length);
    }
    else {
        u64 StringPtr = PRead<u64>(Address);
        char Buffer[256] = {};
        _VRW Arguments;
        Arguments.Address = reinterpret_cast<void*>(StringPtr);
        Arguments.Buffer = Buffer;
        Arguments.Size = Length;
        Arguments.Type = false;

        DeviceIoControl(DriverHandle, VRW_CODE, &Arguments, sizeof(Arguments), &Arguments, sizeof(Arguments), nullptr, nullptr);
        return std::string(Buffer, Length);
    }
}

i32 Communication::FindProcess(const i8* ProcessName) {
    PROCESSENTRY32 ProcessEntry = {};
    ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32First(Snapshot, &ProcessEntry)) {
        do {
            if (_stricmp(ProcessEntry.szExeFile, ProcessName) == 0) {
                ProcessID = ProcessEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(Snapshot, &ProcessEntry));
    }

    CloseHandle(Snapshot);

    return ProcessID;
}

u64 Communication::FindImage() {
    u64 ImageAddress = 0;
    _BA Arguments = {};
    Arguments.SecurityCode = SECURITY_CODE;
    Arguments.ProcessID = ProcessID;
    Arguments.Address = &ImageAddress;

    DeviceIoControl(DriverHandle, BA_CODE, &Arguments, sizeof(Arguments), nullptr, NULL, NULL, NULL);

    Communication::ImageAddress = ImageAddress;

    return ImageAddress;
}
