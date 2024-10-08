#include "bitmap.hpp"

int main()
{
    using Image::Bitmap;

    // 縦 32 pixel, 横 32 pixel の2次元vectorデータを生成する。データはあらかじめ16諧調にしておく。
    const int m = 32, n = 32;
    std::vector<std::vector<uint32_t>> mat(m, std::vector<uint32_t>(n, 1));
    for(auto&& i=0; i<mat.size(); i++)
        for(auto&& j=0; j<mat.at(0).size(); j++)
            mat.at(i).at(j) = ((i*i + j*j)/100) % 16;

    auto bmp = Bitmap();
    bmp.set_data(std::move(mat), Bitmap::DAY);
    bmp.rotate(Bitmap::Rotate::PLUS90);
    bmp.describe();

    auto dog = Bitmap();
    dog.read("data/dog.bmp");
    dog.describe();
}
