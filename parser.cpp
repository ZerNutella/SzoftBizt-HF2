#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <stdint.h>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>

//Struct to store the CIFF header information
struct CIFFHeader
{
    char magic[4];
    uint64_t headerSize;
    uint64_t contentSize;
    uint64_t width;
    uint64_t height;
    std::string caption;
    std::vector<std::string> tags;
};

//Struct to store the basic CAFF block information
struct CAFFBlock
{
    uint8_t ID;
    uint64_t length;
};

//Struct to store the CAFF header information
struct CAFFHeader
{
    char magic[4];
    uint64_t headerSize;
    uint64_t CIFFnum;
};

//Struct to store the CAFF Credits block information
struct CAFFCredit
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint64_t lengthCreator;
    char *creator;
};

//Struct to store the CAFF Animation block information
struct CAFFAnimation
{
    uint64_t duration;
    CIFFHeader header;
    std::vector<unsigned char> pixels;
};

//Function to read CIFF header from a file
bool readCIFFHeader(const std::string &filename, CIFFHeader &header)
{
    std::ifstream file(filename, std::ios::binary);
    //If the file cannot be opened we return false
    if (!file)
    {
        return false;
    }

    //Read magic
    file.read(header.magic, 4);
    //If the magic doesnt match the required text "CIFF" then we return false
    if(strcmp(header.magic, "CIFF") != 0)
        return false;
    //Read header size
    file.read(reinterpret_cast<char *>(&header.headerSize), sizeof(header.headerSize));
    //Read content size
    file.read(reinterpret_cast<char *>(&header.contentSize), sizeof(header.contentSize));
    //Read width
    file.read(reinterpret_cast<char *>(&header.width), sizeof(header.width));
    //Read height
    file.read(reinterpret_cast<char *>(&header.height), sizeof(header.height));
    //Read caption
    std::getline(file, header.caption, '\n');
    //Read tags
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
//Function to read pixel data from the file
bool readPixelData(const std::string &filename, const CIFFHeader &header, std::vector<unsigned char> &pixels)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file)
    {
        return false;
    }
    //Seek to the beginning of the pixel data
    file.seekg(header.headerSize);
    //Calculate the expected size of the pixel data
    uint64_t expectedPixelDataSize = header.width * header.height * 3;
    //Check if the expected pixel data size matches the content size
    if (expectedPixelDataSize != header.contentSize)
    {
        return false;
    }
    //Resize the pixel vector to hold the pixel data
    pixels.resize(expectedPixelDataSize);
    //Read the pixel data
    file.read(reinterpret_cast<char *>(pixels.data()), expectedPixelDataSize);
    return true;
}

bool saveAsJPEG(const std::string& filename, const CIFFHeader& header, const std::vector<unsigned char>& pixels)
{
    std::string jpegFilename = filename.substr(0, filename.find_last_of('.')) + ".jpg";

    //Save the pixel data as a JPEG file
    int result = stbi_write_jpg(jpegFilename.c_str(), header.width, header.height, 3, pixels.data(), 100);
    if (result == 0)
    {
        return false;
    }
    return true;
}

//Function to read CIFF header from a file already opened and read from, used in processing CAFF files
bool readCIFFHeader(std::ifstream &file, CIFFHeader &header)
{
    //Read magic
    file.read(header.magic, 4);
    //If the magic doesnt match the required text "CIFF" then we return false
    if(strcmp(header.magic, "CIFF") != 0)
        return false;
    //Read header size
    file.read(reinterpret_cast<char *>(&header.headerSize), sizeof(header.headerSize));
    //Read content size
    file.read(reinterpret_cast<char *>(&header.contentSize), sizeof(header.contentSize));
    //Read width
    file.read(reinterpret_cast<char *>(&header.width), sizeof(header.width));
    //Read height
    file.read(reinterpret_cast<char *>(&header.height), sizeof(header.height));
    //Read caption
    std::getline(file, header.caption, '\n');
    //Read tags
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

//Function to read pixel data from the file that was already opened and read from, used in processing CAFF files
bool readPixelData(std::ifstream &file, const CIFFHeader &header, std::vector<unsigned char> &pixels)
{
    //Calculate the expected size of the pixel data
    uint64_t expectedPixelDataSize = header.width * header.height * 3;
    //Check if the expected pixel data size matches the content size
    if (expectedPixelDataSize != header.contentSize)
    {
        return false;
    }
    //Resize the pixel vector to hold the pixel data
    pixels.resize(expectedPixelDataSize/3);
    //Read the pixel data
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
    bool hasHeader = false;
    bool hasCredits = false;
    bool hasAnimation = false;
    while (!file.eof())
    {
        CAFFBlock CaffBlock;
        file.read(reinterpret_cast<char *>(&CaffBlock.ID), sizeof(CaffBlock.ID));
        file.read(reinterpret_cast<char *>(&CaffBlock.length), sizeof(CaffBlock.length));

        switch (CaffBlock.ID)
        {
        //The HeaderBlock of the CAFF file
        case 0x1:
        {
            //Read HeaderBlock
            //If a HeaderBlock has already been read then return false, there shouldnt be more than one header block
            if(hasHeader)
                return false;
            CAFFHeader header;
            file.read(header.magic, 4);
            //If the magic doesnt match the required text "CAFF" then we return false
            if(strcmp(header.magic, "CAFF") != 0)
                return false;
            //Process the header block data
            file.read(reinterpret_cast<char *>(&header.headerSize), sizeof(header.headerSize));
            file.read(reinterpret_cast<char *>(&header.CIFFnum), sizeof(header.CIFFnum));
            hasHeader = true;
            break;
        }
        //The CreditsBlock
        case 0x2:
        {
            //Read CreditsBlock
            CAFFCredit credits;
            //Read the year, month, day, hour and minute of the creation of the CAFF
            file.read(reinterpret_cast<char *>(&credits.year), sizeof(credits.year));
            file.read(reinterpret_cast<char *>(&credits.month), sizeof(credits.month));
            file.read(reinterpret_cast<char *>(&credits.day), sizeof(credits.day));
            file.read(reinterpret_cast<char *>(&credits.hour), sizeof(credits.hour));
            file.read(reinterpret_cast<char *>(&credits.minute), sizeof(credits.minute));
            //Read the length of the creator's name
            file.read(reinterpret_cast<char *>(&credits.lengthCreator), sizeof(credits.lengthCreator));
            //Read the creator's name
            credits.creator = new char[credits.lengthCreator + 1];
            file.read(credits.creator, credits.lengthCreator);
            credits.creator[credits.lengthCreator] = '\0';

            //Clean up dynamically allocated memory
            delete[] credits.creator;
            hasCredits = true;
            break;
        }
        //The AnimationBlock
        case 0x3:
        {

            //Read AnimationBlock
            CAFFAnimation animation;
            //Read the duration of the animation from the file
            file.read(reinterpret_cast<char *>(&animation.duration), sizeof(animation.duration));
            //Try to read the header from the CIFF part of the CAFF
            if (!readCIFFHeader(file, animation.header))
                return false;
            //Try to read the pixel data from the CIFF part of the CAFF
            if (!readPixelData(file, animation.header, animation.pixels))
                return false;
            //Just to make sure we generate only the first frame as the task asked us to do
            if(!hasAnimation){
                if (!saveAsJPEG(filename, animation.header, animation.pixels))
                    return false;
            }
            break;
        }
        default:
            //There shouldn't be any other type of blocks so we return false
            return false;
            break;
        }
    }
    //If the file contained all of the required blocks then we return successfully
    return hasAnimation && hasCredits && hasHeader;
}

bool isPartOf(char *w1, char *w2)
{
    int i = 0;
    int j = 0;

    //Go until the end of the first word (search area)
    while (w1[i] != '\0')
    {
        //If the current letter in the first word matches the the first letter of the searched word, then...
        if (w1[i] == w2[j])
        {
            //Check to see if the following letters also match
            while (w1[i] == w2[j] && w2[j] != '\0')
            {
                j++;
                i++;
            }
            //If we get to the end of the searched word then we found it
            if (w2[j] == '\0')
            {
                return true;
            }
            //If not then start from the first letter of the searched word again
            j = 0;
        }
        i++;
    }
    return false;
}

int main(int argc, char *argv[])
{
    //Not enough arguments
    if (argc != 3)
    {
        return -1;
    }
    //If the arguments are for a ciff file
    if (strcmp(argv[1], "-ciff") == 0 && isPartOf(argv[2], (char*)".ciff"))
    {
        CIFFHeader header;
        std::vector<unsigned char> pixels;
        //Read the CIFF file's header
        if(!readCIFFHeader(argv[2], header))
            return -1;
        //Read the pixels of the file
        if(!readPixelData(argv[2], header, pixels))
            return -1;
        //Save it as a JPEG
        if(!saveAsJPEG(argv[2], header, pixels))
            return -1;
    } 
    //If the arguments are for a CAFF file
    else if(strcmp(argv[1], "-caff") == 0 && isPartOf(argv[2], (char*)".caff")){
        //Read the CAFF File
        if(!readCAFF(argv[2]))
            return -1;
    }
    return 0;
}