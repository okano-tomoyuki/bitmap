#ifndef _UTILITY_BITMAP_HPP_
#define _UTILITY_BITMAP_HPP_

// https://any-programming.hatenablog.com/entry/2017/04/27/110723

#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <memory>   
#include <bitset>        
#include <stdexcept>        // std::runtime_error
#include <cstring>          // std::memmove, std::strcmp

#include "utility/core.hpp"

namespace Utility
{

class Bitmap final
{

using u_char  = unsigned char;
using VecI    = std::vector<int>;
using VecVecI = std::vector<VecI>;

private:

    static constexpr int HEADER_SIZE      = 40;         /**!< ヘッダのサイズ 54 = 14 + 40    */
    static constexpr int OFFSET_SIZE      = 54;         /**!< データ位置デフォルトオフセット */
    static constexpr uint32_t FILE_TYPE   = 0x4d42;     /**!< ファイルタイプ("BM")           */

    /**! 4bit カラーパレット(昼) */
    static constexpr uint32_t DAY_COLOR_4BIT[16] = {
        0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0xC0C0C0,
        0x808080, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF
    };

    /**! 4bit カラーパレット(夜) */
    static constexpr uint32_t NIGHT_COLOR_4BIT[16] = {
        0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0xC0C0C0,
        0x808080, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF
    };

    #pragma pack(2)
    struct Header final
    {
        uint16_t type;                              /**!< ファイルタイプ ("BM") = (0x42, 0x4d)  */          
        uint32_t size;                              /**!< ファイルサイズ */
        uint16_t reserved1;                         /**!< 予約領域1 */
        uint16_t reserved2;                         /**!< 予約領域2 */
        uint32_t offset_size;                       /**!< 情報ヘッダのサイズ (40) */             
        uint32_t header_size;                       /**!< ヘッダサイズ (54) */
        uint32_t width;                             /**!< 水平Pixel数 */
        uint32_t height;                            /**!< 垂直Pixel数 */            
        uint16_t planes;                            /**!< プレーン数 (1) */
        uint16_t bpp;                               /**!< 1ピクセルあたりのビット数 Bit Per Pixcel (1/4/8/24) */              
        uint32_t compression;                       /**!< 圧縮方法 (0/1/2) */ 
        uint32_t image_size;                        /**!< 画像部分のファイルサイズ (バイト) */
        uint32_t x_ppm;                             /**!< 水平解像度 Pixels Per Meter */
        uint32_t y_ppm;                             /**!< 垂直解像度 Pixels Per Meter */
        uint32_t num_colors;                        /**!< パレットの色数             */
        uint32_t important_colors;                  /**!< 重要パレットのインデックス  */    
    };
    #pragma pack()

    struct Image final
    {

        struct Color final  
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
    enum ColorTheme {DAY, NIGHT};

    /**! デストラクタ */
    ~Bitmap()
    {}

    /**! */
    static std::unique_ptr<Bitmap> create(const VecVecI& input)
    {
        Header header;
        std::memset(&header, 0, OFFSET_SIZE);

        header.type        = FILE_TYPE;
        header.header_size = HEADER_SIZE;
        header.offset_size = OFFSET_SIZE;
        header.bpp         = 24;
        header.x_ppm       = 1;
        header.y_ppm       = 1;
        header.width       = input.at(0).size();
        header.height      = input.size();
        header.compression = 0;
        header.planes      = 1;
        header.image_size  = header.width * header.height *3;
        header.size        = header.offset_size + header.image_size;

        std::vector<Image::Color> data;
        for (const auto& row : input)
            for (const auto& color : row)
            {
                auto r = static_cast<u_char>(( color & 0x0000FF));
                auto g = static_cast<u_char>(((color & 0x00FF00) >> 8));
                auto b = static_cast<u_char>(((color & 0xFF0000) >> 16));
                data.push_back(Image::Color(r, g, b));
            }

        std::unique_ptr<Image> image(new Image(header.height, header.width, std::move(data)));
        std::unique_ptr<Bitmap> bitmap(new Bitmap(header, std::move(image)));
        return bitmap;
    }

    /**! */
    static std::unique_ptr<Bitmap> load(const std::string& file_path)
    {        
        Header header;
        int real_width;
        u_char* row_data;
        u_char header_buf[OFFSET_SIZE];

        std::FILE* file_ptr = std::fopen(file_path.c_str(), "rb");
        if(!file_ptr)
            throw std::runtime_error("File could not open for read.");  
        std::fread(header_buf, sizeof(u_char), OFFSET_SIZE, file_ptr);
        std::memmove(&header, header_buf, OFFSET_SIZE);

        if (header.type != FILE_TYPE)
            throw std::runtime_error("File is not Bitmap file.");
        if (header.bpp!=24)
            throw std::runtime_error("Color format must be 24 byte length.");

        real_width = header.width * 3 + header.width % 4;
        row_data = new u_char[real_width]();

        std::vector<Image::Color> data;
        for(const auto& i : Utility::range(header.height))
        {
            std::fread(row_data, sizeof(u_char), real_width, file_ptr); 
            for(const auto& j : Utility::range(header.width))
            {
                data.push_back(Image::Color(row_data[3*j], row_data[3*j+1], row_data[3*j+2]));
                uint32_t color;
                color = (row_data[3*j+2] << 16) + (row_data[3*j+1] << 8) + row_data[3*j];
            }           
        }

        std::fclose(file_ptr);
        delete[] row_data;

        std::unique_ptr<Image> image(new Image(header.height, header.width, std::move(data)));
        std::unique_ptr<Bitmap> bitmap(new Bitmap(header, std::move(image)));
        return bitmap;
    }

    /**! */
    void save(const std::string& file_path) const
    {        
        int real_width;
        u_char* row_data;
        u_char header_buf[OFFSET_SIZE];
        std::memset(header_buf, 0, OFFSET_SIZE);

        real_width = image_->width*3 + image_->width%4;
        row_data = new u_char[real_width]();
        std::memmove(header_buf, &header_, OFFSET_SIZE);

        std::FILE* file_ptr = std::fopen(file_path.c_str(), "wb");
        if(file_ptr==nullptr)
            throw std::runtime_error("file could not be opened.");  
        std::fwrite(header_buf, sizeof(u_char), OFFSET_SIZE, file_ptr);
        for(const auto& i : Utility::range(image_->height))
        {
            for(const auto& j : Utility::range(image_->width))
            {
                row_data[j*3]   = image_->data[i*image_->width+j].red;
                row_data[j*3+1] = image_->data[i*image_->width+j].green;
                row_data[j*3+2] = image_->data[i*image_->width+j].blue;
            }

            for(const auto& j : Utility::range(image_->width*3, real_width))
                row_data[j] = 0;

            std::fwrite(row_data, sizeof(u_char), real_width, file_ptr);
        }

        delete(row_data);
        std::fclose(file_ptr);
    }

    static void save_4bit(const std::string& file_path, const enum ColorTheme& color_theme=DAY)
    {
        Header header;
        std::memset(&header, 0, OFFSET_SIZE);
        VecVecI input(10, VecI(10, 0));
        for(const auto& i : Utility::range(input.size()))
            for(const auto& j : Utility::range(input.at(0).size()))
                input[i][j] = j;

        header.type        = FILE_TYPE;
        header.header_size = HEADER_SIZE;
        header.offset_size = 0x76;
        header.bpp         = 4;
        header.x_ppm       = 1;
        header.y_ppm       = 1;
        header.width       = input.at(0).size();
        header.height      = input.size();
        header.compression = 0;
        header.planes      = 1;
        header.image_size  = (header.width / 2 + 1) * header.height;
        header.size        = header.offset_size + header.image_size;

        int real_width;
        u_char* row_data;
        u_char header_buf[OFFSET_SIZE];

        real_width = (header.width / 2 + 1);
        while(real_width%4)
            ++real_width;

        row_data = new u_char[real_width]();

        std::memset(header_buf, 0, OFFSET_SIZE);
        std::memmove(header_buf, &header, OFFSET_SIZE);

        std::FILE* file_ptr = std::fopen(file_path.c_str(), "wb");
        if(file_ptr==nullptr)
            throw std::runtime_error("file could not be opened.");
        std::fwrite(header_buf, sizeof(u_char), OFFSET_SIZE, file_ptr);
        
        if(color_theme==DAY)
            std::fwrite(DAY_COLOR_4BIT, sizeof(u_char), sizeof(DAY_COLOR_4BIT), file_ptr);
        else if(color_theme==NIGHT)
            std::fwrite(NIGHT_COLOR_4BIT, sizeof(u_char), sizeof(NIGHT_COLOR_4BIT), file_ptr);
        else
            std::runtime_error("color theme is unknown.");

        for(const auto& i : Utility::range(header.height))
        {
            for(const auto& j : Utility::range(0, header.width, 2))
                row_data[j/2] = input[i][j];

            for(const auto& j : Utility::range(1, header.width, 2))
                row_data[j/2] += (input[i][j] << 4);

            std::fwrite(row_data, sizeof(u_char), real_width, file_ptr);
        }

        delete(row_data);
        std::fclose(file_ptr);
    }

    /**! */
    void describe() const
    {
        std::cout << "file size             : " << header_.size        << "(byte)"      << std::endl;
        std::cout << "header offset_size    : " << header_.offset_size << "(byte)"      << std::endl;
        std::cout << "header size           : " << header_.header_size << "(byte)"      << std::endl;
        std::cout << "image width           : " << header_.width       << "(pixel)"     << std::endl;
        std::cout << "image height          : " << header_.height      << "(pixel)"     << std::endl;
        std::cout << "color bit per pixel   : " << header_.bpp         << "(bit/pixel)" << std::endl;
        std::cout << "compression type      : " << header_.compression                  << std::endl;
        std::cout << "image size            : " << header_.image_size  <<"(byte)"       << std::endl;
        std::cout << "horizental resolusion : " << header_.x_ppm       << "(ppm)"       << std::endl;
        std::cout << "vertical resolusion   : " << header_.y_ppm       << "(ppm)"       << std::endl;
        std::cout << "number of colors      : " << header_.num_colors                   << std::endl;
        std::cout << "important color index : " << header_.important_colors             << std::endl;
    }

};

} // Utility

#endif // _UTILITY_BITMAP_HPP_
