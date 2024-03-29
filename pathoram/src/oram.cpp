#include "oram.hpp"

#include "utility.hpp"

#include <boost/format.hpp>

namespace PathORAM
{
	using namespace std;
	using boost::format;

ORAM::ORAM(
		const number logCapacity,
		const number blockSize,
		const number Z,
		const shared_ptr<AbsStorageAdapter> storage,
		const shared_ptr<AbsPositionMapAdapter> map,
		const shared_ptr<AbsStashAdapter> stash,
		vector<block> &data,
		const bool initialize,
		const number batchSize) :
		storage(storage),
		map(map),
		stash(stash),
		dataSize(blockSize),
		Z(Z),
		height(logCapacity),
		buckets((number)1 << logCapacity),
		blocks(((number)1 << logCapacity) * Z),
		batchSize(batchSize)
	{
		if (initialize)
		{
			// fill all blocks with random bits, marks them as "empty"
			storage->fillWithZeroes();
			for (number i = 0; i < blocks; ++i)
			{
				map->set(i, getRandomULong(1 << (height - 1)));
			}
			load(data);



		}
	}

	ORAM::ORAM(
		const number logCapacity,
		const number blockSize,
		const number Z,
		const shared_ptr<AbsStorageAdapter> storage,
		const shared_ptr<AbsPositionMapAdapter> map,
		const shared_ptr<AbsStashAdapter> stash,
		const bool initialize,
		const number batchSize) :
		storage(storage),
		map(map),
		stash(stash),
		dataSize(blockSize),
		Z(Z),
		height(logCapacity),
		buckets((number)1 << logCapacity),
		blocks(((number)1 << logCapacity) * Z),
		batchSize(batchSize)
	{
		if (initialize)
		{
			// fill all blocks with random bits, marks them as "empty"
			storage->fillWithZeroes();

			// generate random position map
			for (number i = 0; i < blocks; ++i)
			{
				map->set(i, getRandomULong(1 << (height - 1)));
			}
			//Leonie: Set cache in the beginning?
		}
	}

	ORAM::ORAM(const number logCapacity, const number blockSize, const number Z) :
		ORAM(logCapacity,
			 blockSize,
			 Z,
			 make_shared<InMemoryStorageAdapter>((1 << logCapacity), blockSize, bytes(), Z),
			 make_shared<InMemoryPositionMapAdapter>(((1 << logCapacity) * Z) + Z),
			 make_shared<InMemoryStashAdapter>(3 * logCapacity * Z))
	{
	}

	void ORAM::get(const number block, bytes &response)
	{
		bytes data;
		access(true, block, data, response);
		syncCache();
	}

	void ORAM::put(const number block, const bytes &data)
	{
		bytes response;
		access(false, block, data, response);
		syncCache();
	}

	void ORAM::multiple(const vector<block> &requests, vector<bytes> &response)
	{
		#if INPUT_CHECKS
			if (requests.size() > batchSize)
			{
				throw Exception(boost::format("Too many requests (%1%) for batch size %2%") % requests.size() % batchSize);
			}
		#endif

		// populate cache
		unordered_set<number> locations;
		for (auto &&request : requests)
		{
			readPath(map->get(request.first), locations, false);
		}

		vector<block> cacheResponse;
		getCache(locations, cacheResponse, true);

		// run ORAM protocol (will use cache)
		response.resize(requests.size());
		for (auto i = 0u; i < requests.size(); i++)
		{
			access(requests[i].second.size() == 0, requests[i].first, requests[i].second, response[i]);
		}

		// upload resulting new data
		syncCache();
	}

	void ORAM::load(vector<block> &data)
	{
		const number maxLocation = 1 << height;
		const auto bucketCount	 = (data.size() + Z - 1) / Z; // for rounding errors
		const auto step			 = maxLocation / (long double)bucketCount;

		if (bucketCount > maxLocation)
		{
			throw Exception("bulk load: too much data for ORAM");
		}

		vector<pair<const number, bucket>> writeRequests;
		writeRequests.reserve(bucketCount);

		// shuffle (such bulk load may leak in part the original order)
		const uint n = data.size();
		if (n >= 2)
		{
			// Fisher-Yates shuffle
			for (uint i = 0; i < n - 1; i++)
			{
				uint j = i + getRandomUInt(n - i);
				swap(data[i], data[j]);
			}
		}

		auto iteration = 0uLL;
		bucket bucket;
		for (auto &&record : data)
		{
			// to disperse locations evenly from 1 to maxLocation
			const auto location	  = (number)floor(1 + iteration * step);
			const auto [from, to] = leavesForLocation(location);
			map->set(record.first, getRandomULong(to - from + 1) + from);

			if (bucket.size() < Z)
			{
				bucket.push_back(record);
			}
			if (bucket.size() == Z)
			{
				writeRequests.push_back({location, bucket});
				iteration++;
				bucket.clear();
			}
		}
		const auto location = (number)floor(1 + iteration * step);
		if (bucket.size() > 0)
		{
			for (auto i = 0u; i < bucket.size() - Z; i++)
			{
				bucket.push_back({ULONG_MAX, bytes()});
			}
			writeRequests.push_back({location, bucket});
		}

		storage->set(boost::make_iterator_range(writeRequests.begin(), writeRequests.end()));
	}

	void ORAM::access(const bool read, const number block, const bytes &data, bytes &response)
	{
		// step 1 from paper: remap block 
		const auto previousPosition = map->get(block);
		//TODO: anders als im Paper
		map->set(block, getRandomULong(1 << (height - 1)));

		// step 2 from paper: read path
		unordered_set<number> path;
		readPath(previousPosition, path, true); // stash updated

		// step 3 from paper: update block
		if (!read) // if "write"
		{
			stash->update(block, data);
		}
		stash->get(block, response);

		// step 4 from paper: write path
		writePath(previousPosition); // stash updated
	}

	void ORAM::readPath(const number leaf, unordered_set<number> &path, const bool putInStash)
	{
		// for levels from root to leaf
		for (number level = 0; level < height; level++)
		{
			const auto bucket = bucketForLevelLeaf(level, leaf);
			path.insert(bucket);
		}

		// we may only want to populate cache
		if (putInStash)
		{
			vector<block> blocks;
			getCache(path, blocks, false);

			for (auto &&[id, data] : blocks)
			{
				// skip "empty" buckets
				if (id != ULONG_MAX)
				{
					stash->add(id, data);
				}
			}
		}
	}

	void ORAM::writePath(const number leaf)
	{
		vector<block> currentStash;
		stash->getAll(currentStash);

		vector<int> toDelete;				   // remember the records that will need to be deleted from stash
		vector<pair<number, bucket>> requests; // storage SET requests (batching)

		// following the path from leaf to root (greedy)
		for (int level = height - 1; level >= 0; level--)
		{
			vector<block> toInsert;		  // block to be insterted in the bucket (up to Z)
			vector<number> toDeleteLocal; // same blocks needs to be deleted from stash (these hold indices of elements in currentStash)

			for (number i = 0; i < currentStash.size(); i++)
			{
				const auto &entry	 = currentStash[i];
				const auto entryLeaf = map->get(entry.first);
				// see if this block from stash fits in this bucket
				
				if (canInclude(entryLeaf, leaf, level))
				{
					toInsert.push_back(entry);
					toDelete.push_back(entry.first);

					toDeleteLocal.push_back(i);

					// look up to Z
					if (toInsert.size() == Z)
					{
						break;
					}
				}
			}
			// delete inserted blocks from local stash
			// we remove elements by location, so after operation vector shrinks (nasty bug...)
			sort(toDeleteLocal.begin(), toDeleteLocal.end(), greater<number>());
			for (auto &&removed : toDeleteLocal)
			{
				currentStash.erase(currentStash.begin() + removed);
			}

			const auto bucketId = bucketForLevelLeaf(level, leaf);
			bucket bucket;
			bucket.resize(Z);

			// write the bucket
			for (number i = 0; i < Z; i++)
			{
				// auto block = bucket * Z + i;
				if (toInsert.size() != 0)
				{
					bucket[i] = toInsert.back();
					toInsert.pop_back();
				}
				else
				{
					// if nothing to insert, insert dummy (for security)
					bucket[i] = {ULONG_MAX, getRandomBlock(dataSize)};
				}
			}

			requests.push_back({bucketId, bucket});
		}

		setCache(requests);

		// update the stash adapter, remove newly inserted blocks
		for (auto &&removed : toDelete)
		{
			stash->remove(removed);
		}
	}

	number ORAM::bucketForLevelLeaf(const number level, const number leaf) const
	{
		return (leaf + (1 << (height - 1))) >> (height - 1 - level);
	}

	bool ORAM::canInclude(const number pathLeaf, const number blockPosition, const number level) const
	{
		// on this level, do these paths share the same bucket
		return bucketForLevelLeaf(level, pathLeaf) == bucketForLevelLeaf(level, blockPosition);
	}

	pair<number, number> ORAM::leavesForLocation(const number location)
	{
		const auto level	= (number)floor(log2(location));
		const auto toLeaves = height - level - 1;
		return {location * (1 << toLeaves) - (1 << (height - 1)), (location + 1) * (1 << toLeaves) - 1 - (1 << (height - 1))};
	}

	void ORAM::getCache(const unordered_set<number> &locations, vector<block> &response, const bool dryRun)
	{
		// get those locations not present in the cache
		vector<number> toGet;
		for (auto &&location : locations)
		{
			const auto bucketIt = cache.find(location);
			if (bucketIt == cache.end())
			{
				toGet.push_back(location);
			}
			else if (!dryRun)
			{
				response.insert(response.begin(), (*bucketIt).second.begin(), (*bucketIt).second.end());
			}
		}

		if (toGet.size() > 0)
		{
			// download those blocks
			vector<block> downloaded;
			storage->get(toGet, downloaded);

			// add them to the cache and the result
			bucket bucket;
			for (auto i = 0uLL; i < downloaded.size(); i++)
			{
				if (!dryRun)
				{
					response.push_back(downloaded[i]);
				}
				bucket.push_back(downloaded[i]);
				if (i % Z == Z - 1)
				{
					cache[toGet[i / Z]] = bucket;
					bucket.clear();
				}
			}
		}
	}

	void ORAM::setCache(const vector<pair<number, bucket>> &requests)
	{
		for (auto &&request : requests)
		{
			cache[request.first] = request.second;
		}
	}

	void ORAM::syncCache()
	{
		storage->set(boost::make_iterator_range(cache.begin(), cache.end()));

		cache.clear();
	}
}
