#pragma once

#include "definitions.h"

#include <boost/range/any_range.hpp>
#include <boost/signals2/signal.hpp>
#include <fstream>


namespace PathORAM
{
	using namespace std;

	// range abstraction that is iterable (will be used for vector of pairs and unordered map)
	using request_anyrange = boost::any_range<pair<const number, bucket>, boost::forward_traversal_tag>;

	/**
	 * @brief An abstraction over storage adapter
	 *
	 * The format of the underlying block is the following.
	 * AES block size (16) bytes of IV, the rest is ciphertext.
	 * The ciphertext is AES block size (16) of ID (padded), the rest is user's payload.
	 */
	class AbsStorageAdapter
	{
		using OnStorageRequest = boost::signals2::signal<void(const bool read, const number batch, const number size, const number overhead)>;

		private:
		/**
		 * @brief throws exception if a requested location is outside of capcity
		 *
		 * @param location location in question
		 */
		void checkCapacity(const number location) const;

		/**
		 * @brief check if a given data size is not above the block size
		 *
		 * @param dataSize size of the data to be inserted
		 */
		void checkBlockSize(const number dataSize) const;

		/**
		 * @brief Proxy for setInternal(number location, bytes raw) that emits OnStorageRequest
		 */
		void setAndRecord(const number location, const bytes &raw);

		/**
		 * @brief Proxy for getInternal(number location, bytes &response) that emits OnStorageRequest
		 */
		void getAndRecord(const number location, bytes &response) const;

		/**
		 * @brief Proxy for setInternal(vector<pair<number, bytes>> &requests) that emits OnStorageRequest
		 */
		void setAndRecord(const vector<pair<number, bytes>> &requests);

		/**
		 * @brief Proxy for getInternal(vector<number> &locations, vector<bytes> &response) that emits OnStorageRequest
		 */
		void getAndRecord(const vector<number> &locations, vector<bytes> &response) const;

		const bytes key;		 // AES key for encryption operations
		const number Z;			 // number of blocks in a bucket
		const number batchLimit; // maximum number of requests in a batch

		// Event handler
		OnStorageRequest onStorageRequest;

		friend class StorageAdapterTest_GetSetInternal_Test;
		friend class MockStorage;

		public:
		/**
		 * @brief retrieves the data from the location
		 *
		 * @param location location in question
		 * @param response retrived data broken up into Z blocks {ID, decrypted payload}
		 */
		void get(const number location, bucket &response) const;

		/**
		 * @brief writes the data to the location
		 *
		 * It will encrypt the data before putting it to storage.
		 *
		 * @param location location in question
		 * @param data composition of Z blocks {ID, plaintext payload}
		 */
		void set(const number location, const bucket &data);

		/**
		 * @brief Subscribes to OnStorageRequest notifications.
		 *
		 * @param handler the handler to execute with event
		 * @return boost::signals2::connection the connection object (to be used for unsubscribing)
		 */
		boost::signals2::connection subscribe(const OnStorageRequest::slot_type &handler);

		/**
		 * @brief retrives the data in batch
		 *
		 * \note
		 * Particular storage provider may or may not support batching.
		 * If it does not, batch will be executed sequentially.
		 *
		 * @param locations the locations from which to read
		 * @param response retrived data broken up into IDs and decrypted payloads
		 */
		void get(const vector<number> &locations, vector<block> &response) const;

		/**
		 * @brief writes the data in batch
		 *
		 * \note
		 * Particular storage provider may or may not support batching.
		 * If it does not, batch will be executed sequentially.
		 *
		 * @param requests locations and data requests (IDs and payloads) to write
		 * \note
		 * request_range is usually constructed by boost::make_iterator_range(container.begin(), container.end()),
		 * where container is any iterable entity that yields pair<const number, bucket> (notice const).
		 */
		void set(const request_anyrange requests);

		/**
		 * @brief sets all available locations (given by CAPACITY) to zeroed bytes.
		 * On the storage these zeroes will appear randomized encrypted.
		 */
		void fillWithZeroes();

		/**
		 * @brief whether this adapter supports batch read operations.
		 */
		virtual bool supportsBatchGet() const = 0;

		/**
		 * @brief whether this adapter supports batch write operations.
		 */
		virtual bool supportsBatchSet() const = 0;

		/**
		 * @brief Construct a new Abs Storage Adapter object
		 *
		 * @param capacity the maximum number of buckets (not blocks) to store
		 * @param userBlockSize the size of the user's potion (payload) of the block in bytes.
		 * Has to be at least two AES blocks (32 bytes) and be a multiple of AES block (16 bytes).
		 *
		 * @param key AES key to use for encryption.
		 * Has to be KEYSIZE bytes (32), otherwise will be generated randomly.
		 *
		 * @param Z the number of blocks in a bucket.
		 * GET and SET will operate using Z.
		 *
		 * @param batchLimit the maximum number of requests (regardless of the individual request size) to put in a batch.
		 * If the number of GET or PUT requests exceeds this number, the sequence will be broken up into batches of this size.
		 * Set to 0 to disable batching (i.e. batch size is unlimited).
		 */
		AbsStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const number Z, const number batchLimit);
		virtual ~AbsStorageAdapter() = 0;

		protected:
		const number capacity;		// number of buckets
		const number blockSize;		// whole bucket size (Z times (user portion + ID) + IV)
		const number userBlockSize; // number of bytes in payload portion of block

		/**
		 * @brief actual routine that writes raw bytes to storage
		 *
		 * Does not care about IDs or encryption.
		 *
		 * @param location where to write bytes
		 * @param raw the bytes to write
		 */
		virtual void setInternal(const number location, const bytes &raw) = 0;

		/**
		 * @brief actual routine that retrieves raw bytes to storage
		 *
		 * @param location location from where to read bytes
		 * @param response the retrieved bytes
		 */
		virtual void getInternal(const number location, bytes &response) const = 0;

		/**
		 * @brief batch version of setInternal
		 *
		 * @param requests sequency of blocks to write (location, raw bytes)
		 */
		virtual void setInternal(const vector<pair<number, bytes>> &requests);

		/**
		 * @brief batch version of getInternal
		 *
		 * @param locations sequence (ordered) of locations to read from
		 * @param response this vector will be appended (back-inserted) with blocks of bytes in the order defined by locations
		 */
		virtual void getInternal(const vector<number> &locations, vector<bytes> &response) const;
	};

	/**
	 * @brief In-memory implementation of the storage adapter.
	 *
	 * Uses a RAM array as the underlying storage.
	 */
	class InMemoryStorageAdapter : public AbsStorageAdapter
	{
		private:
		uchar **const blocks;

		public:
		InMemoryStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const number Z, const number batchLimit = 0);

		~InMemoryStorageAdapter() final;

		protected:
		void setInternal(const number location, const bytes &raw) final;
		void getInternal(const number location, bytes &reponse) const final;

		bool supportsBatchGet() const final { return false; };
		bool supportsBatchSet() const final { return false; };

		friend class MockStorage;
	};

	/**
	 * @brief File system implementation of the storage adapter.
	 *
	 * Uses a binary file as the underlying storage.
	 */
	class FileSystemStorageAdapter : public AbsStorageAdapter
	{
		private:
		unique_ptr<fstream> file;

		public:
		/**
		 * @brief Construct a new File System Storage Adapter object
		 *
		 * It is possible to persist the data.
		 * If the file exists, instantiate with override = false, and the key equal to the one used before.
		 *
		 * @param capacity the max number of blocks
		 * @param userBlockSize the size of the user's portion of the block in bytes
		 * @param key the AES key to use (may be empty to generate new random one)
		 * @param filename the file path to use
		 * @param override if true, the file will be opened, otherwise it will be recreated
		 * @param Z the number of blocks in a bucket.
		 * GET and SET will operate using Z.
		 * @param batchLimit the maximum number of requests in a batch.
		 */
		FileSystemStorageAdapter(const number capacity, const number userBlockSize, const bytes key, const string filename, const bool override, const number Z, const number batchLimit = 0);
		~FileSystemStorageAdapter() final;

		protected:
		void setInternal(const number location, const bytes &raw) final;
		void getInternal(const number location, bytes &reponse) const final;

		bool supportsBatchGet() const final { return false; };
		bool supportsBatchSet() const final { return false; };
	};



}
