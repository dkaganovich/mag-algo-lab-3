#include "cache.h"

void MultSimple(Cache& cache, const float* __restrict a, const float* __restrict b, float* __restrict c, int n)
{
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			cache.fetch(&c[i * n + j]);
			c[i * n + j] = 0.f;
			for (int k = 0; k < n; ++k) {
				cache.fetch(&a[i * n + k]);
				cache.fetch(&b[k * n + j]);
				cache.fetch(&c[i * n + j]);
				c[i * n + j] += a[i * n + k] * b[k * n + j];
			}
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
	if (argc < 6) {
		std::cerr << "Illegal number of arguments" << std::endl;
		exit(1);
	}
	int n = atoi(argv[1]);
	int totalSize/*KB*/ = atoi(argv[2]);
	int lineSize/*B*/ = atoi(argv[3]);
	int numWays = atoi(argv[4]);
	int evictionPolicy = atoi(argv[5]);
	char *logFileName = nullptr;
	if (argc >= 7)
		logFileName = argv[6];
	
	float* a = new float[n * n];
	float* b = new float[n * n];
	float* c = new float[n * n];

	FillRandom(a, n);
	FillRandom(b, n);
	Cache cache(totalSize, lineSize, numWays, (CacheEvictionPolicy)evictionPolicy, logFileName);
	{
		MultSimple(cache, a, b, c, n);
		std::cout << n << std::endl 
			<< std::setprecision(4) << cache.calcMissRate() << std::endl
			<< cache.getLifeSpanStats().size() << std::endl;
		for (std::unordered_map<uint64_t, uint64_t>::const_iterator it = cache.getLifeSpanStats().begin(); it != cache.getLifeSpanStats().end(); ++it) {
			std::cout << it->first << "," << it->second << std::endl;
		}
	}

	delete[] a;
	delete[] b;
	delete[] c;
}
