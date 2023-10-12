// Inspired by https://github.com/Mastermindzh/Scripts/blob/master/c%2B%2B/ani2png.c

#pragma once

#include <filesystem>
#include <array>
#include <vector>
#include <cstdint>
#include <optional>
#include <fstream>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <variant>

/**
 * Output debug comments
 */
constexpr bool debug = true;

/**
 * @brief Read binary data from file to vector
 * @param filePath The filepath of the binary file to be read
 * @return Binary data of file
 */
std::vector<uint8_t> readBinaryFile(const std::filesystem::path &filePath)
{
    if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("The file " + filePath.string() + " was not found");
    }
    const auto fileSize = std::filesystem::file_size(filePath);
    std::vector<uint8_t> buffer(fileSize);
    buffer.reserve(fileSize);
    std::ifstream binaryInputFile(filePath, std::ios::in | std::ios::binary);
    if (!binaryInputFile.is_open()) {
        throw std::runtime_error("The file " + filePath.string() + " could not be opened");
    }
    binaryInputFile.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
    binaryInputFile.close();
    if constexpr(debug) {
        std::cout << "> " << filePath << " (size=" << fileSize << ") was successfully read" << std::endl;
    }
    return buffer;
}

/**
 * @brief Write binary file from vector to file
 * @param filePath The filepath of the binary file to be written
 * @param data The vector that contains the binary data to be written
 */
void writeBinaryFile(const std::filesystem::path &filePath,
                     const std::vector<uint8_t> &data)
{
    std::ofstream binaryOutputFile(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!binaryOutputFile.is_open()) {
        throw std::runtime_error("The file " + filePath.string() + " could not be opened");
    }
    for (const auto &entry : data) {
        binaryOutputFile.write(reinterpret_cast<const char *>(&entry), sizeof(char));
    }
    binaryOutputFile.close();
    if constexpr(debug) {
        std::cout << "> " << filePath << " (size=" << data.size() << ") was successfully written" <<
                  std::endl;
    }
}

/**
 * @brief Write text file from string to file
 * @param filePath The filepath of the text file to be written
 * @param data The string that contains the text data to be written
 */
void writeTextFile(const std::filesystem::path &filePath, const std::string &data)
{
    std::ofstream textOutputFile(filePath, std::ios::out | std::ios::trunc);
    if (!textOutputFile.is_open()) {
        throw std::runtime_error("The file " + filePath.string() + " could not be opened");
    }
    textOutputFile.write(data.c_str(), data.size());
    textOutputFile.close();
    if constexpr(debug) {
        std::cout << "> " << filePath << " (size=" << data.size() << ") was successfully written" <<
                  std::endl;
    }
}

/**
 * @brief Read 4 bytes that represent a little endian 32 Bit unsigned number (like a DWORD)
 * @param data The vector that contains the binary data to be written
 * @param start The start index in the vector from which should be read
 * @return A 32 Bit unsigned number
 */
uint32_t read32BitUnsignedIntegerLE(const std::vector<uint8_t> &data,
                                    const std::size_t start)
{
    uint32_t number;
    std::array<uint8_t, 4> bytes { data.at(start), data.at(start + 1), data.at(start + 2), data.at(start + 3) };
    std::memcpy(&number, bytes.data(), sizeof(number));
    return number;
}

/**
 * @brief Read 2 bytes that represent a little endian 16 Bit unsigned number (like a WORD)
 * @param data The vector that contains the binary data to be written
 * @param start The start index in the vector from which should be read
 * @return A 16 Bit unsigned number
 */
uint16_t read16BitUnsignedIntegerLE(const std::vector<uint8_t> &data,
                                    const std::size_t start)
{
    uint16_t number;
    std::array<uint8_t, 2> bytes { data.at(start), data.at(start + 1) };
    std::memcpy(&number, &bytes, sizeof(number));
    return number;
}

/**
 * @brief Read bytes that represent a char string
 * @param data The vector that contains the binary data to be written
 * @param start The start index in the vector from which should be read
 * @param length The number of bytes that should be read and the length of the resulting string
 * @return A char string
 */
std::string readCharString(const std::vector<uint8_t> &data,
                           const std::size_t start, const std::size_t length)
{
    std::string out(length, ' ');
    for (std::size_t i = 0; i < length; i++) {
        out.at(i) = static_cast<char>(data.at(start + i));
    }
    return out;
}

/**
 * Collection of data that a `.ani` file contains
 */
struct AniFileInformation {
    /**
     * A list of all the contained images (their data blocks)
     */
    std::vector<std::vector<uint8_t>> icons = {};
    /** If existing the content of the art tag */
    std::optional<std::string> art = {};
    /** If existing the content of the name tag */
    std::optional<std::string> name = {};
    /** Num bytes in AniHeader (36 bytes) */
    uint32_t cbSizeOf;
    /** Number of unique Icons in this cursor */
    uint32_t cFrames;
    /** Number of Bits before the animation cycles */
    uint32_t cSteps;
    /** reserved, must be zero */
    uint32_t cx;
    /** reserved, must be zero */
    uint32_t cy;
    /** reserved, must be zero */
    uint32_t cBitCount;
    /** reserved, must be zero */
    uint32_t cPlanes;
    /** Default Jiffies (1/60th of a second) if rate chunk not present */
    uint32_t JifRate;
    /** Animation Flag (see AF_ constants) */
    uint32_t flags;
    /** RIFF container data length */
    uint32_t riffDataLength;
    /** RIFF container contains ACON identifier */
    std::string riffContainerType;
};

/**
 * RIFF/.ani file format:
 *
 * Start of the file should be this to be a valid .ani file:
 * "RIFF" {4 Bytes=DWORD=length of file} (container type and length)
 * "ACON" (what does the generic RIFF container represent)
 * Then in no particular order:
 * "LIST" {4 Bytes=DWORD=length of list} (TODO - not important?)
 * "INAM" {4 Bytes=DWORD=length of title} {title data in chars} (the title of the icon)
 * "IART" {4 Bytes=DWORD=length of author} {author data in chars} (the author of the icon)
 * "fram" (TODO - not important?)
 * "icon" {4 Bytes=DWORD=length of icon} {icon data} (TODO - IMPORTANT - WHAT IS THE DATA FORMAT???)
 * "anih" {4 Bytes=DWORD=length of ANI header (36 bytes)} {Data}
 *   > {4 Bytes=DWORD=cbSizeOf} (Number of unique Icons in this cursor)
 *   > {4 Bytes=DWORD=cFrames} (Num bytes in AniHeader)
 *   > {4 Bytes=DWORD=cSteps} (Number of Blits before the animation cycles)
 *   > {4 Bytes=DWORD=cx} (reserved, must be zero)
 *   > {4 Bytes=DWORD=cy} (reserved, must be zero)
 *   > {4 Bytes=DWORD=cBitCount} (reserved, must be zero)
 *   > {4 Bytes=DWORD=cPlanes} (reserved, must be zero)
 *   > {4 Bytes=DWORD=JifRate} (Default Jiffies (1/60th of a second) if rate chunk not present)
 *   > {4 Bytes=DWORD=flags} (Animation Flag - TODO?)
 * "rate" {4 Bytes=DWORD=length of rate block} {Data} (TODO - not important?)
 * "seq " {4 Bytes=DWORD=length of sequence block} {Data} (TODO - not important?)
 *
 * Sources are https://www.gdgsoft.com/anituner/help/aniformat.htm which cites a post by R. James Houghtaling
 * and the website www.wotsit.org by Paul Oliver which is not accessible any more.
 *
 * This parser is not a good one.
 * Without having any information about the actual .ani file format and what the differences are between
 * animated and not animated icons it just checks if it is an animated icon and finds all icon blocks.
 * If there are multiple INAM/IART blocks or something similar this parser WILL FAIL!!!!!
 *
 * @brief Read out all the information from a given `.ani` file binary data vector
 * @param data The `.ani` file binary data vector
 * @return Information object that contains all read data
 */
AniFileInformation readAniFileInformation(const std::vector<uint8_t> &data)
{
    AniFileInformation aniFileInformation {};
    // Check for RIFF at the begin of the data
    if (8 <= data.size() && readCharString(data, 0, 4) == "RIFF") {
        aniFileInformation.riffDataLength = read32BitUnsignedIntegerLE(data, 4);
        if constexpr(debug) {
            std::cout << "> RIFF header was found at " << 0 << std::endl;
        }
    } else {
        throw std::runtime_error(".ani data did not start with RIFF container name and length");
    }
    if (12 <= data.size() && readCharString(data, 8, 4) == "ACON") {
        aniFileInformation.riffContainerType = "ACON";
        if constexpr(debug) {
            std::cout << "> Found RIFF field 'ACON' at " << 8 << std::endl;
        }
    } else {
        throw std::runtime_error(".ani data did not have the ACON field in the RIFF container");
    }
    for (std::size_t i = 12; i < data.size(); i ++) {
        if (i + 8 <= data.size() && readCharString(data, i, 4) == "INAM") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'INAM' at " << i << " [length=" << length << ",data='";
            }
            i += 8;
            if (i + length <= data.size()) {
                aniFileInformation.name = readCharString(data, i, length);
                i += length;
            } else {
                throw std::runtime_error("Unexpected end of file while reading 'INAM' data");
            }
            if constexpr(debug) {
                std::cout << aniFileInformation.name.value_or("ERROR: Was not read") << "']" << std::endl;
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data.size() && readCharString(data, i, 4) == "IART") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'IART' at " << i << " [length=" << length << ",data='";
            }
            i += 8;
            if (i + length <= data.size()) {
                aniFileInformation.art = readCharString(data, i, length);
                i += length;
            } else {
                throw std::runtime_error("Unexpected end of file while reading 'IART' data");
            }
            if constexpr(debug) {
                std::cout << aniFileInformation.art.value_or("ERROR: Was not read") << "']" << std::endl;
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data.size() && readCharString(data, i, 4) == "icon") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'icon' at " << i << " [length=" << length << "]" << std::endl;
            }
            i += 8;
            if (i + length <= data.size()) {
                std::vector<uint8_t> iconData(length);
                iconData.reserve(length);
                for (std::size_t j = 0; j < length; j++) {
                    iconData.at(j) = data.at(i + j);
                }
                aniFileInformation.icons.push_back(iconData);
                i += length;
            } else {
                throw std::runtime_error("Unexpected end of file while reading 'icon' data");
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data.size() && readCharString(data, i, 4) == "seq ") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'seq ' at " << i << " [length=" << length << "]" << std::endl;
            }
            i += 8;
            if (i + length <= data.size()) {
                auto seqContent = readCharString(data, i, length);
                // TODO What is this
                if constexpr(debug) {
                    std::cout << ">> 'seq ' content: '" << seqContent << "'" << std::endl;
                }
                i += length;
            } else {
                throw std::runtime_error("Unexpected end of file while reading 'seq ' data");
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data.size() && readCharString(data, i, 4) == "rate") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'rate' at " << i << " [length=" << length << "]" << std::endl;
            }
            i += 8;
            if (i + length <= data.size()) {
                auto seqContent = readCharString(data, i, length);
                // TODO What is this
                if constexpr(debug) {
                    std::cout << ">> 'rate' content: '" << seqContent << "'" << std::endl;
                }
                i += length;
            } else {
                throw std::runtime_error("Unexpected end of file while reading 'rate' data");
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data.size() && readCharString(data, i, 4) == "anih") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'anih' at " << i << " [length=" << length << "]" << std::endl;
            }
            i += 8;
            if (length != 36) {
                throw std::runtime_error("Unexpected length of 'anih' field " + std::to_string(length) + "!=36");
            }
            if (i + length <= data.size()) {
                aniFileInformation.cbSizeOf = read32BitUnsignedIntegerLE(data, i);
                i += 4;
                aniFileInformation.cFrames = read32BitUnsignedIntegerLE(data, i);
                i += 4;
                aniFileInformation.cSteps = read32BitUnsignedIntegerLE(data, i);
                i += 4;
                aniFileInformation.cx = read32BitUnsignedIntegerLE(data, i);
                i += 4;
                aniFileInformation.cy = read32BitUnsignedIntegerLE(data, i);
                i += 4;
                aniFileInformation.cBitCount = read32BitUnsignedIntegerLE(data, i);
                i += 4;
                aniFileInformation.cPlanes = read32BitUnsignedIntegerLE(data, i);
                i += 4;
                aniFileInformation.JifRate = read32BitUnsignedIntegerLE(data, i);
                i += 4;
                aniFileInformation.flags = read32BitUnsignedIntegerLE(data, i);
                i += 4;
            } else {
                throw std::runtime_error("Unexpected end of file while reading 'anih' data");
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data.size() && static_cast<char>(data.at(i)) == 'L' &&
            static_cast<char>(data.at(i + 1)) == 'I' && static_cast<char>(data.at(i + 2)) == 'S' &&
            static_cast<char>(data.at(i + 3)) == 'T') {
            std::cout << "> Found 'LIST' at " << i;
            i += 4;
            auto length = read32BitUnsignedIntegerLE(data, i);
            std::cout << " [length=" << length << "]" << std::endl;
            i += 4;
            i -= 1;
            continue;
        }
        if (i + 4 <= data.size() && static_cast<char>(data.at(i)) == 'f' &&
            static_cast<char>(data.at(i + 1)) == 'r' && static_cast<char>(data.at(i + 2)) == 'a' &&
            static_cast<char>(data.at(i + 3)) == 'm') {
            std::cout << "> Found 'fram' at " << i << std::endl;
            i += 4;
            i -= 1;
            continue;
        }
        throw std::runtime_error("PARSE PROBLEM: Unexpected input detected at pos " + std::to_string(
                                     i) + " ('" + static_cast<char>(data.at(i)) + "')");
    }
    return aniFileInformation;
}

/**
 * Source: https://stackoverflow.com/a/13001420
 *
 * @brief Swap the endian of a number (LE -> BE or otherwise)
 * @param x The number which should be interpreted with a different endian
 * @return The same number when reading it with the opposite endian
 */
inline void endianSwap(unsigned int &x)
{
    x = (x >> 24) |
        ((x << 8) & 0x00FF0000) |
        ((x >> 8) & 0x0000FF00) |
        (x << 24);
}
