#include <iostream>
#include <vector>
#include <random>
#include <string.h>

#define cimg_display 0        // No need for X11 stuff
#include "CImg.h"

using namespace cimg_library;
using namespace std;

const float PI = 3.14159265359;

float randf() {
    return (float)rand() / RAND_MAX;
}

template <typename T>
vector<vector<T>> matmul(vector<vector<T>> A, vector<vector<T>> B) {
    int n = A.size(), m = A[0].size(), p = B[0].size();
    vector<vector<T>> C(n, vector<T>(p, 0));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < p; j++) {
            for (int k = 0; k < m; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    return C;
}

template <typename T>
vector<T> matvec(vector<vector<T>> A, vector<T> B) {
    int n = A.size(), m = A[0].size();
    vector<T> C(n, 0);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            C[i] += A[i][j] * B[j];
        }
    }
    return C;
}

template <typename T>
vector<T> vecsub(vector<T> A, vector<T> B) {
    int n = A.size();
    vector<T> C(n, 0);
    for (int i = 0; i < n; i++) {
        C[i] += A[i] - B[i];
    }
    return C;
}

template <typename T>
double vecnorm(vector<T> A) {
    double norm = 0;
    for (auto a: A) {
        norm += a * a;
    }
    return sqrt(norm);
}

int main() {
    srand(42);

    // Load geometry

    vector<vector<float>> vertices;
    vector<vector<int>> faces;

    string mesh_name = "chinese_dragon";
    string mesh_path = "meshes/" + mesh_name + ".ply";

    FILE *fp = fopen(mesh_path.c_str(), "r");

    char line[1024];
    int V, F;
    while (fgets(line, 1024, fp)) {
        if (strstr(line, "element vertex")) {
            sscanf(line, "element vertex %d", &V);
            vertices.resize(V);
            // cout << "|V| = " << V << endl;
        }
        if (strstr(line, "element face")) {
            sscanf(line, "element face %d", &F);
            faces.resize(F);
            // cout << "|F| = " << F << endl;
        }
        if (strstr(line, "end_header")) {
            break;
        }
    }

    for (int i = 0; i < V; i++) {
        float x, y, z;
        fscanf(fp, "%f %f %f\n", &x, &y, &z);
        vertices[i] = {x, y, z};
    }

    for (int i = 0; i < F; i++) {
        int n, a, b, c;
        fscanf(fp, "%d %d %d %d\n", &n, &a, &b, &c);
        faces[i] = {a, b, c};
    }

    fclose(fp);

    int r_c = 8, phi_c = 0, theta_c = 90, f = 200;
    
    float c_0 = r_c * sin(theta_c * PI / 180) * cos(phi_c * PI / 180);
    float c_1 = r_c * sin(theta_c * PI / 180) * sin(phi_c * PI / 180);
    float c_2 = r_c * cos(theta_c * PI / 180);

    vector<float> c_w = {c_0, c_1, c_2};

    vector<vector<float>> R = {
        {0, 1, 0},
        {0, 0, 1},
        {1, 0, 0}
    };

    int res_x = 2048, res_y = 2048;

    float sensor_x = 32, sensor_y = sensor_x * static_cast<float>(res_y) / res_x;
    int m_x = res_x / sensor_x, m_y = res_y / sensor_y;

    cimg_library::CImg<float> img(res_y, res_x);
    
    double max_dist = -1;

    for (auto v: vertices) {
        vector<float> x_w = {v[0], v[1], v[2]};
        vector<float> x_c = matvec(R, vecsub(x_w, c_w));
        max_dist = max(max_dist, vecnorm(x_c));
    }

    for (auto v: vertices) {
        vector<float> x_w = {v[0], v[1], v[2]};
        vector<float> x_c = matvec(R, vecsub(x_w, c_w));
        int j = m_x * (f * static_cast<float>(x_c[0]) / static_cast<float>(x_c[2]) + static_cast<float>(sensor_x) / 2);
        int i = m_y * (f * static_cast<float>(x_c[1]) / static_cast<float>(x_c[2]) + static_cast<float>(sensor_y) / 2);
        // cout << i << " " << j << endl;
        if (i >= 0 && i < res_y && j >= 0 && j < res_x) {
            img(i, j) += 2 * pow(1 - vecnorm(x_c) / max_dist, 0.1);
        }
    }

    char filename[128];
    string render_path = "output/" + mesh_name + ".pfm";
    sprintf(filename, render_path.c_str());
    img.save_pfm(filename);

    string command = "mogrify -format jpg " + render_path;
    popen(command.c_str(), "r");
}