// BadEncoder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
//this thingy handles only 1024 x 768 images

#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include "stb_image.h"
#include <string>
#include <vector>
#include <fstream>
#define SCREENWIDTH 1024
#define SCREENHEIGHT 768
#define BIAS         120
#define MAX_FRAMES   600
// 12 x 30
//bias at 32    delivers 13,111kb
//bias at 96    delivers 12,245kb
//bias at 120   delivers 11,799kb

std::string fileDirectory;

    namespace fs = std::filesystem;

    void write_10bit_stream(const std::vector<uint16_t>& flip_coords, const std::string& filename) {
        std::ofstream out(filename, std::ios::binary);
        if (!out) {
            std::cerr << "Failed to open output file: " << filename << "\n";
            return;
        }

        uint32_t accumulator = 0;
        int bits_in_accumulator = 0;

        for (uint16_t word : flip_coords) {
            // Shift accumulator left 10 bits and append the masked 10-bit word
            accumulator = (accumulator << 10) | (word & 0x3FF);
            bits_in_accumulator += 10;

            // Whenever we have at least 8 bits, extract the highest 8 and write a byte
            while (bits_in_accumulator >= 8) {
                bits_in_accumulator -= 8; // Adjust count first to shift correctly
                uint8_t byte_out = (accumulator >> bits_in_accumulator) & 0xFF;
                out.put(byte_out);
            }
        }

        // Flush any remaining bits left in the accumulator at the end of the vector
        // We shift them left to align with the MSB of the final byte (padding with 0s)
        if (bits_in_accumulator > 0) {
            uint8_t final_byte = (accumulator << (8 - bits_in_accumulator)) & 0xFF;
            out.put(final_byte);
        }

        std::cout << "Successfully packed " << flip_coords.size()
            << " coordinates into continuous bits.\n";
    }

    int main()
    {
        std::cout << "paste file path below! make sure there are no spaces in the path" << std::endl;
        std::cin >> fileDirectory;
        std::cout << "starting to parse!" << std::endl;
        int framecount = 0;

        std::vector<fs::path> frames;
        for (const auto& entry : fs::directory_iterator(fileDirectory)) {
            if (entry.path().extension() == ".png") {
                frames.push_back(entry.path());
            }
        }
        std::sort(frames.begin(), frames.end());

        std::cout << "Found " << frames.size() << " frames.\n";
        //ok here we have framecount
        //blender frames are marked 0000,0001 etc

        std::vector<uint16_t> offset; //only use 10 bits in here
        offset.reserve(10000);
        bool currentIsBlack = true;
        uint32_t pixelIterator = 0;
        int frameit = 0;
        for (const auto& filepath : frames) {

            std::cout << "parsing frame " + std::to_string(frameit) <<std::endl;
            frameit++;
            if (frameit > MAX_FRAMES)
            {
                break;
            }
            int width, height, original_channels;

            // Force 1 channel (grayscale) load
            uint8_t* raw_pixels = stbi_load(filepath.string().c_str(), &width, &height, &original_channels, 1);

            if ((width != SCREENWIDTH) || (height != SCREENHEIGHT))
            {
                std::cerr << "incorrect image dimensions!" << std::endl;
                continue;
            }


            if (!raw_pixels) {
                std::cerr << "Failed to load: " << filepath << "\n";
                continue;
            }
            //encoding logic here
            //pixels are in linear memory
            //the processor defaults to black
            
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    uint8_t pixel = raw_pixels[y * width + x];

                    bool pixel_should_be_black = currentIsBlack;
                    if (currentIsBlack && pixel > (127 + BIAS)) {
                        pixel_should_be_black = false;
                    }
                    else if (!currentIsBlack && pixel < (127 - BIAS)) {
                        pixel_should_be_black = true;
                    }

                    if (currentIsBlack != pixel_should_be_black) {
                        // FLIP EVENT
                        offset.push_back(pixelIterator);
                        currentIsBlack = pixel_should_be_black;
                        pixelIterator = 1; // CRITICAL FIX: Count this pixel for the new color
                    }
                    else {
                        // CONTINUOUS RUN
                        pixelIterator++;
                        if (pixelIterator == 1023) {
                            offset.push_back(1023); // Hardware overflow token
                            pixelIterator = 0;      // Next pixel starts fresh at 1
                        }
                    }
                }
            }
            stbi_image_free(raw_pixels);
        }
        if (pixelIterator > 0) {
            offset.push_back(pixelIterator);
        }
        std::cout << "Finished parsing!" << std::endl;
        offset.shrink_to_fit();
        write_10bit_stream(offset, "compressedFile.bin");
        return 0;
    }
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
