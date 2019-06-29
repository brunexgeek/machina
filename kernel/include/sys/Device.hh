#ifndef MACHINA_DEVICE_HH
#define MACHINA_DEVICE_HH


#include <sys/types.h>

namespace machina {


class Device
{
	public:
		virtual const char16_t *getName() const = 0;

		virtual const char16_t *getFileName() const = 0;
};


}

#endif // MACHINA_DEVICE_HH