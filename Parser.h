//
//  infyJSON lib
//
#pragma once

#include "Value.h"
#include <optional>
#include <string_view>

namespace JSON {

	std::optional<Value> parseFromFile(std::string_view path);
	std::string getDebugInfo();

	namespace literals {
		std::optional<Value> operator"" _json(const char * json, std::size_t size);
	}
}

