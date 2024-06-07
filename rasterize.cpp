#include <iostream>
#include <vector>
#include <random>
#include <string.h>

#define cimg_display 0        // No need for X11 stuff
#include "CImg.h"

using namespace cimg_library;
using namespace std;

const float deg = 3.14159265359 / 180;

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

template <typename T>
T sq(T x) {
    return x * x;
}

int main() {
    srand(42);

    // Load geometry

    char *mesh_name = new char[128];
    float r, phi, theta, f;
    int res_x, res_y;
    float sensor_x, sensor_y;

    vector<vector<float>> vertices;
    vector<vector<int>> faces;

    {
        FILE *fp = fopen("scene2", "r");
        char line[128];
        fgets(line, 128, fp);
        fgets(line, 128, fp);
        sscanf(line, "%s %f %f %f %f %d %d %f", mesh_name, &r, &phi, &theta, &f, &res_x, &res_y, &sensor_x);
        fclose(fp);
    }
    sensor_y = sensor_x * static_cast<float>(res_y) / res_x;

    string mesh_path = "meshes/" + string(mesh_name) + ".ply";
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
    
    float c_0 = r * sin(theta * deg) * cos(phi * deg);
    float c_1 = r * sin(theta * deg) * sin(phi * deg);
    float c_2 = r * cos(theta * deg);

    vector<float> c_w = {c_0, c_1, c_2};

    vector<vector<float>> R = {
        {-1 * sin(phi * deg), cos(phi * deg), 0},
        {-1 * cos(theta * deg) * cos(phi * deg), -1 * cos(theta * deg) * sin(phi * deg), sin(theta * deg)},
        {sin(theta * deg) * cos(phi * deg), sin(theta * deg) * sin(phi * deg), cos(theta * deg)}
    };

    int m_x = res_x / sensor_x, m_y = res_y / sensor_y;

    cimg_library::CImg<float> img(res_y, res_x);

    for (auto v: vertices) {
        vector<float> x_w = {v[0], v[1], v[2]};
        vector<float> x_c = matvec(R, vecsub(x_w, c_w));
        int j = m_x * (f * static_cast<float>(x_c[0]) / static_cast<float>(x_c[2]) + static_cast<float>(sensor_x) / 2);
        int i = m_y * (f * static_cast<float>(x_c[1]) / static_cast<float>(x_c[2]) + static_cast<float>(sensor_y) / 2);
        // cout << i << " " << j << endl;
        if (i >= 0 && i < res_y && j >= 0 && j < res_x) {
            img(i, j) += 0.1;
        }
    }

    char filename[128];
    string render_path = "output/" + string(mesh_name) + ".pfm";
    sprintf(filename, render_path.c_str());
    img.save_pfm(filename);

    string command = "mogrify -format jpg " + render_path;
    popen(command.c_str(), "r");
}