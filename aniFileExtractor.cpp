// Inspired by https://github.com/Mastermindzh/Scripts/blob/master/c%2B%2B/ani2png.c

#include "aniFileExtractor.hpp"
#include "printFileInformation.hpp"

int main(int argc, const char **argv)
{
    std::string filePathString;
    if (argc >= 1) {
        filePathString =  argv[1] ;
    }
    if (argc == 3) {
        if (filePathString == "ani") {
            filePathString =  argv[2] ;
            printAniInformation(readBinaryFile(filePathString));
        } else if (filePathString == "ico") {
            filePathString =  argv[2] ;
            printIcoInformation(readBinaryFile(filePathString), 0);
        } else  if (filePathString == "png") {
            filePathString =  argv[2] ;
            printPngInformation(readBinaryFile(filePathString), 0);
        } else {
            // Assume that the images and other information should be extracted
            // into a separate directory
            const std::filesystem::path outDir = { argv[2] };
            if (!std::filesystem::exists(outDir)) {
                std::filesystem::create_directory(outDir);
            }
            std::filesystem::path filePath = filePathString;
            std::filesystem::path imageOutputFilePathPrefix = outDir / filePathString;
            if (filePath.has_extension()) {
                imageOutputFilePathPrefix = outDir / filePath.stem();
            }
            const auto dataBytes = readBinaryFile(filePath);
            const auto aniFileInformation = readAniFileInformation(dataBytes);
            std::string x11cursorConfigTemplate {};
            for (std::size_t iconCounter = 0; iconCounter < aniFileInformation.icons.size(); iconCounter++) {
                const auto pngDataNew = aniFileInformation.icons.at(iconCounter);
                const auto icoInformation = printIcoInformation(pngDataNew, 0);
                writeBinaryFile(imageOutputFilePathPrefix.string() + "_" + std::to_string(iconCounter) + ".ico",
                                pngDataNew);
                x11cursorConfigTemplate.append(std::to_string(icoInformation.directoryHeaders.at(
                                                   0).width) + " 2 4 " + filePath.stem().string() + "_" + std::to_string(
                                                   iconCounter) + ".png TODO_MS\n");
            }
            writeTextFile(outDir / (filePath.stem().string() + "_template.cursor"), x11cursorConfigTemplate);
        }
    } else {
        std::cout << "$ ani2png FILE.ani PNG_FILE_OUTPUT_DIR\n"
                  << "$ ani2png ani FILE.ani\n"
                  << "$ ani2png ico FILE.ico\n"
                  << "$ ani2png png FILE.png" << std::endl;
        return -1;
    }
    return 0;
}
