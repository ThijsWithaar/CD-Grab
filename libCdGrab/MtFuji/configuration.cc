#include "configuration.h"

template<>
FeatureHeader parse(const void* buf)
{
	FeatureHeader r{0};
	return r;
}
