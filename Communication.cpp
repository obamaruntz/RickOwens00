#include <driver/communication.h>

communication::communication() {
    driver_handle = CreateFileA("\\\\.\\RickOwens00", GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    if (driver_handle == INVALID_HANDLE_VALUE) {
        driver_handle = nullptr;
    }
}

communication::~communication() {
    if (driver_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(driver_handle);
    }
}

bool communication::is_connected() {
    return (driver_handle != INVALID_HANDLE_VALUE);
}

bool communication::v_attach(i32 process_id) {
    _VRW arguments;
    arguments.process_handle = reinterpret_cast<HANDLE>(process_id);

    return DeviceIoControl(driver_handle, VRW_ATTACH_CODE, &arguments, sizeof(arguments), &arguments, sizeof(arguments), nullptr, nullptr);
}

std::string communication::readstr(u64 address) {
    i32 StrLength = read<i32>(address + 0x18);

    if (StrLength >= 16) {
        address = read<u64>(address);
    }

    std::vector<i8> Buffer(256);

    _PRW arguments = {};
    arguments.security_code = SECURITY_CODE;
    arguments.address = reinterpret_cast<void*>(address);
    arguments.buffer = Buffer.data();
    arguments.size = Buffer.size();
    arguments.process_id = process_id;
    arguments.Type = false;

    DeviceIoControl(driver_handle, PRW_CODE, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);

    return std::string(Buffer.data());
}

i32 communication::find_process(const i8* process_name) {
    PROCESSENTRY32 process_entry = {};
    process_entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32First(snapshot, &process_entry)) {
        do {
            if (_stricmp(process_entry.szExeFile, process_name) == 0) {
                process_id = process_entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &process_entry));
    }

    CloseHandle(snapshot);

    return process_id;
}

u64 communication::find_image() {
    u64 image_address = 0;
    _BA arguments = {};
    arguments.security_code = SECURITY_CODE;
    arguments.process_id = process_id;
    arguments.address = &image_address;

    DeviceIoControl(driver_handle, BA_CODE, &arguments, sizeof(arguments), nullptr, NULL, NULL, NULL);

    communication::image_address = image_address;

    return image_address;
}
