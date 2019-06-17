//
//  infyJSON lib
//

#include "Value.h"

namespace JSON {

	namespace {
		const JValue dummy;
	}

	bool Value::operator==(const Value& right) const {
		if (this == &right) return true;
 		return _data == right._data;
	}

	bool Value::operator!=(const Value& right) const {
		return _data != right._data;
	}

	bool Value::hasKey(const std::string& right) const
	{
		return is<JObject>() && getAs<JObject>()->count(right) != 0;
	}

	JValue& Value::operator[](const size_t right) {
		if (!is<JArray>()) {
			_data = JArray{};
		}
		auto& array = getAs<JArray>();
		while (array->size() <= right) {
			array->emplace_back();
		}
		return array[right];
    }

	JValue& Value::operator[](const std::string& right) {
		if (!is<JObject>()) {
			_data = JObject{};
		}
		auto& map = getAs<JObject>();
		return map->try_emplace(right).first->second;
    }

	const JValue& Value::operator[](const std::string& right) const
	{
		if (is<JObject>()) {
			auto& map = getAs<JObject>();
			if (auto it = map->find(right); it != map->end()) {
				return it->second;
			}
		}
		return dummy;
	}

	const JValue& Value::operator[](const size_t right) const
	{
		if (is<JArray>()) {
			auto& arr = getAs<JArray>();
			if (right < arr->size()) {
				return arr[right];
			}
		}
		return dummy;
	}

	void Value::write(std::string& result)
	{
		std::visit([&result](auto&& arg) {
			using decayed_t = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<decayed_t, JEmpty>) {
				result += "null";
			}
			else if constexpr (std::is_same_v<decayed_t, JString>) {
				result += '\"' + arg.value() + '\"';
			}
			else if constexpr (std::is_same_v<decayed_t, JDouble> || std::is_same_v<decayed_t, JInt>) {
				result += std::to_string(arg.value());
			}
			else if constexpr (std::is_same_v<decayed_t, JBool>) {
				result += arg.value() ? "true" : "false";
			}
			else if constexpr (std::is_same_v<decayed_t, JObject>) {
				result += '{';
				for (auto& [key, value] : arg) {
					result += '\"' + key + "\":";
					value->write(result);
					result += ',';
				}
				result.back() = '}';
			}
			else if constexpr (std::is_same_v<decayed_t, JArray>) {
				result += '[';
				for (auto& value : arg) {
					value->write(result);
					result += ',';
				}
				result.back() = ']';
			}
			else {
				throw std::bad_variant_access();
			}
		}, _data);
	}

}