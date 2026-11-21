#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <mutex>

// Bakes the lit 3D butt texture (heightfield -> normals -> diffuse/spec/rim/SSS/AO) once,
// so the per-frame drawing is just a few image blits.
namespace buttart
{
constexpr int LW = 600, LH = 640, CX = 300;
constexpr float BK = 1.5f;
constexpr int PADT = 70, PADB = 100;
constexpr int TW = (int) (LW * BK + 0.5f), TH = (int) ((LH + PADT + PADB) * BK + 0.5f);

// logical crop of the baked texture that can ever be visible (zoom + jiggle margin)
constexpr int CROP_X0 = 40, CROP_X1 = 560, CROP_Y0 = 60, CROP_Y1 = 590;

class Baker
{
public:
    // blocking; safe to call from a background thread. skin = rgb 0..255.
    juce::Image bake (int r, int g, int b);

private:
    void bakeGeometry();
    std::mutex geoLock;
    bool geoReady = false;
    std::vector<float> tm, th, tiny;
};

inline const int kSkins[5][3] = { { 245, 208, 176 }, { 224, 163, 122 }, { 198, 134, 66 }, { 141, 85, 36 }, { 92, 58, 33 } };
}
