#include "cache.h"

void MxTranspose(Cache& cache, float* a, int n) {
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			cache.fetch(&a[i * n + j]);
			cache.fetch(&a[j * n + i]);
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
	FillRandom(a, n);

	Cache cache(totalSize, lineSize, numWays, (CacheEvictionPolicy)evictionPolicy, logFileName);
	MxTranspose(cache, a, n);

	std::cout << n << std::endl 
		<< std::setprecision(4) << cache.calcMissRate() << std::endl
		<< cache.getLifeSpanStats().size() << std::endl;
	for (std::unordered_map<uint64_t, uint64_t>::const_iterator it = cache.getLifeSpanStats().begin(); it != cache.getLifeSpanStats().end(); ++it) {
		std::cout << it->first << "," << it->second << std::endl;
	}

	delete[] a;
}
