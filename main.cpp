// Copyright @Harry Reblando
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <omp.h>
#include "PNG.h"
 
// It is ok to use the following namespace delarations in C++ source
// files only. They must never be used in header files.
using namespace std;
using namespace std::string_literals;

Pixel computeBackgroundPixel(const PNG& img1, const PNG& mask,
 const int startRow, const int startCol, const int maxRow, const int maxCol);
void drawBox(PNG& png, int row, int col, int width, int height);

std::pair<int, int> checkMatch(const PNG& mainImage, const PNG& mask,
 int startRow, int startCol, const Pixel& avgColor, int tolerance);

void setImageDimensions(const PNG& image, int& height, int& width);
void printMatchInfo(int row, int col, int height, int width);

bool evaluateMatch(int matchCount, int mismatchCount,
 int totalPixels, int matchPercent);
void searchImage(const PNG& mainImage, int mainHeight, int mainWidth,
 const PNG& srchImage, int srchHeight, int srchWidth, int matchPercent,
  int tolerance);
/**
 * This is the top-level method that is called from the main method to 
 * perform the necessary image search operation. 
 * 
 * \param[in] mainImageFile The PNG image in which the specified searchImage 
 * is to be found and marked (for example, this will be "Flag_of_the_US.png")
 * 
 * \param[in] srchImageFile The PNG sub-image for which we will be searching
 * in the main image (for example, this will be "star.png" or "start_mask.png") 
 * 
 * \param[in] outImageFile The output file to which the mainImageFile file is 
 * written with search image file highlighted.
 * 
 * \param[in] isMask If this flag is true then the searchImageFile should 
 * be deemed as a "mask". The default value is false.
 * 
 * \param[in] matchPercent The percentage of pixels in the mainImage and
 * searchImage that must match in order for a region in the mainImage to be
 * deemed a match.
 * 
 * \param[in] tolerance The absolute acceptable difference between each color
 * channel when comparing  
 */
void imageSearch(const std::string& mainImageFile,
                const std::string& srchImageFile, 
                const std::string& outImageFile, const bool isMask = true, 
                const int matchPercent = 75, const int tolerance = 32) {
    PNG mainImage, srchImage;
    mainImage.load(mainImageFile);
    srchImage.load(srchImageFile);

    int mainHeight, mainWidth, srchHeight, srchWidth;
    setImageDimensions(mainImage, mainHeight, mainWidth);
    setImageDimensions(srchImage, srchHeight, srchWidth);
    
    searchImage(mainImage, mainHeight, mainWidth, srchImage, srchHeight,
        srchWidth, matchPercent, tolerance);
}

void searchImage(const PNG& mainImage, int mainHeight, int mainWidth,
 const PNG& srchImage, int srchHeight, int srchWidth, int matchPercent,
  int tolerance) {
    int numberOfMatches = 0;
    for (int row = 0; row <= mainHeight - srchHeight; row++) {
        for (int col = 0; col <= mainWidth - srchWidth; col++) {
            Pixel avgColor = computeBackgroundPixel(mainImage,
             srchImage, row, col, srchHeight, srchWidth);
            std::pair<int, int> matchResult = checkMatch(mainImage, srchImage,
             row, col, avgColor, tolerance);

            if (evaluateMatch(matchResult.first, matchResult.second,
             srchWidth * srchHeight, matchPercent)) {
                printMatchInfo(row, col, srchHeight, srchWidth);
                numberOfMatches++;
                col += srchWidth - 1;
            }
        }
    }
    std::cout << "Number of matches: " << numberOfMatches << std::endl;
}

bool evaluateMatch(int matchCount, int mismatchCount,
 int totalPixels, int matchPercent) {
    float netMatch = matchCount - mismatchCount;
    float requiredMatches = (totalPixels * matchPercent) / 100;
    return netMatch > requiredMatches;
}

void printMatchInfo(int row, int col, int height, int width) {
    std::cout << "sub-image matched at: " << row << ", " << col;
    std::cout << ", " << row + height << ", ";
    std::cout << col + width << std::endl;
}

void setImageDimensions(const PNG& image, int& height, int& width) {
    height = image.getHeight();
    width = image.getWidth();
}

bool isMatch(const Pixel& imgPixel, const Pixel& avgColor, int tolerance) {
    return std::abs(imgPixel.color.red - avgColor.color.red) < tolerance &&
            std::abs(imgPixel.color.green - avgColor.color.green) < tolerance &&
            std::abs(imgPixel.color.blue - avgColor.color.blue) < tolerance;
}

bool isBlack(const Pixel& maskPixel) {
    const Pixel Black{ .rgba = 0xff'00'00'00U };
    return maskPixel.rgba == Black.rgba;
}

std::pair<int, int> checkMatch(const PNG& mainImage, const PNG& mask,
    int startRow, int startCol, const Pixel& avgColor, int tolerance) {
    int matchCount = 0;
    int mismatchCount = 0;

    for (int mRow = 0; mRow < mask.getHeight(); ++mRow) {
        for (int mCol = 0; mCol < mask.getWidth(); ++mCol) {
            Pixel maskPixel = mask.getPixel(mRow, mCol);
            Pixel pixel = mainImage.getPixel(startRow + mRow, startCol + mCol);
            bool black  = isBlack(maskPixel);
            if (black) {  
                if (isMatch(pixel, avgColor, tolerance)) {
                    matchCount++;
                } else {
                    mismatchCount++;
                }
            } else {
                if (!isMatch(pixel, avgColor, tolerance)) {
                    matchCount++;
                } else {
                    mismatchCount++;
                }
            }
        }
    }
    return {matchCount, mismatchCount};
}

Pixel computeBackgroundPixel(const PNG& img1, const PNG& mask,
 const int startRow, const int startCol, 
    const int maxRow, const int maxCol) {
    const Pixel Black{ .rgba = 0xff'00'00'00U };
    int red = 0, blue = 0, green = 0, count = 0;

    for (int row = 0; (row < maxRow); row++) {
        for (int col = 0; col < maxCol; col++) {
            if (mask.getPixel(row, col).rgba == Black.rgba) {
                const auto pix = img1.getPixel(row + startRow, col + startCol); 
                     // Get corresponding pixel from the larger image
                red += pix.color.red;
                green += pix.color.green;
                blue += pix.color.blue;
                count++;
            }
        }
    }

    unsigned char avgRed = 0, avgGreen = 0, avgBlue = 0;
    if (count > 0) {
        avgRed = red / count;
        avgGreen = green / count;
        avgBlue = blue / count;
    }
    return { .color = {avgRed, avgGreen, avgBlue, 0} };
}
void drawBox(PNG& png, int row, int col, int width, int height) {
    // Draw horizontal lines
    for (int i = 0; (i < width); i++) {
        png.setRed(row, col + i);
        png.setRed(row + height, col + i);
    }
    // Draw vertical lines
    for (int i = 0; (i < height); i++) {
        png.setRed(row + i, col);
        png.setRed(row + i, col + width);
    }
}

/**
 * The main method simply checks for command-line arguments and then calls
 * the image search method in this file.
 * 
 * \param[in] argc The number of command-line arguments. This program
 * needs at least 3 command-line arguments.
 * 
 * \param[in] argv The actual command-line arguments in the following order:
 *    1. The main PNG file in which we will be searching for sub-images
 *    2. The sub-image or mask PNG file to be searched-for
 *    3. The file to which the resulting PNG image is to be written.
 *    4. Optional: Flag (True/False) to indicate if the sub-image is a mask 
 *       (deault: false)
 *    5. Optional: Number indicating required percentage of pixels to match
 *       (default is 75)
 *    6. Optiona: A tolerance value to be specified (default: 32)
 */
int main(int argc, char *argv[]) {
    if (argc < 4) {
        // Insufficient number of required parameters.
        std::cout << "Usage: " << argv[0] << " <MainPNGfile> <SearchPNGfile> "
                  << "<OutputPNGfile> [isMaskFlag] [match-percentage] "
                  << "[tolerance]\n";
        return 1;
    }
    const std::string True("true");
    // Call the method that starts off the image search with the necessary
    // parameters.
    imageSearch(argv[1], argv[2], argv[3],       // The 3 required PNG files
        (argc > 4 ? (True == argv[4]) : true),   // Optional mask flag
        (argc > 5 ? std::stoi(argv[5]) : 75),    // Optional percentMatch
        (argc > 6 ? std::stoi(argv[6]) : 32));   // Optional tolerance

    return 0;
}

// End of source code]
