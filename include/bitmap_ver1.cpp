// https://any-programming.hatenablog.com/entry/2017/04/27/110723

#ifndef _BITMAP_HPP_
#define _BITMAP_HPP_

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <memory>           
#include <stdexcept>        // std::runtime_error
#include <cstring>          // std::memmove, std::strcmp

namespace Utility
{

using u_short = unsigned short;
using u_long = unsigned long;
using u_int = unsigned int;

class Bitmap
{
private:

    static const int HEADER_SIZE = 54;          // ヘッダのサイズ 54 = 14 + 40
    static const int INFO_HEADER_SIZE = 40;     // 情報ヘッダのサイズ
    static const int PALLET_SIZE = 1024;        // パレットのサイズ
    static const int MAX_WIDTH = 1000;          // 幅(pixel)の上限
    static const int MAX_HEIGHT = 1000;         // 高さ(pixel) の上限

    struct Header
    {
        u_short file_type;                      // ファイルタイプ "BM"                 
        u_long file_size;                       // bmpファイルのサイズ (バイト)        
        u_int info_header_size;                 // 情報ヘッダのサイズ = 40             
        u_int header_size;                      // ヘッダサイズ = 54                                        
        u_short planes;                         // プレーン数 常に 1                    
        u_short color;                          // 色 (ビット)     24                   
        long compress;                          // 圧縮方法         0                   
        long image_size;                        // 画像部分のファイルサイズ (バイト)
        long x_ppm;                             // 水平解像度 (ppm)
        long y_ppm;                             // 垂直解像度 (ppm)
    };

    struct Image
    {
        struct Color  
        {
            u_char red;
            u_char green;
            u_char blue;
        };

        int32_t height;
        int32_t width;
        Color data[MAX_HEIGHT][MAX_WIDTH];
    };

    Header header_;
    Image* image_;

    explicit Bitmap(const Header& header, Image* image)
     : header_(header), image_(image)
    {}

public:

    ~Bitmap()
    {
        delete image_;
    }

    static Bitmap load_bmp(const std::string& file_path)
    {
        std::FILE* file_ptr = std::fopen(file_path.c_str(), "rb");
        Header header;
        Image* image = new Image;
        int real_width;
        u_char* row_data;
        u_char header_buf[HEADER_SIZE];

        if(!file_ptr)
            throw std::runtime_error("File could not open for read.");

        std::fread(header_buf, sizeof(u_char), HEADER_SIZE, file_ptr);
        std::memmove(&header.file_type, header_buf, sizeof(header.file_type));
        std::memmove(&header.file_size, header_buf+2, sizeof(header.file_size));
        std::memmove(&header.header_size, header_buf+10, sizeof(header.header_size));
        std::memmove(&header.info_header_size, header_buf+14, sizeof(header.info_header_size));
        std::memmove(&image->width, header_buf+18, sizeof(image->width));
        std::memmove(&image->height, header_buf+22, sizeof(image->height));
        std::memmove(&header.planes, header_buf+26, sizeof(header.planes));
        std::memmove(&header.color, header_buf+28, sizeof(header.color));
        std::memmove(&header.image_size, header_buf+28, sizeof(header.image_size));
        std::memmove(&header.x_ppm, header_buf+38, sizeof(header.x_ppm));
        std::memmove(&header.y_ppm, header_buf+42, sizeof(header.y_ppm));

        if (header.file_type != 0x4d42)
            throw std::runtime_error("File is not Bitmap file.");
        if (header.color!=24)
            throw std::runtime_error("Color format must be 24 byte length.");
        if (image->width > MAX_WIDTH)
            throw std::runtime_error("Image width is too large.");
        if (image->height > MAX_HEIGHT)
            throw std::runtime_error("Image Height is too large.");

        real_width = image->width*3 + image->width%4;
        row_data = new u_char[real_width]();

        for(auto i=0; i<image->height; i++) 
        {
            std::fread(row_data, sizeof(u_char), real_width, file_ptr);            
            for (auto j=0; j<image->width; j++) 
            {
                image->data[image->height-i-1][j].blue = row_data[j*3];
                image->data[image->height-i-1][j].green = row_data[j*3+1];
                image->data[image->height-i-1][j].red = row_data[j*3+2];
            } 
        }

        std::fclose(file_ptr);
        delete[] row_data;

        auto bitmap = Bitmap(header, image);
        return bitmap;
    }

    void to_bmp(const std::string& file_path)
    {
        std::FILE* file_ptr = std::fopen(file_path.c_str(), "wb");
        int real_width;
        u_char* row_data;
        u_char header_buf[HEADER_SIZE];
        std::memset(header_buf, 0, sizeof(header_buf));

        if(file_ptr==nullptr)
            throw std::runtime_error("file could not be opened.");

        header_.color = 24;
        header_.header_size = HEADER_SIZE;
        header_.info_header_size = 40;
        header_.planes = 1;

        real_width = image_->width*3 + image_->width%4;
        row_data = new u_char[real_width]();
        header_.x_ppm = 0;
        header_.y_ppm = 0;
        header_.image_size = image_->height * real_width;
        header_.file_size = header_.image_size + HEADER_SIZE;

        std::memmove(header_buf, &header_.file_type, sizeof(header_.file_type));
        std::memmove(header_buf+2, &header_.file_size, sizeof(header_.file_size));
        std::memmove(header_buf+10, &header_.header_size, sizeof(header_.header_size));
        std::memmove(header_buf+14, &header_.info_header_size, sizeof(header_.info_header_size)); 
        std::memmove(header_buf+18, &image_->width, sizeof(image_->width));
        std::memmove(header_buf+22, &image_->height, sizeof(image_->height));
        std::memmove(header_buf+26, &header_.planes, sizeof(header_.planes));
        std::memmove(header_buf+28, &header_.color, sizeof(header_.color));
        std::memmove(header_buf+34, &header_.image_size, sizeof(header_.image_size));
        std::memmove(header_buf+38, &header_.x_ppm, sizeof(header_.x_ppm));
        std::memmove(header_buf+42, &header_.y_ppm, sizeof(header_.y_ppm));
        
        std::fwrite(header_buf, sizeof(u_char), HEADER_SIZE, file_ptr); 
        for (auto i=0;i<image_->height;i++) 
        {
            for (auto j=0;j<image_->width;j++) 
            {
                row_data[j*3]   = image_->data[image_->height-i-1][j].blue;
                row_data[j*3+1] = image_->data[image_->height-i-1][j].green;
                row_data[j*3+2] = image_->data[image_->height-i-1][j].red;
            }
            
            for (auto j=image_->width*3; j<real_width; j++)
                row_data[j] = 0;
            
            std::fwrite(row_data, sizeof(u_char), real_width, file_ptr);
        }

        delete(row_data);
        std::fclose(file_ptr);
    }

    Bitmap gray() const
    {
        auto bitmap = this->copy();
        u_char gray_value;
        for(auto i=0; i<image_->height; i++)
        {
            for(auto j=0; j<image_->width; j++)
            {
                gray_value = static_cast<u_char>((static_cast<int>(image_->data[i][j].red) + static_cast<int>(image_->data[i][j].green) + static_cast<int>(image_->data[i][j].blue)) / 3);
                bitmap.image_->data[i][j].red = gray_value;
                bitmap.image_->data[i][j].green = gray_value;
                bitmap.image_->data[i][j].blue = gray_value;
            }
        }
        return bitmap;
    }

    Bitmap& gray() 
    {
        u_char gray_value;
        for(auto i=0; i<image_->height; i++)
        {
            for(auto j=0; j<image_->width; j++)
            {
                gray_value = static_cast<u_char>((static_cast<int>(image_->data[i][j].red) + static_cast<int>(image_->data[i][j].green) + static_cast<int>(image_->data[i][j].blue)) / 3);
                image_->data[i][j].red = gray_value;
                image_->data[i][j].green = gray_value;
                image_->data[i][j].blue = gray_value;
            }
        }
        return *this;
    }

    Bitmap& mosaic(const int& size)
    {
        std::runtime_error("Not implemented.");
        return *this;
    }

    Bitmap copy() const
    {
        Image* image = new Image;
        *image = *image_;
        auto bitmap = Bitmap(header_, image);
        return bitmap;
    }

    void describe() const
    {
        std::cout << "file size             : " << header_.file_size << std::endl;
        std::cout << "info header size      : " << header_.info_header_size << std::endl;
        std::cout << "header size           : " << header_.header_size << std::endl;
        std::cout << "image width           : " << image_->width << "(pixel)" << std::endl;
        std::cout << "image height          : " << image_->height << "(pixel)" << std::endl;
        std::cout << "image color           : " << header_.color << "(bit)" << std::endl;
        std::cout << "compression           : " << header_.compress << std::endl;
        std::cout << "image size            : " << header_.image_size <<"(byte)" << std::endl;
        std::cout << "horizental resolusion : " << header_.x_ppm << std::endl;
        std::cout << "vertical resolusion   : " << header_.y_ppm << std::endl;
    }

};

} // Utility

#endif // _BITMAP_HPP_

#include <vector>
using namespace Utility;

int main()
{
    auto bmp0 = Bitmap::load_bmp("./data/dog.bmp");
    bmp0.to_bmp("./data/dogout1.bmp");

    return 0;
}
