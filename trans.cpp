#include <cstdlib>
#include <random>

void MxTranspose(float* a, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            float tmp = a[i * n + j];
            a[i * n + j] = a[j * n + i];
            a[j * n + i] = tmp;
        }
    }
}

void FillRandom(float* a, int n)
{
    std::default_random_engine eng;
    std::uniform_real_distribution<float> dist;

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            a[i * n + j] = dist(eng);
        }
    }
}

int main(int argc, char* argv[])
{

    int n = atoi(argv[1]);
    
    float* a = new float[n * n];
    FillRandom(a, n);

    MxTranspose(a, n);

    delete[] a;
}
