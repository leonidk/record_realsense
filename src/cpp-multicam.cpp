// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include "example.hpp"

#include <iostream>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <array>
std::vector<texture_buffer> buffers;

template <class Archive>
void serialize( Archive & ar,rs::intrinsics & intrin)
{
    ar( intrin.width, intrin.height, intrin.ppx, intrin.ppy,intrin.fx,intrin.fy,intrin.model(),intrin.coeffs );
}
template <class Archive>
void serialize( Archive & ar,rs::extrinsics & extrin)
{
    ar( extrin.rotation, extrin.translation );
}
int main(int argc, char * argv[]) try
{
    std::string prefix_name("recording");
    std::time_t timestamp = std::time(0);
    int max_cameras = 3;
    int rgb_w = 640;
    int rgb_h = 480;
    int z_w = 480;
    int z_h = 360;
    int using_rect = 1;

    if(argc > 1 && std::string(argv[1]) == "help") {
        std::cout << "Usage: " << argv[0] << " <recording_name> <max # of cameras> <0/1 to enable rectification> <rgb_w> <rgb_h> <depth_w> <depth_h>\n";
        std::cout << "Default: " << argv[0] << " " << prefix_name << " " << max_cameras << " " << using_rect << " " << rgb_w << " " << rgb_h << " " << z_w << " " << z_h << std::endl;
        std::cout << "Note: if using rectification, recorded depth will be RGB sized\n";
        std::cout << "Note: Recording is always 30Hz and using autoexposure\n";
        return 0;
    }
    rs::log_to_console(rs::log_severity::warn);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    rs::context ctx;
    if(ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    
    // Enumerate all devices
    std::vector<rs::device *> devices;
    std::vector<std::array<std::ofstream,2>> recordings;
    for(int i=0; i<ctx.get_device_count(); ++i)
    {
        devices.push_back(ctx.get_device(i));
    }
    auto stream_type_depth = rs::stream::depth_aligned_to_rectified_color;
    auto stream_type_color = rs::stream::rectified_color;
    // Configure and start our devices
    for(int i=0; i < devices.size(); i++)
    {
        auto dev = devices[i];

        std::cout << "Starting " << dev->get_name() << "... ";
        dev->enable_stream(rs::stream::depth, rs::preset::best_quality);
        dev->enable_stream(rs::stream::color, rs::preset::best_quality);
        dev->start();
        std::cout << "done." << std::endl;

        {
            std::ofstream os(prefix_name + "_" + std::to_string(timestamp) + "_intrin_depth_" +std::to_string(i) + ".json", std::ios::binary);
            cereal::JSONOutputArchive archive( os );
            archive(dev->get_stream_intrinsics(stream_type_depth));
        }
        {
            std::ofstream os(prefix_name + "_" + std::to_string(timestamp) + "_intrin_color_" +std::to_string(i) + ".json", std::ios::binary);
            cereal::JSONOutputArchive archive( os );
            archive(dev->get_stream_intrinsics(stream_type_color));
        }
        {
            std::ofstream os(prefix_name + "_" + std::to_string(timestamp) + "_extrin_" +std::to_string(i) + ".json", std::ios::binary);
            cereal::JSONOutputArchive archive( os );
            archive(dev->get_extrinsics(stream_type_depth,stream_type_color));
        }
    }

    // Depth and color
    buffers.resize(ctx.get_device_count() * 2);

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Multi-Camera Example";
    GLFWwindow * win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);

    int windowWidth, windowHeight;
    glfwGetWindowSize(win, &windowWidth, &windowHeight);

    // Does not account for correct aspect ratios
    auto perTextureWidth = int(windowWidth / devices.size());
    auto perTextureHeight = 480;

    while (!glfwWindowShouldClose(win))
    {
        // Wait for new images
        glfwPollEvents();
        
        // Draw the images
        int w,h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwGetWindowSize(win, &w, &h);
        glPushMatrix();
        glOrtho(0, w, h, 0, -1, +1);
        glPixelZoom(1, -1);
        int i=0, x=0;
        for(auto dev : devices)
        {
            dev->poll_for_frames();
            const auto c = dev->get_stream_intrinsics(rs::stream::color), d = dev->get_stream_intrinsics(rs::stream::depth);
            buffers[i++].show(*dev, rs::stream::color, x, 0, perTextureWidth, perTextureHeight);
            buffers[i++].show(*dev, rs::stream::depth, x, perTextureHeight, perTextureWidth, perTextureHeight);
            x += perTextureWidth;
        }

        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
