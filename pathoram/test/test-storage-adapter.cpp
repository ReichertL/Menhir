#include "definitions.h"
#include "storage-adapter.hpp"
#include "utility.hpp"

#include "gtest/gtest.h"
#include <boost/format.hpp>
#include <fstream>
#include <openssl/aes.h>

using namespace std;

namespace PathORAM
{
	enum TestingStorageAdapterType
	{
#if USE_REDIS
		StorageAdapterTypeRedis,
#endif
#if USE_AEROSPIKE
		StorageAdapterTypeAerospike,
#endif
		StorageAdapterTypeInMemory,
		StorageAdapterTypeFileSystem
	};

	class StorageAdapterTest : public testing::TestWithParam<TestingStorageAdapterType>
	{
		public:
		inline static const number CAPACITY	  = 10;
		inline static const number BLOCK_SIZE = 32;
		inline static const number Z		  = 3;
		inline static const string FILE_NAME  = "storage.bin";

#if USE_REDIS
		inline static string REDIS_HOST = "tcp://127.0.0.1:6379";
#endif

#if USE_AEROSPIKE
		inline static string AEROSPIKE_HOST = "127.0.0.1";
#endif

		protected:
		unique_ptr<AbsStorageAdapter> adapter;

		StorageAdapterTest()
		{
			adapter = createAdapter(0);
		}

		unique_ptr<AbsStorageAdapter> createAdapter(number batchLimit)
		{
			auto type = GetParam();
			switch (type)
			{
				case StorageAdapterTypeInMemory:
					return make_unique<InMemoryStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), Z, batchLimit);
				case StorageAdapterTypeFileSystem:
					return make_unique<FileSystemStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), FILE_NAME, true, Z, batchLimit);
#if USE_REDIS
				case StorageAdapterTypeRedis:
					return make_unique<RedisStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), REDIS_HOST, true, Z, batchLimit);
#endif
#if USE_AEROSPIKE
				case StorageAdapterTypeAerospike:
					return make_unique<AerospikeStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), AEROSPIKE_HOST, true, Z, "default", batchLimit);
#endif
				default:
					throw Exception(boost::format("TestingStorageAdapterType %1% is not implemented") % type);
			}
		}

		~StorageAdapterTest() override
		{
			remove(FILE_NAME.c_str());
			adapter.reset();
#if USE_REDIS
			if (GetParam() == StorageAdapterTypeRedis)
			{
				make_unique<sw::redis::Redis>(REDIS_HOST)->flushall();
			}
#endif
		}

		bucket generateBucket(number from)
		{
			bucket bucket;
			for (auto i = 0uLL; i < Z; i++)
			{
				bucket.push_back({from + i, fromText("hello" + to_string(from + i), BLOCK_SIZE)});
			}
			return bucket;
		}
	};

	TEST_P(StorageAdapterTest, Initialization)
	{
		SUCCEED();
	}

	TEST_P(StorageAdapterTest, RecoverAfterCrash)
	{
		auto param		   = GetParam();
		auto createAdapter = [param](string filename, bool override, bytes key) -> unique_ptr<AbsStorageAdapter> {
			switch (param)
			{
				case StorageAdapterTypeFileSystem:
					return make_unique<FileSystemStorageAdapter>(CAPACITY, BLOCK_SIZE, key, filename, override, Z);
#if USE_REDIS
				case StorageAdapterTypeRedis:
					return make_unique<RedisStorageAdapter>(CAPACITY, BLOCK_SIZE, key, REDIS_HOST, override, Z);
#endif
#if USE_AEROSPIKE
				case StorageAdapterTypeAerospike:
					return make_unique<AerospikeStorageAdapter>(CAPACITY, BLOCK_SIZE, key, AEROSPIKE_HOST, override, Z);
#endif
				default:
					throw Exception(boost::format("TestingStorageAdapterType %1% is not persistent") % param);
			}
		};

		if (GetParam() != StorageAdapterTypeInMemory)
		{
			auto bucket = generateBucket(5);
			auto key	= getRandomBlock(KEYSIZE);

			string filename = "tmp.bin";
			auto storage	= createAdapter(filename, true, key);

			storage->set(CAPACITY - 1, bucket);
			vector<block> got;
			storage->get(CAPACITY - 1, got);
			ASSERT_EQ(bucket, got);
			storage.reset();

			storage = createAdapter(filename, false, key);

			got.clear();
			storage->get(CAPACITY - 1, got);
			ASSERT_EQ(bucket, got);

			remove(filename.c_str());
		}
		else
		{
			SUCCEED();
		}
	}

	TEST_P(StorageAdapterTest, CrashAtInitialization)
	{
		switch (GetParam())
		{
			case StorageAdapterTypeFileSystem:
				ASSERT_ANY_THROW(make_unique<FileSystemStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), "tmp.bin", false, Z));
				break;
#if USE_REDIS
			case StorageAdapterTypeRedis:
				ASSERT_ANY_THROW(make_unique<RedisStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), "error", false, Z));
				break;
#endif
#if USE_AEROSPIKE
			case StorageAdapterTypeAerospike:
				ASSERT_ANY_THROW(make_unique<AerospikeStorageAdapter>(CAPACITY, BLOCK_SIZE, bytes(), "error", false, Z));
				break;
#endif
			default:
				SUCCEED();
				break;
		}
	}

	TEST_P(StorageAdapterTest, InputsCheck)
	{
		//ASSERT_ANY_THROW(make_unique<InMemoryStorageAdapter>(CAPACITY, AES_BLOCK_SIZE, bytes(), Z));
		//ASSERT_ANY_THROW(make_unique<InMemoryStorageAdapter>(CAPACITY, AES_BLOCK_SIZE * 3 - 1, bytes(), Z));
		ASSERT_ANY_THROW(make_unique<InMemoryStorageAdapter>(CAPACITY, AES_BLOCK_SIZE * 2, bytes(), 0));
	}

	TEST_P(StorageAdapterTest, ReadWriteNoCrash)
	{
		auto bucket = generateBucket(5);

		EXPECT_NO_THROW({
			vector<block> got;
			adapter->set(CAPACITY - 1, bucket);
			adapter->get(CAPACITY - 2, got);
		});
	}

	TEST_P(StorageAdapterTest, ReadEmpty)
	{
		vector<block> bucket;
		adapter->get(CAPACITY - 2, bucket);
		ASSERT_EQ(Z, bucket.size());
		for (auto &&block : bucket)
		{
			ASSERT_EQ(BLOCK_SIZE, block.second.size());
		}
	}

	TEST_P(StorageAdapterTest, IdOutOfBounds)
	{
		auto bucket = generateBucket(5);

		ASSERT_ANY_THROW({
			vector<block> got;
			adapter->get(CAPACITY + 1, got);
		});
		ASSERT_ANY_THROW(adapter->set(CAPACITY + 1, bucket));
	}

	TEST_P(StorageAdapterTest, DataTooBig)
	{
		auto bucket = generateBucket(5);
		bucket[0].second.resize(BLOCK_SIZE + 1, 0x08);

		ASSERT_ANY_THROW(adapter->set(CAPACITY - 1, bucket));
	}

	TEST_P(StorageAdapterTest, NotZBlocks)
	{
		auto bucket = generateBucket(5);
		bucket.push_back({1, bytes()});

		ASSERT_ANY_THROW(adapter->set(CAPACITY - 1, bucket));
	}

	TEST_P(StorageAdapterTest, ReadWhatWasWritten)
	{
		auto bucket = generateBucket(5);

		adapter->set(CAPACITY - 1, bucket);
		vector<block> returned;
		adapter->get(CAPACITY - 1, returned);

		ASSERT_EQ(bucket, returned);
	}

	// if get/set internal for batching are implemented, they are used
	// but get/set internal single still has to work
	TEST_P(StorageAdapterTest, GetSetInternal)
	{
		auto data = bytes{0xa8};
		data.resize(Z * ((BLOCK_SIZE) + sizeof(number)));//AES_BLOCK_SIZE) + AES_BLOCK_SIZE);

		adapter->setInternal(CAPACITY - 1, data);
		bytes returned;
		adapter->getInternal(CAPACITY - 1, returned);

		ASSERT_EQ(data, returned);
	}

	TEST_P(StorageAdapterTest, OverrideData)
	{
		auto bucket = generateBucket(5);

		adapter->set(CAPACITY - 1, bucket);

		bucket = generateBucket(5 + Z + 2);

		adapter->set(CAPACITY - 1, bucket);

		vector<block> returned;
		adapter->get(CAPACITY - 1, returned);

		ASSERT_EQ(bucket, returned);
	}

	TEST_P(StorageAdapterTest, InitializeToEmpty)
	{
		for (number i = 0; i < CAPACITY; i++)
		{
			auto expected = bytes();
			expected.resize(BLOCK_SIZE);

			vector<block> returned;
			adapter->get(i, returned);

			ASSERT_EQ(Z, returned.size());
			for (auto &&block : returned)
			{
				ASSERT_EQ(ULONG_MAX, block.first);
				ASSERT_EQ(expected, block.second);
			}
		}
	}

	TEST_P(StorageAdapterTest, BatchReadWrite)
	{
		const auto runs = 3;

		vector<pair<const number, vector<pair<number, bytes>>>> writes;
		vector<number> reads;
		for (auto i = 0; i < runs; i++)
		{
			writes.push_back({CAPACITY - 4 + i, generateBucket(i * Z)});
			reads.push_back(CAPACITY - 4 + i);
		}
		adapter->set(boost::make_iterator_range(writes.begin(), writes.end()));

		vector<block> read;
		adapter->get(reads, read);

		ASSERT_EQ(runs * Z, read.size());
		for (auto &&block : read)
		{
			auto found = false;
			for (auto &&write : writes)
			{
				for (auto &&wBlock : write.second)
				{
					if (wBlock.first == block.first)
					{
						EXPECT_EQ(wBlock, block);
						found = true;
					}
				}
			}
			EXPECT_TRUE(found);
		}
	}

	TEST_P(StorageAdapterTest, EventHandling)
	{
		tuple<bool, number, number, number> event;

		auto connection = adapter->subscribe([&event](bool read, number batch, number size, number overhead) -> void {
			get<0>(event) = read;
			get<1>(event) = batch;
			get<2>(event) = size;
			get<3>(event) = overhead;
		});

		auto bucket		   = generateBucket(5);
		const auto rawSize = (BLOCK_SIZE + sizeof(number)) * Z; 

		adapter->set(CAPACITY - 1, bucket);

		EXPECT_FALSE(get<0>(event));
		EXPECT_EQ(1, get<1>(event));
		EXPECT_EQ(rawSize, get<2>(event));
		EXPECT_LT(0, get<3>(event));

		vector<block> got;
		adapter->get(CAPACITY - 1, got);

		EXPECT_TRUE(get<0>(event));
		EXPECT_EQ(1, get<1>(event));
		EXPECT_EQ(rawSize, get<2>(event));
		EXPECT_LT(0, get<3>(event));

		vector<pair<const number, vector<block>>> requests = {{CAPACITY - 1, bucket}, {CAPACITY - 2, bucket}};
		adapter->set(boost::make_iterator_range(requests.begin(), requests.end()));

		EXPECT_FALSE(get<0>(event));
		EXPECT_EQ(adapter->supportsBatchSet() ? 2 : 1, get<1>(event));
		EXPECT_EQ(
			(adapter->supportsBatchSet() ? 2 : 1) * rawSize,
			get<2>(event));
		EXPECT_LT(0, get<3>(event));

		got.clear();
		vector<number> locations = {CAPACITY - 1, CAPACITY - 2};
		adapter->get(locations, got);

		EXPECT_TRUE(get<0>(event));
		EXPECT_EQ(adapter->supportsBatchGet() ? 2 : 1, get<1>(event));
		EXPECT_EQ(
			(adapter->supportsBatchGet() ? 2 : 1) * rawSize,
			get<2>(event));
		EXPECT_LT(0, get<3>(event));

		connection.disconnect();
	}

	TEST_P(StorageAdapterTest, BatchLimit)
	{
		const auto BATCH_LIMIT = 3uLL;

		vector<tuple<bool, number, number, number>> events;

		auto adapter	= createAdapter(BATCH_LIMIT);
		auto connection = adapter->subscribe([&events](bool read, number batch, number size, number overhead) -> void {
			events.push_back({read, batch, size, overhead});
		});

		vector<pair<const number, bucket>> setRequests;
		vector<number> getRequests;
		for (auto i = 0uLL; i < BATCH_LIMIT * 5 / 2; i++)
		{
			setRequests.push_back({i, generateBucket(i)});
			getRequests.push_back(i);
		}
		adapter->set(boost::make_iterator_range(setRequests.begin(), setRequests.end()));

		if (adapter->supportsBatchSet())
		{
			EXPECT_EQ(3, events.size());
			EXPECT_EQ(BATCH_LIMIT, get<1>(events[0]));
			EXPECT_EQ(BATCH_LIMIT, get<1>(events[1]));
			EXPECT_EQ(BATCH_LIMIT / 2, get<1>(events[2]));
		}
		else
		{
			EXPECT_EQ(BATCH_LIMIT * 5 / 2, events.size());
			for (auto i = 0uLL; i < BATCH_LIMIT * 5 / 2; i++)
			{
				EXPECT_EQ(1, get<1>(events[i]));
			}
		}
		events.clear();

		vector<pair<number, bytes>> getResponse;
		getResponse.reserve(BATCH_LIMIT * 5 / 2);
		adapter->get(getRequests, getResponse);

		EXPECT_EQ(getRequests.size() * Z, getResponse.size());

		if (adapter->supportsBatchGet())
		{
			EXPECT_EQ(3, events.size());
			EXPECT_EQ(BATCH_LIMIT, get<1>(events[0]));
			EXPECT_EQ(BATCH_LIMIT, get<1>(events[1]));
			EXPECT_EQ(BATCH_LIMIT / 2, get<1>(events[2]));
		}
		else
		{
			EXPECT_EQ(BATCH_LIMIT * 5 / 2, events.size());
			for (auto i = 0uLL; i < BATCH_LIMIT * 5 / 2; i++)
			{
				EXPECT_EQ(1, get<1>(events[i]));
			}
		}
	}

	string printTestName(testing::TestParamInfo<TestingStorageAdapterType> input)
	{
		switch (input.param)
		{
			case StorageAdapterTypeInMemory:
				return "InMemory";
			case StorageAdapterTypeFileSystem:
				return "FileSystem";
#if USE_REDIS
			case StorageAdapterTypeRedis:
				return "Redis";
#endif
#if USE_AEROSPIKE
			case StorageAdapterTypeAerospike:
				return "Aerospike";
#endif
			default:
				throw Exception(boost::format("TestingStorageAdapterType %1% is not implemented") % input.param);
		}
	}

	vector<TestingStorageAdapterType> cases()
	{
		vector<TestingStorageAdapterType> result = {StorageAdapterTypeFileSystem, StorageAdapterTypeInMemory};

#if USE_REDIS
		for (auto host : vector<string>{"127.0.0.1", "redis"})
		{
			try
			{
				// test if Redis is availbale
				auto connection = "tcp://" + host + ":6379";
				make_unique<sw::redis::Redis>(connection)->ping();
				result.push_back(StorageAdapterTypeRedis);
				PathORAM::StorageAdapterTest::REDIS_HOST = connection;
				break;
			}
			catch (...)
			{
			}
		}
#endif

#if USE_AEROSPIKE
		for (auto host : vector<string>{"127.0.0.1", "aerospike"})
		{
			try
			{
				// test if Aerospike is availbale
				as_config config;
				as_config_init(&config);
				as_config_add_host(&config, host.c_str(), 3000);

				aerospike aerospike;
				aerospike_init(&aerospike, &config);

				as_error err;
				aerospike_connect(&aerospike, &err);

				if (err.code == AEROSPIKE_OK)
				{
					result.push_back(StorageAdapterTypeAerospike);
					PathORAM::StorageAdapterTest::AEROSPIKE_HOST = host;
					break;
				}
			}
			catch (...)
			{
			}
		}
#endif

		return result;
	};

	INSTANTIATE_TEST_SUITE_P(StorageAdapterSuite, StorageAdapterTest, testing::ValuesIn(cases()), printTestName);
}

int main(int argc, char **argv)
{
	srand(TEST_SEED);

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
