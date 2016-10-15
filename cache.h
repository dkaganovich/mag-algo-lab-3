#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <random>
#include <chrono>

namespace 
{
	struct CacheLine {
		uint64_t accessTs;
		uintptr_t tag;
	};

	struct CacheSet {
		uint64_t accessCnt;
		std::vector<CacheLine> cacheLines;
		CacheSet(int numWays) : 
			accessCnt(0), cacheLines(numWays, { 0 }) {}
	};

	enum CacheEvictionPolicy
	{
		LRU,
		RR
	};
	// a companion structure to hold eviction policies' names
	const char* const CacheEvictionPolicyName[] = {"LRU", "RR"};

	class Cache
	{
	public:
		Cache(int totalSize/*KB*/, int lineSize/*B*/, int numWays, CacheEvictionPolicy evictionPolicy, const char* logFileName): 
			accessCnt(0), missCnt(0), 
			totalSize(totalSize), lineSize(lineSize), numWays(numWays), evictionPolicy(evictionPolicy), 
			gen(std::chrono::system_clock::now().time_since_epoch().count()), distr(0, numWays - 1)
		{
			offsetMask = (uintptr_t)lineSize - 1;// lineSize is somewhat power of 2
			offsetMaskLen = (short)log2(lineSize);

			int numCacheSet = ((uint64_t)totalSize << 10) / (lineSize * numWays);// the fraction is also a power of 2
			indexMask = (uintptr_t)(numCacheSet - 1) << offsetMaskLen;
			indexMaskLen = (short)log2(numCacheSet);

			for (int i = 0; i < numCacheSet; ++i) 
				cacheSets.push_back(CacheSet(numWays));

			tagMask = (UINTPTR_MAX & ~offsetMask) & ~indexMask;
			tagMaskLen = (short)log2(UINTPTR_MAX) - offsetMaskLen - indexMaskLen;

			if (logFileName != nullptr) {
				logEnabled = true;
				LOG.open(logFileName, std::ofstream::out);
			}
			else {
				logEnabled = false;
			}
		}

		~Cache() 
		{
			if (logEnabled)
				LOG.close();
		}

		bool fetch(const void* ptr) 
		{
			uintptr_t uiptr = reinterpret_cast<uintptr_t>(ptr);
			accessCnt++;

			uintptr_t cacheLineOffset = calcCacheLineOffset(uiptr);// for debugging mainly
			uintptr_t cacheSetIndex = calcCacheSetIndex(uiptr);
			uintptr_t cacheLineTag = calcCacheLineTag(uiptr);

			if (logEnabled)
				LOG << "Accessing " << cacheLineTag << "|" << cacheSetIndex << "|" << cacheLineOffset << std::endl;

			CacheSet& cacheSet = cacheSets[cacheSetIndex];
			cacheSet.accessCnt++;

			bool isExist = false;
			for (int i = 0; i < numWays; ++i) {
				if (cacheSet.cacheLines[i].tag == cacheLineTag) {
					isExist = true;
					break;
				}
			}

			if (!isExist) {
				missCnt++;
				CacheLine cacheLine = { cacheSet.accessCnt, cacheLineTag };
				store(cacheSet, cacheLine);
			}
			return isExist;
		}

		void printConfig(std::ostream& os) 
		{
			os << "**************Cache Config**************" << std::endl
				<< std::setw(20) << std::left << "Metrics:" << std::endl
				<< std::setw(20) << std::left << "Total Size(KB): " << std::setw(20) << std::right << totalSize << std::endl
				<< std::setw(20) << std::left << "Line Size(B): " << std::setw(20) << std::right << lineSize << std::endl
				<< std::setw(20) << std::left << "Num. Ways: " << std::setw(20) << std::right << numWays << std::endl
				<< std::setw(20) << std::left << "Eviction Policy:" << std::setw(20) << std::right << CacheEvictionPolicyName[evictionPolicy] << std::endl
				<< std::setw(20) << std::left << "Calculated Masks:" << std::endl
				<< std::setw(20) << std::left << "Offset Mask: " << std::setw(20) << std::right << std::hex << offsetMask << std::endl
				<< std::setw(20) << std::left << "Index Mask: " << std::setw(20) << std::right << std::hex << indexMask << std::endl
				<< std::setw(20) << std::left << "Tag Mask: " << std::setw(20) << std::right << std::hex << tagMask << std::endl;
		}

		void printStats(std::ostream& os) 
		{
			os << "**************Cache Stats***************" << std::endl
				<< std::setw(20) << std::left << "Miss Rate(%):" 
				<< std::setw(20) << std::right << std::setprecision(4) << 100 * calcMissRate() << std::endl;
		}

		double calcMissRate() {
			return (double)missCnt / accessCnt;
		}

		uint64_t getAccessCnt() {
			return accessCnt;
		}

		uint64_t getMissCnt() {
			return missCnt;
		}

		const std::unordered_map<uint64_t, uint64_t>& getLifeSpanStats() {
			return lifeSpanStats;
		}
	private:
		inline uintptr_t calcCacheLineOffset(uintptr_t uiptr) { return (uiptr & offsetMask); }

		inline uintptr_t calcCacheSetIndex(uintptr_t uiptr) { return (uiptr & indexMask) >> offsetMaskLen; }

		inline uintptr_t calcCacheLineTag(uintptr_t uiptr) { return (uiptr & tagMask) >> (offsetMaskLen + indexMaskLen); }

		void store(CacheSet& cacheSet, CacheLine cacheLine) 
		{
			for (int i = 0; i < numWays; ++i) {
				if (cacheSet.cacheLines[i].accessTs == 0) {// an empty entry exists
					cacheSet.cacheLines[i] = cacheLine;
					return;
				}
			}
			// else try to evict some entry
			int evictIndex;
			switch (evictionPolicy) {
			case LRU: {
				evictIndex = 0;
				uint64_t accessTs = cacheSet.cacheLines[0].accessTs;
				for (int i = 1; i < numWays; ++i) {
					if (accessTs > cacheSet.cacheLines[i].accessTs) {
						accessTs = cacheSet.cacheLines[i].accessTs;
						evictIndex = i;
					}
				}
				break;
			}
			case RR:
				evictIndex = distr(gen);
				break;
			default:
				;
			}

			if (logEnabled)
				LOG << "Up to evict index " << evictIndex << std::endl;

			lifeSpanStats[cacheSet.accessCnt - cacheSet.cacheLines[evictIndex].accessTs]++;// record cache line's lifespan
			cacheSet.cacheLines[evictIndex] = cacheLine;
		}
	private:
		uint64_t accessCnt;
		uint64_t missCnt;
		std::unordered_map<uint64_t, uint64_t> lifeSpanStats;

		bool logEnabled;
		std::ofstream LOG;

		std::vector<CacheSet> cacheSets;

		int totalSize, lineSize, numWays;
		CacheEvictionPolicy evictionPolicy;

		std::default_random_engine gen;
		std::uniform_int_distribution<int> distr;

		uintptr_t offsetMask, indexMask, tagMask;
		short offsetMaskLen, indexMaskLen, tagMaskLen;
	};
}
