# infyJSON
infyJSON is a JSON parsing library written using C++17 features. Why would we need again another JSON library? Well, because C++17 is cool.
## Usage

All you need is to add **Parser.h**, **Parser.cpp**, **Value.h**, **Value.cpp** in your project and compile it with c++17 flag.
```cpp
#include "Parser.h"
using namespace JSON;
...
std::optional<Value> json = JSON::parseFromFile("myFile.json");  
if (json && json->is<JsonArrayHObj>()) {
  for (const auto& val : json->getAs<JsonArrayHObj>().value()) {
  	if (val->is<JsonObjectHObj>()) {
  		auto& object = val->getAs<JsonObjectHObj>();
  		std::cout << object["name"]->getAs<JsonStringHObj>().value() << '\n';
  		std::cout << object["number"]->getAs<JsonNumberHObj>().value() << '\n';
  		std::cout << "Is cool? " << object["is_cool"]->getAs<JsonBooleanHObj>().value() << '\n';
  	}
  }
}
...
std::string s = getResponseFromServer();
auto json2 = JSON::parseFromString(s); // supports const char* as well
...
using namespace JSON::literals;
auto json3 = "[2.55, "laurel", "yanny", true, null]"_json;
auto json4 = R"(
{
  "first" : 42,
  "second" : ["myCoolStringInCoolArray", -93.33, false]
}
)"_json;
...
```
You might be wondering what these weird HObj types are. So, they are just wrappers around dynamically allocated objects and they behave 100% like ordinary objects on stack. That's a solution to overcome a problem of passing incomplete types in std::unordered_map and preserve simple copy/move operations when it's needed.

You can access member functions of HObj underlying object through '->' or just dereference/call value() method to get lvalue reference to object itself. **Remember**: HObj behaves like object on stack, so when you pass it as copy to function, underlying object will be copied, which can be pretty expensive - so don't forget to use references. HObjects are deleted at scope exit.
Operator[] can be used on HObj without dereferencing (useful for JSON object (std::unordered_map) and array (std::vector)).

If you're sure enough that your input is proper JSON without sudden EOFs, you can remove EOF checks in Parser.cpp at 11 and 12 lines.

## Debug

If you're curious why JSON::parseFromFile() returns nullopt, you can define macro INFYJSON_DEBUG. This will reduce parsing speed a bit, but function call JSON::getDebugInfo() will return a number and contents of last parsed line. When macro isn't defined, function always returns "Last parsed line(1)".

## Support
Visual Studio 15.8.0 and higher.

GCC 8.2 (latest at the moment) still doesn't support floating-point version of std::from_chars :(
