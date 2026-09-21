#pragma once
#include <fstream>
#include <filesystem>
#include <string>
#include <fcntl.h>

namespace fs = std::filesystem;

inline bool isNvidiaConnected() {
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator("/sys/class/drm", ec)) {
        std::string name = entry.path().filename().string();
        const auto pos = name.find('-');
        if (pos == std::string::npos) continue;

        std::string status, vendor;
        if (std::ifstream(entry.path() / "status") >> status && status == "connected") {
            // NVIDIA 为 0x10de
            if (std::ifstream("/sys/class/drm/" + name.substr(0, pos) + "/device/vendor") >> vendor) {
                if (vendor == "0x10de") return true;
            }
        }
    }
    return false;
}

inline bool getGpuAfterReboot() {
    constexpr auto filepath = "/sys/firmware/efi/efivars/OemMagicVariable-9f33f85c-13ca-4fd1-9c4a-96217722c593";
    const int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        std::cerr << "无法打开文件" << std::endl;
        return false;
    }
    unsigned char buffer[256];
    const ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
    close(fd);

    constexpr size_t target_offset = 0x66; // 偏移 0x66 (102)
    if (bytes_read <= static_cast<ssize_t>(target_offset)) {
        std::cerr << "读取字节数不足，仅读取到: " << bytes_read << " B" << std::endl;
        return false;
    }

    unsigned char byte_val = buffer[target_offset];
    return (byte_val == 0x01);
}