#include "minirt/minirt.h"

#include <cmath>
#include <iostream>
#include <vector>
#include <thread>
#include <tuple>
#include <chrono>

using namespace minirt;
using namespace std;
using namespace std::chrono;
using hrc = high_resolution_clock;

typedef vector<tuple<int,int>> tup_arr;

void initScene(Scene &scene) {
    Color red {1, 0.2, 0.2};
    Color blue {0.2, 0.2, 1};
    Color green {0.2, 1, 0.2};
    Color white {0.8, 0.8, 0.8};
    Color yellow {1, 1, 0.2};

    Material metallicRed {red, white, 50};
    Material mirrorBlack {Color {0.0}, Color {0.9}, 1000};
    Material matteWhite {Color {0.7}, Color {0.3}, 1};
    Material metallicYellow {yellow, white, 250};
    Material greenishGreen {green, 0.5, 0.5};

    Material transparentGreen {green, 0.8, 0.2};
    transparentGreen.makeTransparent(1.0, 1.03);
    Material transparentBlue {blue, 0.4, 0.6};
    transparentBlue.makeTransparent(0.9, 0.7);

    scene.addSphere(Sphere {{0, -2, 7}, 1, transparentBlue});
    scene.addSphere(Sphere {{-3, 2, 11}, 2, metallicRed});
    scene.addSphere(Sphere {{0, 2, 8}, 1, mirrorBlack});
    scene.addSphere(Sphere {{1.5, -0.5, 7}, 1, transparentGreen});
    scene.addSphere(Sphere {{-2, -1, 6}, 0.7, metallicYellow});
    scene.addSphere(Sphere {{2.2, 0.5, 9}, 1.2, matteWhite});
    scene.addSphere(Sphere {{4, -1, 10}, 0.7, metallicRed});

    scene.addLight(PointLight {{-15, 0, -15}, white});
    scene.addLight(PointLight {{1, 1, 0}, blue});
    scene.addLight(PointLight {{0, -10, 6}, red});

    scene.setBackground({0.05, 0.05, 0.08});
    scene.setAmbient({0.1, 0.1, 0.1});
    scene.setRecursionLimit(20);

    scene.setCamera(Camera {{0, 0, -20}, {0, 0, 0}});
}

void thread_func(ViewPlane& viewPlane, Scene& scene, Image& image, int numOfSamples, tup_arr points) {
    for (int j = 0; j < points.size(); j++){
        int x = get<0>(points[j]);
        int y = get<1>(points[j]);
        const auto color = viewPlane.computePixel(scene, x, y, numOfSamples);
        image.set(x, y, color);
    }
}

int main(int argc, char **argv) {
    int viewPlaneResolutionX = (argc > 1 ? std::stoi(argv[1]) : 600);
    int viewPlaneResolutionY = (argc > 2 ? std::stoi(argv[2]) : 600);
    int numOfSamples = (argc > 3 ? std::stoi(argv[3]) : 1);
    int nt = (argc > 2 ? stoi(argv[4]) : 1);
    std::string sceneFile = (argc > 5 ? argv[5] : "");

    Scene scene;
    if (sceneFile.empty()) {
        initScene(scene);
    } else {
        scene.loadFromFile(sceneFile);
    }

    const double backgroundSizeX = 4;
    const double backgroundSizeY = 4;
    const double backgroundDistance = 15;

    const double viewPlaneDistance = 5;
    const double viewPlaneSizeX = backgroundSizeX * viewPlaneDistance / backgroundDistance;
    const double viewPlaneSizeY = backgroundSizeY * viewPlaneDistance / backgroundDistance;

    ViewPlane viewPlane {viewPlaneResolutionX, viewPlaneResolutionY,
                         viewPlaneSizeX, viewPlaneSizeY, viewPlaneDistance};

    Image image(viewPlaneResolutionX, viewPlaneResolutionY); // computed image

    vector<tup_arr> thread_points;

    for (int i = 0; i < nt; i++) {
        tup_arr initial_array;
        thread_points.push_back(initial_array);
    }

    int selected_thread = 0;

    for(int x = 0; x < viewPlaneResolutionX; x++)
        for(int y = 0; y < viewPlaneResolutionY; y++) {
            thread_points[selected_thread].emplace_back(x,y);

            selected_thread++;
            if (selected_thread > nt - 1){
                selected_thread = 0;
            }
        }

    vector<thread> threads;

    auto ts = hrc::now();
    for (int i = 0; i < nt; i++) {
        thread thread(&thread_func, ref(viewPlane), ref(scene), ref(image), numOfSamples, thread_points[i]);
        threads.push_back(move(thread));
    }

    for (int i = 0; i < nt; i++) {
        threads[i].join();
    }
    auto te = hrc::now();
    double time = duration<double>(te - ts).count();
    cout << "Time = " << time << endl;

    image.saveJPEG("report_2/raytracing.jpg");

    return 0;
}
