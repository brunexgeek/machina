#ifndef MACHINA_MEMORY_H
#define MACHINA_MEMORY_H


#include <sys/types.h>
#include <sys/Screen.hh>


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

		void print(
			TextScreen &screen );

	private:
		static Memory instance;

		Memory();

		void initialize();

};


} //machina


#endif // MACHINA_MEMORY_H