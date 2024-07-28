#include "bitmap.hpp"

int main()
{
    // 縦 32 pixel, 横 32 pixel の2次元vectorデータを生成する。データはあらかじめ16諧調にしておく。
    const int m = 32, n = 32;
    std::vector<std::vector<int>> mat(m, std::vector<int>(n, 1));
    for(auto&& i=0; i<mat.size(); i++)
        for(auto&& j=0; j<mat.at(0).size(); j++)
            mat.at(i).at(j) = ((i*i + j*j)/100) % 16;

    auto bmp = Bitmap();
    bmp.set_data(std::move(mat), Bitmap::DAY);
    bmp.rotate(Bitmap::Rotate::PLUS90);
    bmp.describe();
    bmp.show();
}