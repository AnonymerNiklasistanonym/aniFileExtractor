#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <variant>

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

using PrintTableColumnDataInternal = std::tuple<std::string, std::vector<std::uint8_t>>;
using PrintTableColumnData = std::variant<std::string, PrintTableColumnDataInternal>;
using PrintTableColumn = std::tuple<std::size_t, std::size_t, std::string, PrintTableColumnData>;

std::string tableColumnDataToStr(const PrintTableColumnData &data)
{
    if (std::holds_alternative<std::string>(data)) {
        return std::get<std::string>(data);
    }
    const auto dataInternal = std::get<PrintTableColumnDataInternal>(data);
    const auto dataType = std::get<0>(dataInternal);
    const auto dataRaw = std::get<1>(dataInternal);
    std::stringstream ss {};
    if (dataType != "-") {
        ss << "'";
        for (const auto &dataRawElement : dataRaw) {
            ss << static_cast<int>(dataRawElement) << " ";;
        }
        if (dataRaw.size() > 0) {
            ss.seekp(-1, std::ios_base::end);
        }
        ss << "' -> ";
    }
    ss << "'";
    if (dataType == "32Bit unsigned int") {
        if (dataRaw.size() < 4) {
            throw std::runtime_error("32Bit unsigned int BE data must contain 4 byte");
        }
        ss << static_cast<unsigned int>(read32BitUnsignedIntegerLE(dataRaw, 0));
    } else if (dataType == "32Bit unsigned int BE") {
        if (dataRaw.size() < 4) {
            throw std::runtime_error("32Bit unsigned int BE data must contain 4 byte");
        }
        auto finalNum = static_cast<unsigned int>(read32BitUnsignedIntegerLE(dataRaw, 0));
        endianSwap(finalNum);
        ss << finalNum;
    } else if (dataType == "16Bit unsigned int") {
        if (dataRaw.size() < 2) {
            throw std::runtime_error("16Bit unsigned int LE data must contain 4 byte");
        }
        ss << static_cast<unsigned int>(read16BitUnsignedIntegerLE(dataRaw, 0));
    } else {
        for (const auto &dataRawElement : dataRaw) {
            if (dataType == "-") {
                ss << static_cast<int>(dataRawElement) << " ";
            }
            if (dataType == "int") {
                ss << static_cast<int>(dataRawElement) << " ";
            }
            if (dataType == "char") {
                ss << static_cast<char>(dataRawElement) << " ";
            }
        }
        if (dataRaw.size() > 0) {
            ss.seekp(-1, std::ios_base::end);
        }
    }
    ss << "'";
    if (dataType != "-") {
        ss << " (" << dataType << ")";
    }
    return ss.str();
}

void printTable(const std::vector<PrintTableColumn> &columns)
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
        maxLengthData = std::max(maxLengthData, tableColumnDataToStr(std::get<3>(column)).length());
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
                  << " | " << padRight(tableColumnDataToStr(std::get<3>(column)), maxLengthData,
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
    table.emplace_back(std::tuple{ start + 0, 8, "PNG signature", std::tuple{ "-", std::vector(&data[start + 0], &data[start + 8]) } });

    for (std::size_t i = start + 8; i < data.size(); i++) {
        if (i + 8 < data.size()) {
            auto chunkSize = static_cast<unsigned int>(read32BitUnsignedIntegerLE(data, i));
            endianSwap(chunkSize);
            const auto chunkDataType = readCharString(data, i + 4, 4);

            table.emplace_back(std::tuple{ i, 4, "chunk size (only data)", std::tuple{"32Bit unsigned int BE", std::vector(&data[i], &data[i + 4]) } });
            i += 4;
            table.emplace_back(std::tuple{ i, 4, "chunk type", std::tuple{"char", std::vector(&data[i], &data[i + 4]) } });
            i += 4;
            if (chunkSize > 0) {
                table.emplace_back(std::tuple{ i, chunkSize, "chunk data", "TODO" });
                i += chunkSize;
            }
            if (i + 4 < data.size()) {
                table.emplace_back(std::tuple{ i, 4, "crc (Cyclic Redundancy Check)", std::tuple{ "-", std::vector(&data[i], &data[i + 4]) } });
                i += 4;
            } else if (chunkDataType != "IEND") {
                std::cout << "crc value missing since the data is too short" << std::endl;
            }
            i -= 1;
        }
    }

    printTable(table);
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

std::tuple<PngDirectoryHeaderInformation, const std::vector<PrintTableColumn>>
        printIcoDirectoryHeaderInformation(const std::vector<uint8_t> &data,
                const std::size_t start, const int directoryNumber)
{
    std::vector<PrintTableColumn> table {};
    PngDirectoryHeaderInformation pngDirectoryHeaderInformation;
    std::string imgNum = " image #" + std::to_string(directoryNumber);
    pngDirectoryHeaderInformation.width = data.at(start + 0);
    table.emplace_back(std::tuple{ start + 0, 1, "width" + imgNum, std::tuple{ "int", std::vector{ pngDirectoryHeaderInformation.width } } });
    pngDirectoryHeaderInformation.height = data.at(start + 1);
    table.emplace_back(std::tuple{ start + 1, 1, "height" + imgNum, std::tuple{ "int", std::vector{ pngDirectoryHeaderInformation.height } } });
    pngDirectoryHeaderInformation.colorCount = data.at(start + 2);
    table.emplace_back(std::tuple{ start + 2, 1, "colorCount", std::tuple{ "int", std::vector{ pngDirectoryHeaderInformation.colorCount } } });
    table.emplace_back(std::tuple{ start + 3, 1, "reserved", std::tuple{ "int", std::vector{ data.at(start + 3) } } });
    pngDirectoryHeaderInformation.planes = read16BitUnsignedIntegerLE(data, start + 4);
    table.emplace_back(std::tuple{ start + 4, 2, "planes", std::tuple{ "16Bit unsigned int", std::vector(&data[start + 4], &data[start + 6]) } });
    pngDirectoryHeaderInformation.bitCount = read16BitUnsignedIntegerLE(data, start + 6);
    table.emplace_back(std::tuple{ start + 6, 2, "bitCount", std::tuple{ "16Bit unsigned int", std::vector(&data[start + 6], &data[start + 8]) } });
    pngDirectoryHeaderInformation.bytesInRes = read16BitUnsignedIntegerLE(data, start + 8);
    table.emplace_back(std::tuple{ start + 8, 2, "bytesInRes", std::tuple{ "16Bit unsigned int", std::vector(&data[start + 8], &data[start + 10]) } });
    pngDirectoryHeaderInformation.imageOffset = read16BitUnsignedIntegerLE(data, start + 12);
    table.emplace_back(std::tuple{ start + 12, 2, "imageOffset", std::tuple{ "16Bit unsigned int", std::vector(&data[start + 12], &data[start + 14]) } });

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
    table.emplace_back(std::tuple{ start + 0, 2, "reserved", std::tuple{ "16Bit unsigned int", std::vector(&data[start + 0], &data[start + 2]) } });

    icoInformation.imageType = read16BitUnsignedIntegerLE(data, 2);
    table.emplace_back(std::tuple{ start + 2, 2, "imageType", std::tuple{ "16Bit unsigned int", std::vector(&data[start + 2], &data[start + 4]) } });
    icoInformation.imageCount = read16BitUnsignedIntegerLE(data, 4);
    table.emplace_back(std::tuple{ start + 4, 2, "imageCount", std::tuple{ "16Bit unsigned int", std::vector(&data[start + 4], &data[start + 6]) } });

    // The directory headers
    icoInformation.directoryHeaders.resize(icoInformation.imageCount);
    for (int i = 0; i < icoInformation.imageCount; i++) {
        const auto icoDirHeaderInformation = printIcoDirectoryHeaderInformation(data, 6 + (i * 16), i);
        icoInformation.directoryHeaders.at(i) = std::get<0>(icoDirHeaderInformation);
        table.insert(table.end(), std::get<1>(icoDirHeaderInformation).begin(),
                     std::get<1>(icoDirHeaderInformation).end());
    }
    for (std::size_t i = 0; i < icoInformation.directoryHeaders.size(); i++) {
        table.emplace_back(std::tuple{ start + icoInformation.directoryHeaders.at(i).imageOffset, start + icoInformation.directoryHeaders.at(i).bytesInRes, "image data #" + std::to_string(i), "TODO" });
    }

    printTable(table);

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
    table.emplace_back(std::tuple{ i, 4, "RIFF", std::tuple{"char", std::vector(&data[i], &data[i + 4]) } });
    i += 4;
    const auto fileSize = read16BitUnsignedIntegerLE(data, i);
    table.emplace_back(std::tuple{ i, 4, "fileSize", std::tuple{ "32Bit unsigned int", std::vector(&data[i], &data[i + 4]) } });
    i += 4;
    table.emplace_back(std::tuple{ i, 4, "ACON", std::tuple{"char", std::vector(&data[i], &data[i + 4]) } });
    i += 4;
    std::size_t chunkCounter = 0;
    while (i < fileSize - static_cast<std::size_t>(4)) {
        table.emplace_back(std::tuple{ i, 4, "chunk id #" + std::to_string(chunkCounter), std::tuple{"char", std::vector(&data[i], &data[i + 4]) } });
        i += 4;
        const auto chunkSize = read16BitUnsignedIntegerLE(data, i);
        table.emplace_back(std::tuple{ i, 4, "chunk size #" + std::to_string(chunkCounter), std::tuple{ "32Bit unsigned int", std::vector(&data[i], &data[i + 4]) } });
        i += 4;
        table.emplace_back(std::tuple{ i, chunkSize, "chunk data #" + std::to_string(chunkCounter), "TODO" });
        i += chunkSize;
        chunkCounter += 1;
    }

    printTable(table);
}
