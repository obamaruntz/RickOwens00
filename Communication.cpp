#include "Communication.h"

Communication::Communication() {
    driverHandle = CreateFileA("\\\\.\\RickOwens00", GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
}

Communication::~Communication() {
    if (driverHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(driverHandle);
    }
}

bool Communication::isConnected() {
    return (driverHandle != INVALID_HANDLE_VALUE);
}

bool Communication::virtualAttach(i32 processId) {
    _VRW arguments;
    arguments.processHandle = reinterpret_cast<HANDLE>(processId);

    return DeviceIoControl(driverHandle, VRW_ATTACH_CODE, &arguments, sizeof(arguments), &arguments, sizeof(arguments), nullptr, nullptr);
}

std::string Communication::readString(u64 address) {
    i32 stringLength = read<i32>(address + 0x18);

    if (stringLength >= 16) {
        address = read<u64>(address);
    }

    std::vector<i8> buffer(256);

    _PRW arguments = {};
    arguments.securityCode = SECURITY_CODE;
    arguments.address = reinterpret_cast<void*>(address);
    arguments.buffer = buffer.data();
    arguments.size = buffer.size();
    arguments.processId = processId;
    arguments.type = false;

    DeviceIoControl(driverHandle, PRW_CODE, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);

    return std::string(buffer.data());
}

i32 Communication::findProcess(const i8* processName) {
    PROCESSENTRY32 processEntry = {};
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32First(snapshot, &processEntry)) {
        do {
            if (_stricmp(processEntry.szExeFile, processName) == 0) {
                processId = processEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    CloseHandle(snapshot);

    return processId;
}

u64 Communication::findImage() {
    _BA arguments = {};
    arguments.securityCode = SECURITY_CODE;
    arguments.processId = processId;
    arguments.address = &imageAddress;

    DeviceIoControl(driverHandle, BA_CODE, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);

    return imageAddress;
}

HANDLE Communication::getDriverHandle() {
    return driverHandle;
}

i32 Communication::getProcessId() {
    return processId;
}

u64 Communication::getImageAddress() {
    return imageAddress;
}
