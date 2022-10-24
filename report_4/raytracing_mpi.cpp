#include "minirt/minirt.h"

#include <mpi.h>
#include <cmath>
#include <iostream>
#include <variant>

using namespace minirt;
using namespace std;

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

int main(int argc, char **argv) {

    MPI_Init(&argc, &argv);

    int viewPlaneResolutionX = (argc > 1 ? std::stoi(argv[1]) : 600);
    int viewPlaneResolutionY = (argc > 2 ? std::stoi(argv[2]) : 600);
    int numOfSamples = (argc > 3 ? std::stoi(argv[3]) : 1);
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
    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Vector contains coordinates for each "thread"
    vector<tup_arr> thread_points;
    for (int i = 0; i < size; i++) {
        tup_arr initial_array;
        thread_points.push_back(initial_array);
    }

    // Giving each "thread" their coordinates
    int selected_thread = 0;
    for(int x = 0; x < viewPlaneResolutionX; x++)
        for(int y = 0; y < viewPlaneResolutionY; y++) {
            thread_points[selected_thread].emplace_back(x,y);

            selected_thread++;
            if (selected_thread > size - 1){
                selected_thread = 0;
            }
        }

    double ts = MPI_Wtime();

    const int MAX_SIZE = (viewPlaneResolutionY * viewPlaneResolutionX * 5) / size + 1;

    // [ X, Y, R, G, B,     X, Y, R, G,B,  ...]
    vector<double> myPixels;
    myPixels.reserve(MAX_SIZE);

    // Compute color and store all information about pixel in vector
    for (int j = 0; j < thread_points[rank].size(); j++){
        int x = get<0>(thread_points[rank][j]);
        int y = get<1>(thread_points[rank][j]);
        const auto color = viewPlane.computePixel(scene, x, y, numOfSamples);
        myPixels.push_back(x);
        myPixels.push_back(y);
        myPixels.push_back(color.red);
        myPixels.push_back(color.green);
        myPixels.push_back(color.blue);
    }

    double te = MPI_Wtime();
    double time = te - ts;
    double max_time;

    MPI_Reduce(&time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    MPI_Request send_request;
    MPI_Isend(&myPixels[0], MAX_SIZE, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &send_request);

    if (rank == 0) {
        Image finalImage(viewPlaneResolutionX, viewPlaneResolutionY);
        for (int src_rank = 0; src_rank < size; src_rank++) {
            double* a = new double[MAX_SIZE];
            // Receiving pixel coordinates and their color from each "thread"
            MPI_Recv(a, MAX_SIZE, MPI_DOUBLE, src_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int i =0; i < MAX_SIZE-1; i=i+5)
                // [X,Y,R,G,B,  X,Y,R,G,B,  ....]
                finalImage.set(*(a+i),*(a+i+1), Color(*(a+i+2), *(a+i+3), *(a+i+4)));
            delete [] a;
        }
        finalImage.saveJPEG("report_4/raytracing_" + to_string(size) + ".jpg");

        printf("Time = %.5f\n", max_time);
    }

    MPI_Finalize();
    return 0;
}
