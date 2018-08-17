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
}

