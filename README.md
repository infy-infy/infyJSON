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
```
You might be wondering what these weird HObj types are. So, they are just wrappers around dynamically allocated objects and they behaves 100% like ordinary objects on stack. That's a solution to overcome a problem of passing incomplete types in std::unordered_map and preserve simple copy/move operations when it needed.

You can access member functions of HOnj underlying object through '->' or just dereference/call value() method to get lvalue reference to object itself. **Remember**: HObj behaves like object on stack, so when you pass it as copy to function, underlying object will be copied, which can be pretty expensive - so don't forget to use references. HObjects are deleted at scope exit.
Operator[] can be used on HObj without dereferencing (useful for JSON object (std::unordered_map) and array (std::vector)).

## Support
Visual Studio 15.8.0+
GCC 8.2 (latest at the moment) still doesn't support floating-point version of std::from_chars :(