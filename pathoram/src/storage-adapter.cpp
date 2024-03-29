#include "storage-adapter.hpp"


#include <algorithm>
#include <chrono>
#include <boost/format.hpp>
#include <cstring>
#include <openssl/aes.h>
#include <utility.hpp>
#include <vector>

#include <iostream>

#define RECORD_AND_EXECUTE(condition, plain, record)                                                            \
	if (condition)                                                                                              \
	{                                                                                                           \
		plain;                                                                                                  \
	}                                                                                                           \
	else                                                                                                        \
	{                                                                                                           \
		auto start = chrono::steady_clock::now();                                                               \
		plain;                                                                                                  \
		auto elapsed = chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now() - start).count(); \
		record;                                                                                                 \
	}

namespace PathORAM
{
	using namespace std;
	using boost::format;

#pragma region AbsStorageAdapter

	AbsStorageAdapter::~AbsStorageAdapter()
	{
	}

	void AbsStorageAdapter::get(const vector<number> &locations, vector<block> &response) const
	{
		
		//cout<<"get()\n";
		for (auto &&location : locations)
		{
			checkCapacity(location);
		}

		// optimize for single operation
		vector<bytes> raws;
		raws.reserve(locations.size());

		if (locations.size() == 1)
		{
			//cout<<"case 1\n";
			raws.resize(1);
			getAndRecord(locations[0], raws[0]);
		}
		else
		{
			if (batchLimit == 0 || locations.size() <= batchLimit)
			{
				//cout<<"case 2\n";
				getAndRecord(locations, raws);
			}
			else
			{
				//cout<<"case 3\n";
				vector<number> batch;
				number pointer = 0;
				while (pointer < locations.size())
				{
					batch.reserve(min(batchLimit, locations.size() - pointer));
					copy(
						locations.begin() + pointer,
						(number)distance(locations.begin() + pointer, locations.end()) > batchLimit ?
							  locations.begin() + pointer + batchLimit :
							  locations.end(),
						back_inserter(batch));
					getAndRecord(batch, raws);
					batch.clear();
					pointer += batchLimit;
				}
			}
		}

		response.reserve(locations.size() * Z);
		for (auto &&raw : raws)
		{
			// decompose to ID and cipher

			/*
			bytes decrypted;
			encrypt(
				key.begin(),
				key.end(),
				raw.begin(),
				raw.begin() + AES_BLOCK_SIZE,
				raw.begin() + AES_BLOCK_SIZE,
				raw.end(),
				decrypted,
				DECRYPT);
			

			const auto length = decrypted.size() / Z;




			for (auto i = 0uLL; i < Z; i++)
			{
				// decompose to ID and data (extract ID from bytes)
				uchar buffer[AES_BLOCK_SIZE];
				copy(decrypted.begin() + i * length, decrypted.begin() + i * length + AES_BLOCK_SIZE, buffer);

				response.push_back(
					{((number *)buffer)[0],
					 bytes(decrypted.begin() + i * length + AES_BLOCK_SIZE, decrypted.begin() + (i + 1) * length)});
			}

			*/
			//std::cout<<toText(raw,raw.size())<<"\n";
			//bytes decrypted (raw.begin() + AES_BLOCK_SIZE,raw.end());
			bytes decrypted (raw.begin(),raw.end());
			//copy(raw.begin() + AES_BLOCK_SIZE,raw.end(), decrypted.begin());
			
			//std::cout<<toText(decrypted,decrypted.size())<<"\n";

			const auto length = decrypted.size() / Z;
			for (auto i = 0uLL; i < Z; i++)
			{
				// decompose to ID and data (extract ID from bytes)
				uchar buffer[sizeof(number)];
				copy(decrypted.begin() + i * length, decrypted.begin() + i * length + sizeof(number), buffer);

				response.push_back(
					{((number *)buffer)[0],
					 bytes(decrypted.begin() + i * length +  sizeof(number), decrypted.begin() + (i + 1) * length)});
			}
		}
	}

	void AbsStorageAdapter::set(const request_anyrange requests)
	{
		//cout<<"set()\n";
		vector<block> writes;

		for (auto &&[location, blocks] : requests)
		{
			checkCapacity(location);

#if INPUT_CHECKS
			if (blocks.size() != Z)
			{
				throw Exception(boost::format("each set request must contain exactly Z=%1% blocks (%2% given)") % Z % blocks.size());
			}
#endif

			bytes toEncrypt;
			toEncrypt.reserve( (sizeof(number) + userBlockSize) * Z);
			int jj=0;
			for (auto &&block : blocks)
			{
				//std::cout<<"block "<<jj<<"\n";
				jj+=1;
				checkBlockSize(block.second.size());

				// pad if necessary
				if (block.second.size() < userBlockSize)
				{
					block.second.resize(userBlockSize, 0x00);
				}

				// represent ID as a vector of bytes of length AES_BLOCK_SIZE
				const number buffer[1] = {block.first};
				bytes id((uchar *)buffer, (uchar *)buffer + sizeof(number));
				//id.resize(AES_BLOCK_SIZE, 0x00);

				// merge ID and data
				toEncrypt.insert(toEncrypt.end(), id.begin(), id.end());
				//std::cout<<toText(id,id.size())<<"\n";
				toEncrypt.insert(toEncrypt.end(), block.second.begin(), block.second.end());
				//std::cout<<toText(block.second,block.second.size()+1)<<"\n";
			}

			/*auto iv = getRandomBlock(AES_BLOCK_SIZE);
			bytes encrypted;
			encrypt(
				key.begin(),
				key.end(),
				iv.begin(),
				iv.end(),
				toEncrypt.begin(),
				toEncrypt.end(),
				iv, // append result to IV
				ENCRYPT);
			writes.push_back({location, iv});*/
			bytes iv;// = getRandomBlock(AES_BLOCK_SIZE);
			iv.insert(iv.end(),toEncrypt.begin(),toEncrypt.end());
			writes.push_back({location, iv}); //Change by menhir: TEE takes care of keeping the data private
			//std::cout<<toText(iv,iv.size()+1)<<"\n";

		}

		// optimize for single operation
		if (writes.size() == 1)
		{
			setAndRecord(writes[0].first, writes[0].second);
		}
		else
		{
			if (batchLimit == 0 || writes.size() <= batchLimit)
			{
				setAndRecord(writes);
			}
			else
			{
				vector<pair<number, bytes>> batch;
				number pointer = 0;
				while (pointer < writes.size())
				{
					batch.reserve(min(batchLimit, writes.size() - pointer));
					copy(
						writes.begin() + pointer,
						(number)distance(writes.begin() + pointer, writes.end()) > batchLimit ?
							  writes.begin() + pointer + batchLimit :
							  writes.end(),
						back_inserter(batch));
					setAndRecord(batch);
					batch.clear();
					pointer += batchLimit;
				}
			}
		}
	}

	void AbsStorageAdapter::get(const number location, bucket &response) const
	{
		const auto locations = vector<number>{location};
		get(locations, response);
	}

	void AbsStorageAdapter::set(const number location, const bucket &data)
	{
		const auto requests = vector<pair<const number, vector<block>>>{{location, data}};

		set(boost::make_iterator_range(requests.begin(), requests.end()));
	}

	void AbsStorageAdapter::getInternal(const vector<number> &locations, vector<bytes> &response) const
	{
		response.resize(response.size() + locations.size());
		for (unsigned int i = 0; i < locations.size(); i++)
		{
			getAndRecord(locations[i], response[response.size() - locations.size() + i]);
		}
	}

	void AbsStorageAdapter::setInternal(const vector<block> &requests)
	{
		for (auto &&request : requests)
		{
			setAndRecord(request.first, request.second);
		}
	}

	void AbsStorageAdapter::checkCapacity(const number location) const
	{
#if INPUT_CHECKS
		if (location >= capacity)
		{
			throw Exception(boost::format("id %1% out of bound (capacity %2%)") % location % capacity);
		}
#endif
	}

	void AbsStorageAdapter::checkBlockSize(const number dataLength) const
	{
#if INPUT_CHECKS
		if (dataLength > userBlockSize)
		{
			throw Exception(boost::format("data of size %1% is too long for a block of %2% bytes") % dataLength % userBlockSize);
		}
#endif
	}

	AbsStorageAdapter::AbsStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const number Z, const number batchLimit) :
		key(key.size() == KEYSIZE ? key : getRandomBlock(KEYSIZE)),
		Z(Z),
		batchLimit(batchLimit),
		capacity(capacity),
		blockSize((userBlockSize + sizeof(number)) * Z ),//blockSize((userBlockSize + AES_BLOCK_SIZE) * Z + AES_BLOCK_SIZE), // IV + Z * (ID + PAYLOAD)
		userBlockSize(userBlockSize)
	{
		/*if (userBlockSize < 2 * AES_BLOCK_SIZE)
		{
			throw Exception(boost::format("block size %1% is too small, need at least %2%") % userBlockSize % (2 * AES_BLOCK_SIZE));
		}

		if (userBlockSize % AES_BLOCK_SIZE != 0)
		{
			throw Exception(boost::format("block size must be a multiple of %1% (provided %2% bytes)") % AES_BLOCK_SIZE % userBlockSize);
		}*/

		if (Z == 0)
		{
			throw Exception(boost::format("Z must be greater than zero (provided %1%)") % Z);
		}
	}

	void AbsStorageAdapter::fillWithZeroes()
	{
		vector<pair<const number, bucket>> requests;
		requests.reserve(capacity);

		for (auto i = 0uLL; i < capacity; i++)
		{
			bucket bucket;
			for (auto j = 0uLL; j < Z; j++)
			{
				bucket.push_back({ULONG_MAX, bytes()});
			}

			requests.push_back({i, bucket});
		}

		set(boost::make_iterator_range(requests.begin(), requests.end()));
	}

	boost::signals2::connection AbsStorageAdapter::subscribe(const OnStorageRequest::slot_type &handler)
	{
		return onStorageRequest.connect(handler);
	}

	void AbsStorageAdapter::setAndRecord(const number location, const bytes &raw)
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty(),
			setInternal(location, raw),
			onStorageRequest(false, 1, raw.size(), elapsed));
	}

	void AbsStorageAdapter::getAndRecord(const number location, bytes &response) const
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty(),
			getInternal(location, response),
			onStorageRequest(true, 1, response.size(), elapsed));
	}

	void AbsStorageAdapter::setAndRecord(const vector<pair<number, bytes>> &requests)
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty() || !supportsBatchSet(),
			setInternal(requests),
			{
				auto size = 0;
				for (auto &&request : requests)
				{
					size += request.second.size();
				}
				onStorageRequest(false, requests.size(), size, elapsed);
			});
	}

	void AbsStorageAdapter::getAndRecord(const vector<number> &locations, vector<bytes> &response) const
	{
		RECORD_AND_EXECUTE(
			onStorageRequest.empty() || !supportsBatchGet(),
			getInternal(locations, response),
			{
				auto size = 0;
				for (auto raw = response.end() - locations.size(); raw != response.end(); raw++)
				{
					size += (*raw).size();
				}
				onStorageRequest(true, locations.size(), size, elapsed);
			});
	}

#pragma endregion AbsStorageAdapter

#pragma region InMemoryStorageAdapter

	InMemoryStorageAdapter::~InMemoryStorageAdapter()
	{
		for (number i = 0; i < capacity; i++)
		{
			delete[] blocks[i];
		}
		delete[] blocks;
	}






	InMemoryStorageAdapter::InMemoryStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const number Z, const number batchLimit) :
		AbsStorageAdapter(capacity, userBlockSize, key, Z, batchLimit),
		blocks(new uchar *[capacity])
	{
		for (auto i = 0uLL; i < capacity; i++)
		{
			blocks[i] = new uchar[blockSize];
		}


		//fillWithZeroes();
		
	}


	void InMemoryStorageAdapter::getInternal(const number location, bytes &response) const
	{
		response.insert(response.begin(), blocks[location], blocks[location] + blockSize);
	}

	void InMemoryStorageAdapter::setInternal(const number location, const bytes &raw)
	{
		copy(raw.begin(), raw.end(), blocks[location]);
	}

#pragma endregion InMemoryStorageAdapter

#pragma region FileSystemStorageAdapter

	FileSystemStorageAdapter::~FileSystemStorageAdapter()
	{
		file->close();
	}

	FileSystemStorageAdapter::FileSystemStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const string filename, const bool override, const number Z, const number batchLimit) :
		AbsStorageAdapter(capacity, userBlockSize, key, Z, batchLimit),
		file(make_unique<fstream>())
	{
		auto flags = fstream::in | fstream::out | fstream::binary;
		if (override)
		{
			flags |= fstream::trunc;
		}

		file->open(filename, flags);
		if (!file->is_open())
		{
			throw Exception(boost::format("cannot open %1%: %2%") % filename % strerror(errno));
		}

		if (override)
		{
			file->seekg(0, file->beg);
			uchar placeholder[blockSize];

			for (number i = 0; i < capacity; i++)
			{
				file->write((const char *)placeholder, blockSize);
			}

			fillWithZeroes();
		}
	}

	void FileSystemStorageAdapter::getInternal(const number location, bytes &response) const
	{
		uchar placeholder[blockSize];
		file->seekg(location * blockSize, file->beg);
		file->read((char *)placeholder, blockSize);

		response.insert(response.begin(), placeholder, placeholder + blockSize);
	}

	void FileSystemStorageAdapter::setInternal(const number location, const bytes &raw)
	{
		uchar placeholder[blockSize];
		copy(raw.begin(), raw.end(), placeholder);

		file->seekp(location * blockSize, file->beg);
		file->write((const char *)placeholder, blockSize);
	}

#pragma endregion FileSystemStorageAdapter

}
