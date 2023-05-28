#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdint.h>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>

// Struct to store the CIFF header information
struct CIFFHeader
{
    char magic[4];
    long long headerSize;
    long long contentSize;
    long long width;
    long long height;
    std::string caption;
    std::vector<std::string> tags;
};

struct CAFFBlock
{
    uint8_t ID;
    long long length;
};

struct CAFFHeader
{
    char magic[4];
    long long headerSize;
    long long CIFFnum;
};

struct CAFFCredit
{
    short year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    long long lengthCreator;
    char *creator;
};

struct CAFFAnimation
{
    long long duration;
    CIFFHeader header;
    std::vector<unsigned char> pixels;
};

// Function to read CIFF header from a file
bool readCIFFHeader(const std::string &filename, CIFFHeader &header)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        return false;
    }

    // Read magic
    file.read(header.magic, 4);
    // Read header size
    file.read(reinterpret_cast<char *>(&header.headerSize), sizeof(header.headerSize));
    // Read content size
    file.read(reinterpret_cast<char *>(&header.contentSize), sizeof(header.contentSize));
    // Read width
    file.read(reinterpret_cast<char *>(&header.width), sizeof(header.width));
    // Read height
    file.read(reinterpret_cast<char *>(&header.height), sizeof(header.height));
    // Read caption
    std::getline(file, header.caption, '\n');
    // Read tags
    char c;
    std::string tag;
    while (file.get(c))
    {
        if (c == '\0')
        {
            header.tags.push_back(tag);
            tag.clear();
        }
        else
        {
            tag += c;
        }
    }
    return true;
}
// Function to read pixel data from the file
bool readPixelData(const std::string &filename, const CIFFHeader &header, std::vector<unsigned char> &pixels)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        return false;
    }
    // Seek to the beginning of the pixel data
    file.seekg(header.headerSize);
    // Calculate the expected size of the pixel data
    long long expectedPixelDataSize = header.width * header.height * 3;
    // Check if the expected pixel data size matches the content size
    if (expectedPixelDataSize != header.contentSize)
    {
        return false;
    }
    // Resize the pixel vector to hold the pixel data
    pixels.resize(expectedPixelDataSize);
    // Read the pixel data
    file.read(reinterpret_cast<char *>(pixels.data()), expectedPixelDataSize);
    return true;
}

bool saveAsJPEG(const std::string& filename, const CIFFHeader& header, const std::vector<unsigned char>& pixels)
{
    std::string jpegFilename = filename.substr(0, filename.find_last_of('.')) + ".jpg";

    // Save the pixel data as a JPEG file
    int result = stbi_write_jpg(jpegFilename.c_str(), header.width, header.height, 3, pixels.data(), 100);
    if (result == 0)
    {
        return false;
    }
    return true;
}

// Function to read CIFF header from a file
bool readCIFFHeader(std::ifstream &file, CIFFHeader &header)
{
    // Read magic
    file.read(header.magic, 4);
    // Read header size
    file.read(reinterpret_cast<char *>(&header.headerSize), sizeof(header.headerSize));
    // Read content size
    file.read(reinterpret_cast<char *>(&header.contentSize), sizeof(header.contentSize));
    // Read width
    file.read(reinterpret_cast<char *>(&header.width), sizeof(header.width));
    // Read height
    file.read(reinterpret_cast<char *>(&header.height), sizeof(header.height));
    // Read caption
    std::getline(file, header.caption, '\n');
    // Read tags
    char c;
    std::string tag;
    while (file.get(c))
    {
        if (c == '\0')
        {
            header.tags.push_back(tag);
            tag.clear();
        }
        else
        {
            tag += c;
        }
    }
    return true;
}

// Function to read pixel data from the file
bool readPixelData(std::ifstream &file, const CIFFHeader &header, std::vector<unsigned char> &pixels)
{
    // Calculate the expected size of the pixel data
    long long expectedPixelDataSize = header.width * header.height * 3;
    // Check if the expected pixel data size matches the content size
    if (expectedPixelDataSize != header.contentSize)
    {
        return false;
    }
    // Resize the pixel vector to hold the pixel data
    pixels.resize(expectedPixelDataSize/3);
    // Read the pixel data
    file.read(reinterpret_cast<char *>(pixels.data()), expectedPixelDataSize);
    return true;
}

bool readCAFF(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        return false;
    }
    while (!file.eof())
    {
        uint8_t blockId;
        file.read(reinterpret_cast<char *>(&blockId), sizeof(blockId));

        switch (blockId)
        {
        case 0x1:
        {
            // Read HeaderBlock
            CAFFHeader header;
            file.read(header.magic, 4);
            // Process the header block data
            file.read(reinterpret_cast<char *>(&header.headerSize), sizeof(header.headerSize));
            file.read(reinterpret_cast<char *>(&header.CIFFnum), sizeof(header.CIFFnum));
            break;
        }
        case 0x2:
        {
            // Read CreditsBlock
            CAFFCredit credits;
            file.read(reinterpret_cast<char *>(&credits.year), sizeof(credits.year));
            file.read(reinterpret_cast<char *>(&credits.month), sizeof(credits.month));
            file.read(reinterpret_cast<char *>(&credits.day), sizeof(credits.day));
            file.read(reinterpret_cast<char *>(&credits.hour), sizeof(credits.hour));
            file.read(reinterpret_cast<char *>(&credits.minute), sizeof(credits.minute));
            file.read(reinterpret_cast<char *>(&credits.lengthCreator), sizeof(credits.lengthCreator));
            // Read the creator's name
            credits.creator = new char[credits.lengthCreator + 1];
            file.read(credits.creator, credits.lengthCreator);
            credits.creator[credits.lengthCreator] = '\0';

            // Clean up dynamically allocated memory
            delete[] credits.creator;

            break;
        }
        case 0x3:
        {
            // Read AnimationBlock
            CAFFAnimation animation;
            file.read(reinterpret_cast<char *>(&animation.duration), sizeof(animation.duration));
            if (!readCIFFHeader(file, animation.header))
                return false;
            if (!readPixelData(file, animation.header, animation.pixels))
                return false;
            if (!saveAsJPEG(filename, animation.header, animation.pixels))
                return true;
            break;
        }
        default:
            break;
        }
    }
    return true;
}

bool isPartOf(char *w1, char *w2)
{
    int i = 0;
    int j = 0;

    while (w1[i] != '\0')
    {
        if (w1[i] == w2[j])
        {
            while (w1[i] == w2[j] && w2[j] != '\0')
            {
                j++;
                i++;
            }
            if (w2[j] == '\0')
            {
                return true;
            }
            j = 0;
        }
        i++;
    }
    return false;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        return -1;
    }
    if (strcmp(argv[1], "-ciff") == 0 && isPartOf(argv[2], (char*)".ciff"))
    {
        CIFFHeader header;
        std::vector<unsigned char> pixels;
        if(!readCIFFHeader(argv[2], header))
            return -1;
        if(!readPixelData(argv[2], header, pixels))
            return -1;
        if(!saveAsJPEG(argv[2], header, pixels))
            return -1;
    }else if(strcmp(argv[1], "-caff") == 0 && isPartOf(argv[2], (char*)".caff")){
        if(!readCAFF(argv[2]))
            return -1;
    }
    return 0;
}