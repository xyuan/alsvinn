#pragma once

#include <sstream>
#include <exception>

/// 
/// Throws an exception with the given message
///
#define THROW(message) {\
	std::stringstream ssForException; \
	ssForException << message; \
	ssForException << std::endl << "At " << __FILE__<<":" << __LINE__ << std::endl;\
	throw std::runtime_error(ssForException.str());\
}
	