#ifndef MACHINA_MEMORY_H
#define MACHINA_MEMORY_H


#include <sys/types.h>


namespace machina {


class Memory
{
	public:
		~Memory();

		static Memory &getInstance();

		void *allocate(
			size_t size );

		void free(
			void * address );

	private:
		static Memory instance;

		Memory();

		void initialize();

};


} //machina


#endif // MACHINA_MEMORY_H