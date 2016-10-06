// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <array>
#include "image.h"
#include "imio.h"
struct raw_header 
{
    int32_t width, height, channels, bytesperpixel;
};
int main(int argc, char * argv[]) 
{
    std::string name;
    if(argc == 1)
        return 0;
    name = argv[1];
    auto inf = std::ifstream(name, std::ios::binary);
    if(!inf)
        return 1;
    raw_header hd;
    inf.read((char*)&hd,sizeof(raw_header));
    std::cout << hd.width << " " << hd.height << " " << hd.channels << " " << hd.bytesperpixel << std::endl;
    while(inf) {
        if(hd.bytesperpixel == 2) {
            auto img = img::Img<uint16_t>(hd.width,hd.height);
            uint32_t ts;
            inf.read((char*)&ts,sizeof(uint32_t));
            inf.read((char*)img.ptr,hd.width*hd.height*sizeof(uint16_t));
            std::cout << ts << std::endl;
            img::imshow("data",img);
            img::getKey(false);
        }
        if(hd.bytesperpixel == 1 && hd.channels == 3) {
            auto img = img::Image<uint8_t,3>(hd.width,hd.height);
            uint32_t ts;
            inf.read((char*)&ts,sizeof(uint32_t));
            inf.read((char*)img.ptr,hd.width*hd.height*3);
            std::cout << ts << std::endl;
            img::imshow("data",img);
            img::getKey(false);
        }
    }
}
