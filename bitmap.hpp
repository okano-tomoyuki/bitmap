#ifndef _IMAGE_BITMAP_HPP_
#define _IMAGE_BITMAP_HPP_

#include <iostream>         // std::cout, std::endl
#include <iomanip>          // std::setw, std::setfill
#include <algorithm>        // std::reverse
#include <vector>           // std::vector
#include <cstdlib>          // std::FILE, std::fopen, std::fwrite, std::fread, std::fclose
#include <stdexcept>        // std::runtime_error
#include <cstring>          // std::memmove, std::memset
#include <cstdint>          // u_char, uint_16_t, uint32_t
#include <string>           // std::string, std::to_string

#ifdef __unix__
#include <unistd.h>
#else
#include <windows.h>
#endif

namespace Image
{

class Bitmap final
{

using u_char  = unsigned char;

public:

    /** 
     * @enum  ColorTheme
     * @brief 4bit用のデフォルトカラーテーマ
     */
    enum ColorTheme { DAY, NIGHT, GRAYSCALE };

    /** 
     * @enum  Format
     * @brief Bitmapフォーマット 
     */
    enum Format { BIT1  = 1, BIT4  = 4, BIT8  = 8, BIT24 = 24 };

    /** 
     * @enum  Rotate
     * @brief 回転用メソッドの引数専用の型 
     */
    enum Rotate { MINUS90, PLUS90 };

    /** 
     * @enum  Mirror
     * @brief 反転用メソッドの引数専用の型
     */
    enum Mirror { HORIZONTAL, VERTICAL };

    /** コンストラクタ */
    explicit Bitmap()
    {}

    /** デストラクタ */
    ~Bitmap()
    {}

    /** コピー用オペレータ= */
    Bitmap& operator=(const Bitmap& bitmap) &
    {
        color_pallet_   = bitmap.color_pallet_;
        data_           = bitmap.data_;
        header_         = bitmap.header_;
        return *this;
    }

    /** 
     * @fn read
     * @brief Bitmapデータ読取処理
     * @note  現状不要な機能であるため実装しない。 
     */
    Bitmap& read(const std::string& file_path) &
    {
        std::FILE* file_ptr = std::fopen(file_path.c_str(), "rb");
        if(file_ptr==nullptr) 
            throw std::runtime_error("file could not be opened.");

        if (std::fread(&header_, sizeof(u_char), OFFSET_SIZE, file_ptr) != OFFSET_SIZE)
            throw std::runtime_error("read header.");

        if (header_.type != FILE_TYPE)
            throw std::runtime_error("file type.");

        if (header_.bpp != BIT24)
        {
            int pallet_size = (1 << (header_.bpp + 2));
            if (header_.offset_size != OFFSET_SIZE + pallet_size)
                throw std::runtime_error("pallet size.");
            color_pallet_.resize(pallet_size / 4);
            if (std::fread(color_pallet_.data(), sizeof(uint32_t), color_pallet_.size(), file_ptr) != color_pallet_.size())
                throw std::runtime_error("read pallet.");
        }

        data_.resize(header_.height, std::vector<uint32_t>(header_.width, 0));
        int width_in_bytes  = (header_.width * header_.bpp + 31) / 32 * 4;
        
        u_char row_data[width_in_bytes];

        for(int i = 0; i < header_.height; i++)
        {
            std::fread(row_data, sizeof(u_char), width_in_bytes, file_ptr);
            if (header_.bpp != BIT24)
            {
                const int size_in_byte = 8 / header_.bpp;
                for(int j = 0; j < header_.width; j++)
                {
                    u_char remain = header_.bpp * (size_in_byte - (j % size_in_byte + 1));
                    data_[i][j] = ((row_data[j / size_in_byte] >> remain) & ((1 << header_.bpp) - 1));
                }
            }
            else 
            {
                for(int j = 0; j < header_.width; j++)
                {
                    data_[i][j] |= (row_data[j] << (j % 3 * 8));
                }     
            }
        }

        std::fclose(file_ptr);

        return *this;
    }

    /**
     * @fn set_data
     * @brief ピクセル値データセット(カラーパレットはユーザ定義)
     * @param std::vector<std::vector<int> data x方向, y方向のピクセル値を16諧調の2次元ベクターとしてセットする。
     * @param std::vector<uint32_t> color_palet 0~15 のピクセルとして使用するRGB値を1次元ベクターとしてセットする。
     * @param enum Format format Bitmapのデータフォーマット。現状16諧調のみをサポートし、必要に応じて拡張する。
     * @return Bitmap& データセット後の自身のインスタンスを返却する。
     */
    Bitmap& set_data(std::vector<std::vector<uint32_t>>&& data, const std::vector<uint32_t>& color_pallet, const enum Format& format=BIT4) &
    {
        if(format != BIT4)
            throw std::runtime_error("Only 4bit format is implemented.");
        
        int pallet_size = 0;
        if(format != BIT24)
        {
            pallet_size += (1 << (static_cast<int>(format) + 2));
            color_pallet_.resize(pallet_size / 4);
            if(color_pallet.size() != color_pallet_.size())
                throw std::runtime_error("color pallet size isn't correct.");
            color_pallet_ = color_pallet;
        }

        header_.bpp         = static_cast<int>(format);
        header_.offset_size = OFFSET_SIZE + pallet_size;
        header_.width       = data.at(0).size();
        header_.height      = data.size();
        int width_in_bytes  = (header_.width * header_.bpp + 31) / 32 * 4;
        header_.image_size  = width_in_bytes * header_.height;
        header_.size        = header_.offset_size + header_.image_size;
        data_               = std::move(data);

        return *this;
    }

    /** 
     * @fn set_data
     * @brief ピクセル値データセット(カラーパレットはユーザ定義)
     * @param std::vector<std::vector<int> data x方向, y方向のピクセル値を16諧調の2次元ベクターとしてセットする。
     * @param enum ColorTheme color_theme 3種のカラーパレットから選択する。 { DAY, NIGHT, GRAYSCALE }
     * @param enum Format format Bitmapのデータフォーマット。現状16諧調のみをサポートし、必要に応じて拡張する。
     * @return Bitmap& データセット後の自身のインスタンスを返却する。
     */
    Bitmap& set_data(std::vector<std::vector<uint32_t>>&& data, const enum ColorTheme& color_theme=DAY, const enum Format& format=BIT4) &
    {
        if(format != BIT4)
            throw std::runtime_error("Only 4bit format is implemented.");       

        if(color_theme == DAY)
            return set_data(std::move(data), DAY_COLOR_4BIT, format);
        else if(color_theme == NIGHT)
            return set_data(std::move(data), NIGHT_COLOR_4BIT, format);
        else if(color_theme == GRAYSCALE)
            return set_data(std::move(data), GRAYSCALE_COLOR_4BIT, format);
        else
            throw std::runtime_error("color format is unknown.");

    }

    /** 
     * @fn mirror
     * @brief 画像データを鏡像に変換するメソッド
     * @param enum Mirror mirror  { HORIZONTAL=上下反転, VERTICAL=左右反転 }
     * @return Bitmap& 鏡像変換後の自身のインスタンスを返却する。
     */
    Bitmap& mirror(const enum Mirror& mirror=HORIZONTAL) &
    {
        if(mirror==HORIZONTAL)
            std::reverse(data_.begin(), data_.end());
        else if(mirror==VERTICAL)
            for(auto&& row : data_)
                std::reverse(row.begin(), row.end());
        else
            throw std::runtime_error("mirror method is unknown.");

        return *this;
    }

    /** 
     * @fn rotate
     * @brief 画像データを鏡像に変換するメソッド
     * @param enum Rotate rotate  { PLUS90=時計回り90度回転, MINUS90=反時計回り90度回転 }
     * @return Bitmap& 回転後の自身のインスタンスを返却する。
     * @note 180°回転させたい場合は bitmap.rotate(PLUS90).rotate(PLUS90); のように使用する。
     */
    Bitmap& rotate(const enum Rotate& rotate=PLUS90) &
    {
        mirror();
        int w = header_.width;
        int h = header_.height;
        header_.height = w;
        header_.width  = h;
        int width_in_bytes = (header_.width * header_.bpp + 31) / 32 * 4;
        header_.image_size = width_in_bytes * header_.height;
        header_.size = header_.offset_size + header_.image_size;

        std::vector<std::vector<uint32_t>> tmp(header_.height, std::vector<uint32_t>(header_.width, 0));
        if(rotate == PLUS90) 
        {
            for(int i=0; i<header_.height; i++)
            {
                for(int j=0; j<header_.width; j++)
                {
                    tmp[i][j] = data_[header_.width-1-j][header_.height-1-i];
                }
            }
        } 
        else if(rotate == MINUS90) 
        {
            for(int i=0; i<header_.height; i++)
            {
                for(int j=0; j<header_.width; j++)
                {
                    tmp[i][j] = data_[j][i];
                }
            }
        } 
        else 
        {
            throw std::runtime_error("rotate method is unknown.");
        }
        
        data_ = std::move(tmp);
        return *this;
    }

    /** 
     * @fn save
     * @brief 画像データを保存するメソッド
     * @param std::string file_path 保存先ファイルパス
     */
    void save(const std::string& file_path) const
    {
        std::FILE* file_ptr = std::fopen(file_path.c_str(), "wb");
        if(file_ptr==nullptr) 
            throw std::runtime_error("file could not be opened.");

        u_char header_buf[OFFSET_SIZE];
        std::memmove(header_buf, &header_, OFFSET_SIZE);
        std::fwrite(header_buf, sizeof(u_char), OFFSET_SIZE, file_ptr);

        if(header_.bpp != static_cast<int>(BIT24))
            std::fwrite(color_pallet_.data(), sizeof(uint32_t), color_pallet_.size(), file_ptr);

        int width_in_bytes = (header_.width * header_.bpp + 31) / 32 * 4;
        u_char row_data[width_in_bytes];

        for(int i = 0; i < header_.height; i++)
        {
            std::memset(row_data, 0, width_in_bytes);
            if (header_.bpp != BIT24)
            {
                const int size_in_byte = 8 / header_.bpp;
                for(int j = 0; j < header_.width; j++)
                {
                    u_char remain = header_.bpp * (size_in_byte - (j % size_in_byte + 1));
                    row_data[j / size_in_byte] |= ((data_[i][j] & ((1 << header_.bpp) - 1)) << remain);
                }
            }
            std::fwrite(row_data, sizeof(u_char), width_in_bytes, file_ptr);
        }

        std::fclose(file_ptr);        
    }

    /** 
     * @fn describe
     * @brief 画像データのメタ情報をコンソール出力するメソッド
     * @param bool show_data trueとした場合、実際のピクセル値も出力する。
     */
    void describe(const bool& show_data=false) const
    {
        std::cout   << "header size  : " << header_.header_size    << "\t[Byte]\n"
                    << "offset size  : " << header_.offset_size    << "\t[Byte]\n"
                    << "size         : " << header_.size           << "\t[Byte]\n"
                    << "width        : " << header_.width          << "\t[Pixel]\n"
                    << "height       : " << header_.height         << "\t[Pixel]\n"
                    << "planes       : " << header_.planes         << "\n"
                    << "compression  : " << header_.compression    << "\n"
                    << "image size   : " << header_.image_size     << "\t[Byte]\n"
                    << "x            : " << header_.x_ppm          << "\t[PPM (Pixel Per Meter)]\n"
                    << "y            : " << header_.y_ppm          << "\t[PPM (Pixel Per Meter)]\n"
                    << "format       : " << header_.bpp            << "\t[Bit]\n";
        
        if(header_.bpp != BIT24)
        {
            std::cout << "color pallet : ";
            if (is_console())
            {
                for (const auto& color : color_pallet_)
                {
                    std::cout << bg_color(((color & 0xFF0000) >> 16), ((color & 0x00FF00) >> 8), ((color & 0x0000FF) >> 0)) << ' ';
                }
                std::cout << reset() << std::endl;
            }
            else
            {
                for(const auto& color : color_pallet_)
                    std::cout << std::setw(6) << std::setfill('0') << std::hex << color << " ";
                std::cout << std::dec << std::endl;        
            }
        }
    }

    void show() const
    {
        if (is_console())
        {
            if(header_.bpp == BIT4)
            {
                for (const auto& line : data_)
                {
                    for (const auto& cell : line)
                    {
                        int blue  = (color_pallet_[cell] & 0x0000FF ) >>  0;
                        int green = (color_pallet_[cell] & 0x00FF00 ) >>  8;
                        int red   = (color_pallet_[cell] & 0xFF0000 ) >> 16;
                        if (is_console())
                            std::cout << bg_color(red, green, blue) << ' ';
                        else
                            std::cout << '(' << red << ',' << green << ',' << blue << ')' << ' ';
                    }
                    if (is_console())
                        std::cout << reset() << std::endl;
                    else
                        std::cout << std::endl;
                }
            }
        }
    }

private:

    static constexpr int HEADER_SIZE    = 40;       /**!< header size(40)                */
    static constexpr int OFFSET_SIZE    = 54;       /**!< default data start offset(54)  */
    static constexpr uint32_t FILE_TYPE = 0x4d42;   /**!< file type ("BM")               */

    #pragma pack(2)
    /**! Bitmap header */
    struct Header final
    {
        uint16_t type;                              /**!< file type ("BM") = (0x42, 0x4d)  */          
        uint32_t size;                              /**!< file size */
        uint16_t reserved1;                         /**!< reserved1(0) */
        uint16_t reserved2;                         /**!< reserved2(0) */
        uint32_t offset_size;                       /**!< data offset (54+α) */             
        uint32_t header_size;                       /**!< header size (40) */
        uint32_t width;                             /**!< horizontal pixel size */
        uint32_t height;                            /**!< vertical pixel size */            
        uint16_t planes;                            /**!< plane (1) */
        uint16_t bpp;                               /**!< Bit Per Pixcel (1/4/8/24) */              
        uint32_t compression;                       /**!< compression method (0/1/2) */ 
        uint32_t image_size;                        /**!< image size */
        uint32_t x_ppm;                             /**!< horizontal Pixels Per Meter */
        uint32_t y_ppm;                             /**!< vertical Pixels Per Meter */
        uint32_t num_colors;                        /**!< pallet collor size */
        uint32_t important_colors;                  /**!< important color index */
        explicit Header()
         :  type(FILE_TYPE),
            reserved1(0),
            reserved2(0),
            header_size(HEADER_SIZE),
            planes(1),
            compression(0),
            x_ppm(1),
            y_ppm(1),
            num_colors(0),
            important_colors(0)
        {}
    };
    #pragma pack()

    std::vector<uint32_t> color_pallet_;            /**!< color pallet   */
    std::vector<std::vector<uint32_t>> data_;       /**!< value of pixel */
    Header  header_;                                /**!< Bitmap header  */

    /**! 4bit カラーパレット(昼) */
    const std::vector<uint32_t> DAY_COLOR_4BIT = {
        0xEBFFFF, 0xCCFFFF, 0x66FFFF, 0x00CCFF, 0x0099FF, 0x3366FF, 0x33FF00, 0x33CC00,
        0x199900, 0xFFFF00, 0xFFCC00, 0xFF9900, 0xFF5066, 0xFF0000, 0xB70014, 0x8E0011
    };

    /**! 4bit カラーパレット(夜) */
    const std::vector<uint32_t> NIGHT_COLOR_4BIT = {
        0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0xC0C0C0,
        0x808080, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF
    };

    /**! 4bit カラーパレット(グレースケール) */
    const std::vector<uint32_t> GRAYSCALE_COLOR_4BIT = {
        0x000000, 0x111111, 0x222222, 0x333333, 0x444444, 0x555555, 0x666666, 0x777777,
        0x888888, 0x999999, 0xAAAAAA, 0xBBBBBB, 0xCCCCCC, 0xDDDDDD, 0xEEEEEE, 0xFFFFFF
    };

    static std::string bg_color(const int& red, const int& green, const int& blue)
    {
        return std::string("\x1b[48;2;" + std::to_string(red) + ";" + std::to_string(green) + ";" + std::to_string(blue) + "m").c_str();
    }

    static std::string reset()
    {
        return std::string("\x1b[39m\x1b[49m");
    }

    static bool is_console()
    {
#ifdef __unix__
        return (isatty(STDOUT_FILENO) == 1);
#else
        DWORD mode;
        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        return (GetFileType(handle) == FILE_TYPE_CHAR && GetConsoleMode(handle, &mode) != 0);
#endif
    }

};

}

#endif // _IMAGE_BITMAP_HPP_
