#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include "aniFileExtractor.hpp"

/**
 * @brief Pad a string (right) with characters
 */
std::string padRight(const std::string &stringToPad, const std::size_t padLength,
                     const char padCharacter)
{
    std::ostringstream ss;
    ss << stringToPad;
    for (std::size_t i = stringToPad.length(); i < padLength; i++) {
        ss << padCharacter;
    }
    return ss.str();
}

/**
 * @brief Pad a number or anything that can be put into a string stream (right) with characters
 */
template<typename U>
std::string padRightNumber(const U &numberToPad, const std::size_t padLength,
                           const char padCharacter)
{
    std::ostringstream ss;
    ss << numberToPad;
    return padRight(ss.str(), padLength, padCharacter);
}

enum class PrintTableColumnDataType {
    HIDE, NONE, CHAR, INT, UINT_16, UINT_32, UINT_32_BE
};
using PrintTableColumn =
    std::tuple<const std::size_t, const std::size_t, const std::string, const PrintTableColumnDataType>;

std::string tableColumnDataToStr(const std::size_t start, const std::size_t size,
                                 const PrintTableColumnDataType dataType,
                                 const std::vector<std::uint8_t> &dataRaw)
{
    std::stringstream ss {};
    if (dataType == PrintTableColumnDataType::HIDE) {
        return "";
    }
    if (dataType != PrintTableColumnDataType::NONE) {
        ss << "'";
        for (std::size_t i = start; i < start + size; i++) {
            ss << static_cast<int>(dataRaw.at(i)) << " ";;
        }
        if (dataRaw.size() > 0) {
            ss.seekp(-1, std::ios_base::end);
        }
        ss << "' -> ";
    }
    ss << "'";
    if (dataType == PrintTableColumnDataType::UINT_32) {
        if (dataRaw.size() < start + 4) {
            throw std::runtime_error("32Bit unsigned int BE data must contain 4 byte");
        }
        ss << static_cast<unsigned int>(read32BitUnsignedIntegerLE(dataRaw, start));
    } else if (dataType == PrintTableColumnDataType::UINT_32_BE) {
        if (dataRaw.size() < start + 4) {
            throw std::runtime_error("32Bit unsigned int BE data must contain 4 byte");
        }
        auto finalNum = static_cast<unsigned int>(read32BitUnsignedIntegerLE(dataRaw, start));
        endianSwap(finalNum);
        ss << finalNum;
    } else if (dataType == PrintTableColumnDataType::UINT_16) {
        if (dataRaw.size() < start + 2) {
            throw std::runtime_error("16Bit unsigned int LE data must contain 4 byte");
        }
        ss << static_cast<unsigned int>(read16BitUnsignedIntegerLE(dataRaw, start));
    } else {
        for (std::size_t i = start; i < start + size; i++) {
            if (dataType == PrintTableColumnDataType::NONE || dataType == PrintTableColumnDataType::INT) {
                ss << static_cast<int>(dataRaw.at(i)) << " ";
            }
            if (dataType == PrintTableColumnDataType::CHAR) {
                ss << static_cast<char>(dataRaw.at(i)) << " ";
            }
        }
        if (dataRaw.size() > 0) {
            ss.seekp(-1, std::ios_base::end);
        }
    }
    ss << "'";
    if (dataType != PrintTableColumnDataType::NONE) {
        auto dataTypeStr = "unknown";
        switch (dataType) {
            case PrintTableColumnDataType::CHAR:
                dataTypeStr = "char";
                break;
            case PrintTableColumnDataType::INT:
                dataTypeStr = "int";
                break;
            case PrintTableColumnDataType::UINT_16:
                dataTypeStr = "16Bit unsigned int";
                break;
            case PrintTableColumnDataType::UINT_32:
                dataTypeStr = "32Bit unsigned int";
                break;
            case PrintTableColumnDataType::UINT_32_BE:
                dataTypeStr = "32Bit unsigned int BE";
                break;
            default:
                break;
        }
        ss << " (" << dataTypeStr << ")";
    }
    return ss.str();
}

void printTable(const std::vector<PrintTableColumn> &columns, const std::vector<std::uint8_t> &data)
{
    const std::tuple<std::string, std::string, std::string, std::string> header {"Position", "Size", "Purpose", "Data" };
    // Calculate padding information
    std::size_t maxLengthPosition = std::get<0>(header).length();
    std::size_t maxLengthSize = std::get<1>(header).length();
    std::size_t maxLengthPurpose = std::get<2>(header).length();
    std::size_t maxLengthData = std::get<2>(header).length();
    for (const auto &column : columns) {
        maxLengthPosition = std::max(maxLengthPosition, std::to_string(std::get<0>(column)).length());
        maxLengthSize = std::max(maxLengthSize, std::to_string(std::get<1>(column)).length());
        maxLengthPurpose = std::max(maxLengthPurpose, std::get<2>(column).length());
        maxLengthData = std::max(maxLengthData, tableColumnDataToStr(std::get<0>(column),
                                 std::get<1>(column), std::get<3>(column), data).length());
    }
    std::cout << "| " << padRight(std::get<0>(header), maxLengthPosition, ' ')
              << " | " << padRight(std::get<1>(header), maxLengthSize, ' ')
              << " | " << padRight(std::get<2>(header), maxLengthPurpose, ' ')
              << " | " << padRight(std::get<3>(header), maxLengthData, ' ') << " |" << std::endl;
    std::cout << "| " << padRight("", maxLengthPosition, '-')
              << " | " << padRight("", maxLengthSize, '-')
              << " | " << padRight("", maxLengthPurpose, '-')
              << " | " << padRight("", maxLengthData, '-') << " |" << std::endl;
    for (const auto &column : columns) {
        std::cout << "| " << padRightNumber(std::get<0>(column), maxLengthPosition, ' ')
                  << " | " << padRightNumber(std::get<1>(column), maxLengthSize, ' ')
                  << " | " << padRight(std::get<2>(column), maxLengthPurpose, ' ')
                  << " | " << padRight(tableColumnDataToStr(std::get<0>(column), std::get<1>(column),
                                       std::get<3>(column), data), maxLengthData,
                                       ' ') << " |" << std::endl;
    }
}

/**
 * Sources:
 * - https://docs.fileformat.com/image/png/
 * - http://www.libpng.org/pub/png/spec/1.2/PNG-Structure.html
 *
 * A PNG file consists of a PNG signature followed by a series of chunks.
 * The first eight bytes of a PNG file always contain the following (decimal) values: {{{ 137 80 78 71 13 10 26 10 }}}
 * This signature indicates that the remainder of the file contains a single PNG image, consisting of a series of chunks
 * beginning with an IHDR chunk and ending with an IEND chunk.
 * Each chunk has the following format:
 *   > {4 Bytes=BigEndianUnsignedInt=chunkLength} (Number of byte data of chunk)
 *   > {4 Bytes=4 ASCII chars=chunkType}
 *   > {chunkLength Bytes=chunkData}
 *   > {4 Bytes=crc}
 */
void printPngInformation(const std::vector<uint8_t> &data, const std::size_t start)
{
    if (start + 8 >= data.size()) {
        std::cout << "> data too small to contain png signature!" << std::endl;
        return;
    }
    if (!(data.at(start + 0) == 137 && data.at(start + 1) == 'P' && data.at(start + 2) == 'N' &&
          data.at(start + 3) == 'G' &&
          data.at(start + 4) == 13 && data.at(start + 5) == 10 && data.at(start + 6) == 26 &&
          data.at(start + 7) == 10)) {
        std::cout << "> png signature (the leading 8 bytes) is incorrect!" << std::endl;
        return;
    }

    std::vector<PrintTableColumn> table {};
    table.emplace_back(std::tuple{ start + 0, 8, "PNG signature", PrintTableColumnDataType::NONE });

    for (std::size_t i = start + 8; i < data.size(); i++) {
        if (i + 8 < data.size()) {
            auto chunkSize = static_cast<unsigned int>(read32BitUnsignedIntegerLE(data, i));
            endianSwap(chunkSize);
            const auto chunkDataType = readCharString(data, i + 4, 4);

            table.emplace_back(std::tuple{ i, 4, "chunk size (only data)", PrintTableColumnDataType::UINT_32_BE });
            i += 4;
            table.emplace_back(std::tuple{ i, 4, "chunk type", PrintTableColumnDataType::CHAR });
            i += 4;
            if (chunkSize > 0) {
                table.emplace_back(std::tuple{ i, chunkSize, "chunk data", PrintTableColumnDataType::HIDE });
                i += chunkSize;
            }
            if (i + 4 < data.size()) {
                table.emplace_back(std::tuple{ i, 4, "crc (Cyclic Redundancy Check)", PrintTableColumnDataType::NONE });
                i += 4;
            } else if (chunkDataType != "IEND") {
                std::cout << "crc value missing since the data is too short" << std::endl;
            }
            i -= 1;
        }
    }

    printTable(table, data);
}

struct PngDirectoryHeaderInformation {
    uint8_t width;
    uint8_t height;
    uint8_t colorCount;
    uint16_t planes;
    uint16_t bitCount;
    uint16_t bytesInRes;
    uint16_t imageOffset;
};

std::tuple<PngDirectoryHeaderInformation, std::vector<PrintTableColumn>>
        printIcoDirectoryHeaderInformation(const std::vector<uint8_t> &data,
                const std::size_t start, const int directoryNumber)
{
    std::vector<PrintTableColumn> table {};
    PngDirectoryHeaderInformation pngDirectoryHeaderInformation;
    std::string imgNum = " image #" + std::to_string(directoryNumber);
    pngDirectoryHeaderInformation.width = data.at(start + 0);
    table.emplace_back(std::tuple{ start + 0, 1, "width" + imgNum, PrintTableColumnDataType::INT });
    pngDirectoryHeaderInformation.height = data.at(start + 1);
    table.emplace_back(std::tuple{ start + 1, 1, "height" + imgNum, PrintTableColumnDataType::INT });
    pngDirectoryHeaderInformation.colorCount = data.at(start + 2);
    table.emplace_back(std::tuple{ start + 2, 1, "colorCount", PrintTableColumnDataType::INT });
    table.emplace_back(std::tuple{ start + 3, 1, "reserved", PrintTableColumnDataType::INT });
    pngDirectoryHeaderInformation.planes = read16BitUnsignedIntegerLE(data, start + 4);
    table.emplace_back(std::tuple{ start + 4, 2, "planes", PrintTableColumnDataType::UINT_16 });
    pngDirectoryHeaderInformation.bitCount = read16BitUnsignedIntegerLE(data, start + 6);
    table.emplace_back(std::tuple{ start + 6, 2, "bitCount", PrintTableColumnDataType::UINT_16 });
    pngDirectoryHeaderInformation.bytesInRes = read16BitUnsignedIntegerLE(data, start + 8);
    table.emplace_back(std::tuple{ start + 8, 2, "bytesInRes", PrintTableColumnDataType::UINT_16 });
    pngDirectoryHeaderInformation.imageOffset = read16BitUnsignedIntegerLE(data, start + 12);
    table.emplace_back(std::tuple{ start + 12, 2, "imageOffset", PrintTableColumnDataType::UINT_16 });

    return { pngDirectoryHeaderInformation, table };
}

struct IcoInformation {
    std::vector<PngDirectoryHeaderInformation> directoryHeaders = {};
    std::vector<std::vector<uint8_t>> data = {};
    uint16_t imageType;
    uint16_t imageCount;
};

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
IcoInformation printIcoInformation(const std::vector<uint8_t> &data, const std::size_t start)
{
    std::vector<PrintTableColumn> table {};
    IcoInformation icoInformation;
    // The default header
    table.emplace_back(std::tuple{ start + 0, 2, "reserved", PrintTableColumnDataType::UINT_16 });

    icoInformation.imageType = read16BitUnsignedIntegerLE(data, 2);
    table.emplace_back(std::tuple{ start + 2, 2, "imageType", PrintTableColumnDataType::UINT_16 });
    icoInformation.imageCount = read16BitUnsignedIntegerLE(data, 4);
    table.emplace_back(std::tuple{ start + 4, 2, "imageCount", PrintTableColumnDataType::UINT_16 });

    // The directory headers
    icoInformation.directoryHeaders.resize(icoInformation.imageCount);
    for (int i = 0; i < icoInformation.imageCount; i++) {
        const auto icoDirHeaderInformation = printIcoDirectoryHeaderInformation(data, 6 + (i * 16), i);
        icoInformation.directoryHeaders.at(i) = std::get<0>(icoDirHeaderInformation);
        for (std::size_t j = 0; j < std::get<1>(icoDirHeaderInformation).size(); j++) {
            table.emplace_back(std::get<1>(icoDirHeaderInformation).at(j));
        }
    }
    for (std::size_t i = 0; i < icoInformation.directoryHeaders.size(); i++) {
        table.emplace_back(std::tuple{ start + icoInformation.directoryHeaders.at(i).imageOffset, start + icoInformation.directoryHeaders.at(i).bytesInRes, "image data #" + std::to_string(i), PrintTableColumnDataType::HIDE });
    }

    printTable(table, data);

    return icoInformation;
}

/**
 * Sources:
 * - https://www.gdgsoft.com/anituner/help/aniformat.htm
 * - https://learn.microsoft.com/en-us/windows/win32/xaudio2/resource-interchange-file-format--riff-
 *
 * It is essentially a RIFF container.
 * In comparison to IFF multi-byte integers are in RIFF stored in the LE order.
 *
 * The structure is as follows:
 * - "RIFF" (Where "RIFF" is the literal FOURCC code)
 * - fileSize (a 4-byte value giving the size of the data in the file)
 *   [which includes the size of fileType and data that follows, but does not include the "RIFF"]
 * - fileType (a FOURCC that identifies the specific file type)
 * - data (consists of chunks in any order)
 *
 * Chunks have the following structure:
 * - chunkID (chunkID is a FOURCC that identifies the data contained in the chunk)
 * - chunkSize (a 4-byte value giving the size of the data section of the chunk)
 *   [chunkSize gives the size of the valid! data in the chunk]
 *   [It does not include the padding, the size of chunkID, or the size of chunkSize]
 * - data (data is zero or more bytes of data)
 *   [The data is always padded to the nearest WORD boundary]
 */
void printAniInformation(const std::vector<uint8_t> &data)
{
    std::vector<PrintTableColumn> table {};

    if (12 >= data.size()) {
        std::cout << "> No ani header found (too small)" << std::endl;
        return;
    }
    if (!(data.at(0) == 'R' && data.at(1) == 'I' && data.at(2) == 'F' && data.at(3) == 'F')) {
        std::cout << "> No ani RIFF header found (leading 4 bytes incorrect)" << std::endl;
        return;
    }
    if (!(data.at(8) == 'A' && data.at(9) == 'C' && data.at(10) == 'O' &&
          data.at(11) == 'N')) {
        std::cout << "> The filetype was not ACON in the header" << std::endl;
        return;
    }

    std::size_t i = 0;
    table.emplace_back(std::tuple{ i, 4, "RIFF", PrintTableColumnDataType::CHAR });

    i += 4;
    const auto fileSize = read16BitUnsignedIntegerLE(data, i);
    table.emplace_back(std::tuple{ i, 4, "fileSize", PrintTableColumnDataType::UINT_32 });
    i += 4;
    table.emplace_back(std::tuple{ i, 4, "ACON", PrintTableColumnDataType::CHAR });
    i += 4;
    std::size_t chunkCounter = 0;
    while (i < fileSize - static_cast<std::size_t>(4)) {
        table.emplace_back(std::tuple{ i, 4, "chunk id #" + std::to_string(chunkCounter), PrintTableColumnDataType::CHAR });
        i += 4;
        const auto chunkSize = read16BitUnsignedIntegerLE(data, i);
        table.emplace_back(std::tuple{ i, 4, "chunk size #" + std::to_string(chunkCounter), PrintTableColumnDataType::UINT_32 });
        i += 4;
        table.emplace_back(std::tuple{ i, chunkSize, "chunk data #" + std::to_string(chunkCounter), PrintTableColumnDataType::HIDE });
        i += chunkSize;
        chunkCounter += 1;
    }

    printTable(table, data);
}
