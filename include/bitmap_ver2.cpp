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


class Bitmap
{
using u_short = unsigned short;
using u_long = unsigned long;
using u_int = unsigned int;

private:

    static const int HEADER_SIZE = 54;          // ヘッダのサイズ 54 = 14 + 40
    static const int INFO_HEADER_SIZE = 40;     // 情報ヘッダのサイズ
    static const int PALLET_SIZE = 1024;        // パレットのサイズ
    static const u_int FILE_TYPE = 0x4d42;      // ファイルタイプ("BM")

    struct Header
    {
        u_short file_type;                      // ファイルタイプ "BM"                 
        u_long file_size;                       // bmpファイルのサイズ (バイト)        
        u_int info_header_size;                 // 情報ヘッダのサイズ = 40             
        u_int header_size;                      // ヘッダサイズ = 54                                        
        u_short planes;                         // プレーン数 常に 1                    
        u_short color;                          // 色 (ビット)     24                   
        u_int compress;                         // 圧縮方法         0                   
        u_int image_size;                       // 画像部分のファイルサイズ (バイト)
        u_int x_ppm;                            // 水平解像度 (ppm)
        u_int y_ppm;                            // 垂直解像度 (ppm)
    };


    char data : 1;
    char dat : 4;
    char d : 8;


    struct Image
    {
        struct Color  
        {
            u_char red;
            u_char green;
            u_char blue;
            explicit Color(const u_char& r, const u_char& g, const u_char& b)
             : red(r), green(g), blue(b)
            {}
        };

        int32_t height;
        int32_t width;
        std::vector<Color> data;
        explicit Image(const int32_t& h, const int32_t& w, std::vector<Color>&& d)
         : height(h), width(w), data(std::move(d))
        {}
    };

    Header header_;
    std::unique_ptr<Image> image_;

    explicit Bitmap(const Header& header, std::unique_ptr<Image>&& image)
     : header_(header), image_(std::move(image))
    {}

public:

    ~Bitmap()
    {}

    static std::unique_ptr<Bitmap> create_bmp(const std::vector<std::vector<int>>& input)
    {
        Header header;
        int32_t image_width;
        int32_t image_height;
        int real_width;
        u_char* row_data;

        header.file_type = FILE_TYPE;
        header.header_size = HEADER_SIZE;
        header.info_header_size = INFO_HEADER_SIZE;
        header.color = 24;
        header.x_ppm = 1;
        header.y_ppm = 1;
        image_height = input.size();
        image_width = input.at(0).size();

        real_width = image_width*3 + image_width%4;
        row_data = new u_char[real_width]();

        std::vector<Image::Color> data;
        for (const auto& row : input)
            for (const auto& element : row)
                data.push_back(Image::Color(static_cast<u_char>(element), static_cast<u_char>(element), static_cast<u_char>(element)));

        std::unique_ptr<Image> image(new Image(image_height, image_width, std::move(data)));
        std::unique_ptr<Bitmap> bitmap(new Bitmap(header, std::move(image)));
        return bitmap;
    }

    static std::unique_ptr<Bitmap> load_bmp(const std::string& file_path)
    {
        std::FILE* file_ptr = std::fopen(file_path.c_str(), "rb");
        Header header;
        int32_t image_width;
        int32_t image_height;
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
        std::memmove(&image_width, header_buf+18, sizeof(image_width));
        std::memmove(&image_height, header_buf+22, sizeof(image_height));
        std::memmove(&header.planes, header_buf+26, sizeof(header.planes));
        std::memmove(&header.color, header_buf+28, sizeof(header.color));
        std::memmove(&header.image_size, header_buf+28, sizeof(header.image_size));
        std::memmove(&header.x_ppm, header_buf+38, sizeof(header.x_ppm));
        std::memmove(&header.y_ppm, header_buf+42, sizeof(header.y_ppm));

        if (header.file_type != 0x4d42)
            throw std::runtime_error("File is not Bitmap file.");
        if (header.color!=24)
            throw std::runtime_error("Color format must be 24 byte length.");

        real_width = image_width*3 + image_width%4;
        row_data = new u_char[real_width]();

        std::vector<Image::Color> data;
        for(auto i=0; i<image_height; i++)
        {
            std::fread(row_data, sizeof(u_char), real_width, file_ptr);         
            for (auto j=0; j<image_width; j++) 
                data.push_back(Image::Color(row_data[3*j], row_data[3*j+1], row_data[3*j+2]));
        }

        std::fclose(file_ptr);
        delete[] row_data;

        std::unique_ptr<Image> image(new Image(image_height, image_width, std::move(data)));
        std::unique_ptr<Bitmap> bitmap(new Bitmap(header, std::move(image)));
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
                row_data[j*3]   = image_->data[i*image_->width+j].red;
                row_data[j*3+1] = image_->data[i*image_->width+j].green;
                row_data[j*3+2] = image_->data[i*image_->width+j].blue;
            }
            
            for (auto j=image_->width*3; j<real_width; j++)
                row_data[j] = 0;
            
            std::fwrite(row_data, sizeof(u_char), real_width, file_ptr);
        }

        delete(row_data);
        std::fclose(file_ptr);
    }

    void describe() const
    {
        std::cout << "file size             : " << header_.file_size << "(byte)" << std::endl;
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
    auto bmp0 = Bitmap::load_bmp("../data/dog.bmp");
    bmp0->describe();
    bmp0->to_bmp("../data/origindog.bmp");

    std::vector<std::vector<int>> input(1000, std::vector<int>(1000, 100));

    auto bmp1 = Bitmap::create_bmp(input);
    bmp1->describe();
    bmp1->to_bmp("../data/sample.bmp");

    return 0;
}
