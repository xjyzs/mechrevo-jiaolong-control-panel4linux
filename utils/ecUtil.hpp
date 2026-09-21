#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include "sudoHelper.hpp"

/**
 * @brief 读取 EC
 *
 * @return int 失败返回 -1
 */
int readEc(int addr1, int addr2 = -1) {
    std::stringstream cmd;

    // 格式化为 0x0000 格式
    auto to_hex_str = [](int addr) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << addr;
        return ss.str();
    };
    cmd << "echo '\\_SB.INOU.ECRR " << to_hex_str(addr1)
        << "' | tee /proc/acpi/call >/dev/null; tr -d '\\0' < /proc/acpi/call";

    if (addr2 != -1) {
        cmd << "; echo -n ' '; echo '\\_SB.INOU.ECRR " << to_hex_str(addr2)
            << "' | tee /proc/acpi/call >/dev/null; tr -d '\\0' < /proc/acpi/call";
    }

    std::string result;
    root_system(cmd.str(), &result);

    std::stringstream ss(result);
    std::string hex1, hex2;

    try {
        if (addr2 == -1) {
            if (ss >> hex1) {
                return std::stoi(hex1, nullptr, 16);
            }
        } else {
            if (ss >> hex1 >> hex2) {
                int high = std::stoi(hex1, nullptr, 16);
                int low  = std::stoi(hex2, nullptr, 16);
                return (high << 8) | low;
            }
        }
    } catch (...) {
        return -1;
    }

    return -1;
}

/**
 * @brief 写入 EC
 */
bool writeEc(int addr, int value) {
    std::stringstream cmd;
    cmd << "echo '\\_SB.INOU.ECRW 0x"
        << std::hex << std::setw(4) << std::setfill('0') << addr
        << " 0x"
        << std::hex << std::setw(2) << std::setfill('0') << value
        << "' | tee /proc/acpi/call >/dev/null; tr -d '\\0' < /proc/acpi/call";

    std::string result;
    try {
        root_system(cmd.str(), &result);
    } catch (...) {
        return false;
    }
    return true;
}