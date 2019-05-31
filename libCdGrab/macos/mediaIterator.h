#pragma once

#include <string>

#include <IOKit/IOKitLib.h>


class MediaIterator
{
public:
	MediaIterator();

	MediaIterator(CFMutableDictionaryRef& match);

	~MediaIterator();

	io_object_t next();

private:
	io_iterator_t it;
};


std::string GetBSDPath(io_object_t& medium);
