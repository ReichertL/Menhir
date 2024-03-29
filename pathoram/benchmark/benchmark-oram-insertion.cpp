#include "definitions.h"
#include "oram.hpp"
#include "utility.hpp"

#include <benchmark/benchmark.h>
#include <boost/format.hpp>
#include <fstream>

using namespace std;

namespace PathORAM
{
	class ORAMBenchmarkThis : public ::benchmark::Fixture
	{
		public:
		inline static number LOG_CAPACITY;
		inline static number Z;
		inline static number BLOCK_SIZE;
		inline static number BATCH_SIZE;

		inline static number CAPACITY;
		inline static number ELEMENTS;

		inline static const auto ITERATIONS = 1;//1 << 10;

		protected:
		//unique_ptr<ORAM> oram;
		shared_ptr<ORAM> oram;

		void Configure(number CAPACITY, number BLOCK_SIZE)
		{
			this->Z			   = 3;
			
			this->LOG_CAPACITY =  std::max((number) ceil(log((CAPACITY+1)/Z)/log(2)),2ull);
			
			cout<<"LOG_CAPACITY "<<this->LOG_CAPACITY<<endl;

			this->BLOCK_SIZE   = BLOCK_SIZE;
			this->BATCH_SIZE   = 1;
			this->CAPACITY	   = CAPACITY;//(1 << LOG_CAPACITY) * Z;
			cout<<"CAPACITY "<<this->CAPACITY<<endl;

			
			cout.flush();
			this->oram = make_shared<PathORAM::ORAM>(
					this->LOG_CAPACITY,
					this->BLOCK_SIZE,
					this->Z,
					make_shared<InMemoryStorageAdapter>((1 << this->LOG_CAPACITY), this->BLOCK_SIZE, bytes(), this->Z),
					make_shared<InMemoryPositionMapAdapter>(((1 << this->LOG_CAPACITY) * this->Z) + this->Z),
					make_shared<InMemoryStashAdapter>(3 * this->LOG_CAPACITY * this->Z),
					true,
					this->BATCH_SIZE);

		ulong id=(ulong) 0;
		auto data = fromText(to_string(id), BLOCK_SIZE);
		oram->put(id, data);

			/*this->oram = make_unique<ORAM>(
				LOG_CAPACITY,
				BLOCK_SIZE,
				Z);*/
		}
	};

	BENCHMARK_DEFINE_F(ORAMBenchmarkThis, Payload)
	(benchmark::State& state)
	{
		Configure(state.range(0), state.range(1));

		// put all
		for (number id = 1; id < CAPACITY; id++)
		{


			auto data = fromText(to_string(id), BLOCK_SIZE);
			oram->put(id, data);
		}

	
	}


	BENCHMARK_REGISTER_F(ORAMBenchmarkThis, Payload)
		// base case
		//->Args({10000, 64})
		->Args({100, 64})

		->Iterations(ORAMBenchmarkThis::ITERATIONS)
		->Unit(benchmark::kMicrosecond);
}

BENCHMARK_MAIN();
