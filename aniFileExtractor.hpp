// Inspired by https://github.com/Mastermindzh/Scripts/blob/master/c%2B%2B/ani2png.c

#include <filesystem>
#include <vector>
#include <memory>
#include <cstdint>
#include <optional>
#include <fstream>
#include <string>
#include <cstring>
#include <iostream>

/**
 * Output debug comments
 */
constexpr bool debug = true;

/**
 * @brief Read binary data from file to vector
 * @param filePath The filepath of the binary file to be read
 * @return Binary data of file
 */
std::shared_ptr<std::vector<uint8_t>> readBinaryFile(const std::filesystem::path &filePath)
{
    if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("The file " + filePath.string() + " was not found");
    }
    const auto fileSize = std::filesystem::file_size(filePath);
    //std::vector<uint8_t> buffer(fileSize);
    //buffer.reserve(fileSize);
    auto buffer = std::make_shared<std::vector<uint8_t>>(fileSize);
    buffer->reserve(fileSize);
    std::ifstream binaryInputFile(filePath, std::ios::in | std::ios::binary);
    if (!binaryInputFile.is_open()) {
        throw std::runtime_error("The file " + filePath.string() + " could not be opened");
    }
    //binaryInputFile.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    binaryInputFile.read(reinterpret_cast<char *>(buffer->data()), fileSize);
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
                     const std::shared_ptr<std::vector<uint8_t>> &data)
{
    std::ofstream binaryOutputFile(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!binaryOutputFile.is_open()) {
        throw std::runtime_error("The file " + filePath.string() + " could not be opened");
    }
    for (const auto &entry : *data) {
        binaryOutputFile.write(reinterpret_cast<const char *>(&entry), sizeof(char));
    }
    binaryOutputFile.close();
    if constexpr(debug) {
        std::cout << "> " << filePath << " (size=" << data->size() << ") was successfully written" <<
                  std::endl;
    }
}

/**
 * @brief Read 4 bytes that represent a little endian 32 Bit unsigned number (like a DWORD)
 * @param data The vector that contains the binary data to be written
 * @param start The start index in the vector from which should be read
 * @return A 32 Bit unsigned number
 */
uint32_t read32BitUnsignedIntegerLE(const std::shared_ptr<std::vector<uint8_t>> &data,
                                    const std::size_t start)
{
    uint32_t number;
    uint8_t bytes[4] { data->at(start), data->at(start + 1), data->at(start + 2), data->at(start + 3) };
    std::memcpy(&number, bytes, sizeof(number));
    return number;
}

/**
 * @brief Read 2 bytes that represent a little endian 16 Bit unsigned number (like a WORD)
 * @param data The vector that contains the binary data to be written
 * @param start The start index in the vector from which should be read
 * @return A 16 Bit unsigned number
 */
uint16_t read16BitUnsignedIntegerLE(const std::shared_ptr<std::vector<uint8_t>> &data,
                                    const std::size_t start)
{
    uint16_t number;
    uint8_t bytes[2] { data->at(start), data->at(start + 1) };
    std::memcpy(&number, bytes, sizeof(number));
    return number;
}

/**
 * @brief Read bytes that represent a char string
 * @param data The vector that contains the binary data to be written
 * @param start The start index in the vector from which should be read
 * @param length The number of bytes that should be read and the length of the resulting string
 * @return A char string
 */
std::string readCharString(const std::shared_ptr<std::vector<uint8_t>> &data,
                           const std::size_t start, const std::size_t length)
{
    std::string out = "";
    for (std::size_t i = start; i < start + length; i++) {
        out += static_cast<char>(data->at(i));
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
    std::vector<std::shared_ptr<std::vector<uint8_t>>> icons = {};
    /** If existing the content of the art tag */
    std::optional<std::string> art = {};
    /** If existing the content of the name tag */
    std::optional<std::string> name = {};
    /** Num bytes in AniHeader (36 bytes) */
    uint32_t cbSizeOf;
    /** Number of unique Icons in this cursor */
    uint32_t cFrames;
    /** Number of Blits before the animation cycles */
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

AniFileInformation getAniFileInformation(const std::shared_ptr<std::vector<uint8_t>> &data)
{
    AniFileInformation aniFileInformation;
    // Check for RIFF at the begin of the data
    if (8 <= data->size() && readCharString(data, 0, 4) == "RIFF") {
        aniFileInformation.riffDataLength = read32BitUnsignedIntegerLE(data, 4);
        if constexpr(debug) {
            std::cout << "> RIFF header was found at " << 0 << std::endl;
        }
    }
    else {
        throw std::runtime_error(".ani data did not start with RIFF container name and length");
    }
    if (12 <= data->size() && readCharString(data, 8, 4) == "ACON") {
        aniFileInformation.riffContainerType = "ACON";
        if constexpr(debug) {
            std::cout << "> Found RIFF field 'ACON' at " << 8 << std::endl;
        }
    }
    else {
        throw std::runtime_error(".ani data did not have the ACON field in the RIFF container");
    }
    for (std::size_t i = 12; i < data->size(); i ++) {
        if (i + 8 <= data->size() && readCharString(data, i, 4) == "INAM") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'INAM' at " << i << " [length=" << length << ",data='";
            }
            i += 8;
            if (i + length <= data->size()) {
                aniFileInformation.name = readCharString(data, i, length);
                i += length;
            }
            else {
                throw std::runtime_error("Unexpected end of file while reading 'INAM' data");
            }
            if constexpr(debug) {
                std::cout << aniFileInformation.name.value_or("ERROR: Was not read") << "']" << std::endl;
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data->size() && readCharString(data, i, 4) == "IART") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'IART' at " << i << " [length=" << length << ",data='";
            }
            i += 8;
            if (i + length <= data->size()) {
                aniFileInformation.art = readCharString(data, i, length);
                i += length;
            }
            else {
                throw std::runtime_error("Unexpected end of file while reading 'IART' data");
            }
            if constexpr(debug) {
                std::cout << aniFileInformation.art.value_or("ERROR: Was not read") << "']" << std::endl;
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data->size() && readCharString(data, i, 4) == "icon") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'icon' at " << i << " [length=" << length << "]" << std::endl;
            }
            i += 8;
            if (i + length <= data->size()) {
                auto iconData = std::make_shared<std::vector<uint8_t>>(length);
                iconData->reserve(length);
                for (std::size_t j = 0; j < length; j++) {
                    iconData->at(j) = data->at(i + j);
                }
                aniFileInformation.icons.push_back(iconData);
                i += length;
            }
            else {
                throw std::runtime_error("Unexpected end of file while reading 'icon' data");
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data->size() && readCharString(data, i, 4) == "seq ") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'seq ' at " << i << " [length=" << length << "]" << std::endl;
            }
            i += 8;
            if (i + length <= data->size()) {
                auto seqContent = readCharString(data, i, length);
                // TODO What is this
                if constexpr(debug) {
                    std::cout << ">> 'seq ' content: '" << seqContent << "'" << std::endl;
                }
                i += length;
            }
            else {
                throw std::runtime_error("Unexpected end of file while reading 'seq ' data");
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data->size() && readCharString(data, i, 4) == "rate") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'rate' at " << i << " [length=" << length << "]" << std::endl;
            }
            i += 8;
            if (i + length <= data->size()) {
                auto seqContent = readCharString(data, i, length);
                // TODO What is this
                if constexpr(debug) {
                    std::cout << ">> 'rate' content: '" << seqContent << "'" << std::endl;
                }
                i += length;
            }
            else {
                throw std::runtime_error("Unexpected end of file while reading 'rate' data");
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data->size() && readCharString(data, i, 4) == "anih") {
            auto length = read32BitUnsignedIntegerLE(data, i + 4);
            if constexpr(debug) {
                std::cout << "> Found RIFF field 'anih' at " << i << " [length=" << length << "]" << std::endl;
            }
            i += 8;
            if (length != 36) {
                throw std::runtime_error("Unexpected length of 'anih' field " + std::to_string(length) + "!=36");
            }
            if (i + length <= data->size()) {
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
            }
            else {
                throw std::runtime_error("Unexpected end of file while reading 'anih' data");
            }
            i -= 1;
            continue;
        }
        if (i + 8 <= data->size() && static_cast<char>(data->at(i)) == 'L' &&
            static_cast<char>(data->at(i + 1)) == 'I' && static_cast<char>(data->at(i + 2)) == 'S' &&
            static_cast<char>(data->at(i + 3)) == 'T') {
            std::cout << "> Found 'LIST' at " << i;
            i += 4;
            auto length = read32BitUnsignedIntegerLE(data, i);
            std::cout << " [length=" << length << "]" << std::endl;
            i += 4;
            i -= 1;
            continue;
        }
        if (i + 4 <= data->size() && static_cast<char>(data->at(i)) == 'f' &&
            static_cast<char>(data->at(i + 1)) == 'r' && static_cast<char>(data->at(i + 2)) == 'a' &&
            static_cast<char>(data->at(i + 3)) == 'm') {
            std::cout << "> Found 'fram' at " << i << std::endl;
            i += 4;
            i -= 1;
            continue;
        }
        throw std::runtime_error("PARSE PROBLEM: Unexpected input detected at pos " + std::to_string(
                                     i) + " ('" + static_cast<char>(data->at(i)) + "')");
    }
    return aniFileInformation;
}

/**
 * Source: https://stackoverflow.com/a/13001420
 *
 * @brief Swap the endian of a number (LE -> BR or otherwise)
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

/**
 * Sources:
 * - https://docs.fileformat.com/image/png/
 *
 * The first eight bytes of a PNG file always contain the following (decimal) values: {{{ 137 80 78 71 13 10 26 10 }}}
 * Then a list of chunks is following where each chunk:
 *   > {4 Bytes=BigEndianUnsignedInt=chunkLength} (Number of byte data of chunk)
 *   > {4 Bytes=4 ASCII chars=chunkType}
 *   > {chunkLength Bytes=chunkData}
 *   > {4 Bytes=crc}
 */
void printPngInformation(const std::shared_ptr<std::vector<uint8_t>> &data)
{
    if (8 >= data->size()) {
        std::cout << "> No png header found (too small)" << std::endl;
        return;
    }
    if (!(data->at(0) == 137 && data->at(1) == 80 && data->at(2) == 78 && data->at(3) == 71 &&
          data->at(4) == 13 && data->at(5) == 10 && data->at(6) == 26 && data->at(7) == 10)) {
        std::cout << "> No png header found (leading 8 bytes incorrect)" << std::endl;
        return;
    }
    std::cout <<             "Position | Size | Purpose     | Content [png data header]" << std::endl;
    std::cout << 0  << "       | 1    | 137         | '" << static_cast<int>(data->at(
                  0)) << "'" << std::endl;
    std::cout << 1  << "       | 1    | 80          | '" << static_cast<int>(data->at(
                  1)) << "'" << std::endl;
    std::cout << 2  << "       | 1    | 78          | '" << static_cast<int>(data->at(
                  2)) << "'" << std::endl;
    std::cout << 3  << "       | 1    | 71          | '" << static_cast<int>(data->at(
                  3)) << "'" << std::endl;
    std::cout << 4  << "       | 1    | 13          | '" << static_cast<int>(data->at(
                  4)) << "'" << std::endl;
    std::cout << 5  << "       | 1    | 10          | '" << static_cast<int>(data->at(
                  5)) << "'" << std::endl;
    std::cout << 6  << "       | 1    | 26          | '" << static_cast<int>(data->at(
                  6)) << "'" << std::endl;
    std::cout << 7  << "       | 1    | 10          | '" << static_cast<int>(data->at(
                  7)) << "'" << std::endl;
    for (std::size_t i = 8; i < data->size(); i++) {
        if (i + 8 < data->size()) {
            auto chunkSize = static_cast<unsigned int>(read32BitUnsignedIntegerLE(data, i));
            endianSwap(chunkSize);
            const auto chunkDataType = readCharString(data, i + 4, 4);
            std::cout << i  <<     "       | 4    | chunk size  | '" << chunkSize << "'" << std::endl;
            i += 4;
            std::cout << i  <<     "       | 4    | chunk type  | '" << chunkDataType << "'" << std::endl;
            i += 4;
            i += chunkSize;
            if (i + 4 < data->size()) {
                std::cout << i  <<     "       | 4    | crc         | '" << static_cast<int>(data->at(
                              i)) << " " << static_cast<int>(data->at(i + 1)) << " " << static_cast<int>(data->at(
                                          i + 2)) << " " << static_cast<int>(data->at(i + 3)) << "'" << std::endl;
                i += 4;
            }
            else if (chunkDataType != "IEND") {
                std::cout << "crc value missing since the data is too short" << std::endl;
            }
            // +4 because of the crc value
            i -= 1;
        }
    }
}

void printIcoDirectoryHeaderInformation(const std::shared_ptr<std::vector<uint8_t>> &data,
                                        const std::size_t start, const int directoryNumber)
{
    std::cout <<             "Position | Size | Purpose     | Content [ico directory header #" <<
              directoryNumber << "]" << std::endl;
    std::cout << start + 0  << "       | 1    | width       | '" << static_cast<int>(data->at(
                  start + 0)) << "'" << std::endl;
    std::cout << start + 1  << "       | 1    | height      | '" << static_cast<int>(data->at(
                  start + 1)) << "'" << std::endl;
    std::cout << start + 2  << "       | 1    | colorCount  | '" << static_cast<int>(data->at(
                  start + 2)) << "'" << std::endl;
    std::cout << start + 3  << "       | 1    | reserved    | '" << static_cast<int>(data->at(
                  start + 3)) << "'" << std::endl;
    std::cout << start + 4  << "       | 2    | planes      | '" << read16BitUnsignedIntegerLE(data,
              start + 4) << "'" << std::endl;
    std::cout << start + 6  << "       | 2    | bitCount    | '" << read16BitUnsignedIntegerLE(data,
              start + 6) << "'" << std::endl;
    std::cout << start + 8  << "       | 4    | bytesInRes  | '" << read32BitUnsignedIntegerLE(data,
              start + 8) << "'" << std::endl;
    const auto imageOffset = read16BitUnsignedIntegerLE(data, start + 12);
    std::cout << start + 12 << "       | 4    | imageOffset | '" << imageOffset << "'" << std::endl;
}

/**
 * Sources:
 * - https://en.wikipedia.org/wiki/ICO_(file_format)
 * - https://docs.fileformat.com/image/ico/
 *
 * All values in ICO/CUR files are represented in little-endian byte order (default in programming).
 *
 * File structure:
 *
 * 1. Icon Header:     Stores general information about the ICO file.
 * 2. Directory[1..n]: Stores general information about every image in the file.
 * 3. Icon[1..n]:      The actual data for the image sin old AND/XOR DIB format or newer PNG format.
 *
 * Icon Header
 * -------------
 *
 * | Offset# | Size | Purpose
 * | 0       | 2    | Reserved. Must always be 0.
 * | 2       | 2    | Specifies image type: 1 for icon (.ICO) image, 2 for cursor (.CUR) image. Other values are invalid.
 * | 4       | 2    | Specifies number of images in the file.
 *
 * Directory
 * -------------
 *
 * | Offset# | Size | Purpose
 * | 0       | 1    | Specifies image width in pixels. Can be any number between 0 and 255. Value 0 means image width is 256 pixels.
 * | 1       | 1    | Specifies image height in pixels. Can be any number between 0 and 255. Value 0 means image height is 256 pixels.
 * | 2       | 1    | Specifies number of colors in the color palette. Should be 0 if the image does not use a color palette.
 * | 3       | 1    | Reserved. Should be 0.[Notes 2]
 * | 4       | 2    | In ICO format: Specifies color planes. Should be 0 or 1.[Notes 3] In CUR format: Specifies the horizontal coordinates of the hotspot in number of pixels from the left.
 * | 6       | 2    | In ICO format: Specifies bits per pixel. [Notes 4] In CUR format: Specifies the vertical coordinates of the hotspot in number of pixels from the top.
 * | 8       | 4    | Specifies the size of the image's data in bytes
 * | 12      | 4    | Specifies the offset of BMP or PNG data from the beginning of the ICO/CUR file
 *
 * @brief printIcoDataHeader
 * @param data
 */
void printIcoInformation(const std::shared_ptr<std::vector<uint8_t>> &data)
{
    // The default header
    std::cout << "Position | Size | Purpose     | Content [ico file header]" << std::endl;
    std::cout << "0        | 2    | reserved    | '" << read16BitUnsignedIntegerLE(data,
              0) << "'" << std::endl;
    std::cout << "2        | 2    | image type  | '" << read16BitUnsignedIntegerLE(data,
              2) << "'" << std::endl;
    const auto imageCount = read16BitUnsignedIntegerLE(data, 4);
    std::cout << "4        | 2    | image #     | '" << imageCount << "'" << std::endl;
    // The directory headers
    for (int i = 0; i < imageCount; i++) {
        printIcoDirectoryHeaderInformation(data, 6 + (i * 16), i);
    }
}