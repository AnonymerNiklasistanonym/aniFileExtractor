// Inspired by https://github.com/Mastermindzh/Scripts/blob/master/c%2B%2B/ani2png.c

#include "aniFileExtractor.hpp"

int main(int argc, const char **argv)
{
    std::string filePathString;
    if (argc >= 1) {
        filePathString = argv[1];
    }
    if (argc == 3) {
        if (filePathString == "ani") {
            //filePathString = argv[2];
            //const auto dataBytes = readBinaryFile(filePathString);
            // Print ani information
            //printAniInformation(dataBytes);
        }
        else if (filePathString == "ico") {
            filePathString = argv[2];
            const auto dataBytes = readBinaryFile(filePathString);
            // Print ico information
            printIcoInformation(dataBytes);
        }
        else  if (filePathString == "png") {
            filePathString = argv[2];
            const auto dataBytes = readBinaryFile(filePathString);
            // Print png information
            printPngInformation(dataBytes);
        }
        else {
            // Assume that the images and other information should be extracted
            // into a separate directory
            const std::string directoryPathString = argv[2];
            std::filesystem::path outDir = directoryPathString;
            std::filesystem::path filePath = filePathString;
            std::filesystem::path basenameFilePath = outDir / filePathString;
            if (filePath.has_extension()) {
                basenameFilePath = outDir / filePath.stem();
            }
            if (!std::filesystem::exists(outDir)) {
                std::filesystem::create_directory(outDir);
            }
            const auto dataBytes = readBinaryFile(filePath);
            const auto aniFileInformation = readAniFileInformation(dataBytes);
            for (std::size_t iconCounter = 0; iconCounter < aniFileInformation.icons.size(); iconCounter++) {
                const auto pngDataNew = aniFileInformation.icons.at(iconCounter);
                writeBinaryFile(basenameFilePath.string() + "_" + std::to_string(iconCounter) + ".png", pngDataNew);
            }
        }
    }
    else {
        std::cout << "$ ani2png FILE.ani PNG_FILE_OUTPUT_DIR\n"
                  << "$ ani2png ani FILE.ani\n"
                  << "$ ani2png ico FILE.ico\n"
                  << "$ ani2png png FILE.png" << std::endl;
        return -1;
    }
    return 0;
}
