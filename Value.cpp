//
//  infyJSON lib
//

#include "Value.h"

namespace JSON {


	bool Value::operator==(const Value& right) const {
		if (this == &right) return true;
 		return _data == right._data;
	}

	bool Value::operator!=(const Value& right) const {
		return _data != right._data;
	}

	bool Value::hasKey(const std::string& right) const
	{
		return is<JsonObjectHObj>() && getAs<JsonObjectHObj>()->count(right) != 0;
	}

	ValueHObj& Value::operator[](const size_t right) {
		if (!is<JsonArrayHObj>()) {
			_data = JsonArrayHObj{};
		}
		auto& array = getAs<JsonArrayHObj>();
		while (array->size() <= right) {
			array->emplace_back();
		}
		return array[right];
    }

	ValueHObj& Value::operator[](const std::string& right) {
		if (!is<JsonObjectHObj>()) {
			_data = JsonObjectHObj{};
		}
		auto& map = getAs<JsonObjectHObj>();
		return map->try_emplace(right).first->second;
    }

}