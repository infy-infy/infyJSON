//
//  infyJSON lib
//

#include "Parser.h"
#include <fstream>
#include <charconv>
#include <algorithm>
#include <cctype>

#define GET_NEXT(var) if (_parser::isEOF()) return 0; var = _parser::get()
#define GET_NEXT_NON_SPACE(var) if (_parser::isEOF()) return 0; var = getFirstNonSpaceChar()

namespace JSON {

	namespace {
		constexpr char tab = '\t';
		constexpr char space = ' ';
	}

	namespace _parser {

		enum BasicValue {
			STRING,
			NUMBER
		};

		std::unique_ptr<char[]> _buf;
		const char* _pos;
		const char* _last;
		bool _eof;
		std::string _lastReadLine;
		size_t _lineNumber;

		void init(std::string_view path) {
			_lineNumber = 1;
#ifdef INFYJSON_DEBUG
			_lastReadLine.clear();
#endif
			std::ifstream input(std::string(path), std::ios::in | std::ios::binary | std::ios::ate);
			unsigned int length = static_cast<unsigned int>(input.tellg());
			if (length != 0) {
				_buf = std::unique_ptr<char[]>(new char[length]);
				_pos = _buf.get();
				_last = _buf.get() + length;
				input.seekg(0, std::ios::beg);
				input.read(_buf.get(), length);
				_eof = false;
			} else {
				_eof = true;
			}
			input.close();
		}

		char get() {
			auto c = *_pos;
			++_pos;
#ifdef INFYJSON_DEBUG
			_lastReadLine.push_back(c);
			if (c == '\n') {
				_lastReadLine.clear();
				++_lineNumber;
			}
#endif
			_eof = (_pos == _last);
			return c;
		}

		bool isEOF() {
			return _eof;
		}

		class LastQuoteFinder {
			bool _escapedStarted{ false };
		public:
			bool check(char c) {
				if (c == '\\') {
					_escapedStarted = !_escapedStarted;
				} else if (c == '\"' && !_escapedStarted) {
					return true;
				} else {
					_escapedStarted = false;
				}
				return false;
			}
		};

		std::pair<const char*, const char*> getBasicValueBorders(BasicValue val) {
			switch (val)
			{
			case BasicValue::STRING:
			{
				const char* v1 = _pos;
				bool isBadChar{ false };
				LastQuoteFinder finder;
				auto search = [&](const char c) {
					isBadChar = isBadChar || (c >= '\x00' && c <= '\x1F');
					return finder.check(c);
				};
				const char* v2 = std::find_if(v1, _last, search);

				if (v2 + 1 == _last || v2 == _last) { // so, json which contains only string without last quote won't crash programm
					_eof = true;
				}
				_pos = v2 + 1;
#ifdef INFYJSON_DEBUG
				_lastReadLine.append(v1, v2);
#endif
				return isBadChar ? std::pair<const char*, const char*>(nullptr, nullptr) : std::pair(v1, v2);
			}
			case BasicValue::NUMBER:
			{
				const char* v1 = _pos;
				const char* v2 = std::find_if_not(v1, _last, [](const char c) {
					return isdigit(c) || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-';
				});
				if (v2 == _last) {
					_eof = true;
				}
				_pos = v2;
				--v1; // to include first digit or '-'
#ifdef INFYJSON_DEBUG
				_lastReadLine.append(v1, v2);
#endif
				return std::pair(v1, v2);
			}
			default:
				return std::pair<const char*, const char*>();
			}
		}

		std::string getLastReadLine() {
			using namespace std::string_literals;
			return "Last parsed line("s + std::to_string(_lineNumber) + "): "s + _lastReadLine;
		}

	}

	namespace {

		bool isSpaceChar(char c) 
		{ 
			return (c == tab || c == space); 
		}

		bool isNewLineChar(char c) 
		{ 
			return (c == '\n' || c == '\r'); 
		}

		char getFirstNonSpaceChar() {
			char c = _parser::get();
			while ((isSpaceChar(c) || isNewLineChar(c)) && !_parser::isEOF())
			{
				c = _parser::get();
			}
			return c;
		}

		//legacy hex to int
		/*int _hexToInt(char c) {
			switch (c)
			{
			case '0':
				return 0;
			case '1':
				return 1;
			case '2':
				return 2;
			case '3':
				return 3;
			case '4':
				return 4;
			case '5':
				return 5;
			case '6':
				return 6;
			case '7':
				return 7;
			case '8':
				return 8;
			case '9':
				return 9;
			case 'a':
			case 'A':
				return 10;
			case 'b':
			case 'B':
				return 11;
			case 'c':
			case 'C':
				return 12;
			case 'd':
			case 'D':
				return 13;
			case 'e':
			case 'E':
				return 14;
			case 'f':
			case 'F':
				return 15;
			default:
				return -1;
			}
		}*/


		bool isWord(char c, Value& o)
		{
			std::string s;
			if (c == 'n' || c == 't') {
				s += c;
				for (int i = 0; i < 3; i++) {
					GET_NEXT(c); //safe since nothing else starts with 'n' or 't'
					s += c;
				}
				if (s == "null") {
					return true;
				}
				if (s == "true") {
					o = true;
					return true;
				}
				return false;
			} else if (c == 'f') {
				s += c;
				for (int i = 0; i < 4; i++) {
					GET_NEXT(c);
					s += c;
				}
				if (s == "false") {
					o = false;
					return true;
				}
				return false;
			}
			return false;
		}

		bool isNumber(char c, Value& o)
		{	
			if (std::isdigit(c) || c == '-') {
				auto range = _parser::getBasicValueBorders(_parser::NUMBER);
				auto& number = *o.emplace<JNumber>();
				auto res = std::from_chars(range.first, range.second, number);
				return res.ptr == range.second;
			}
			return false;
		}

		//legacy escaped
		/*bool isEscaped(char c, std::string& ws)
		{
			switch (c)
			{
			case '\"':
				ws += '\"';
				return true;
			case '\\':
				ws += '\\';
				return true;
			case '/':
				ws += '/';
				return true;
			case 'b':
				ws += '\b';
				return true;
			case 'f':
				ws += '\f';
				return true;
			case 'n':
				ws += '\n';
				return true;
			case 'r':
				ws += '\r';
				return true;
			case 't':
				ws += '\t';
				return true;
			case 'u': {
				//int escapedChar[2];
				//escapedChar[0] = escapedChar[1] = 0;
				for (int i = 0; i < 4; i++)
				{
					int hexDigit = _hexToInt(getChar());
					if (hexDigit != -1) {
						escapedChar[1 - i / 2] += hexDigit * static_cast<int>(pow(16, 1 - i % 2));
					}
					else {
						return false;
					}
					GET_NEXT(c);
					ws += c;
				}
				//ws += static_cast<char>(escapedChar[0]);
				//ws += static_cast<char>(escapedChar[1]);
				return true;
			}
			default:
				return false;
			}
		}*/

		bool isString(char c, Value& o)
		{
			if (c != '\"') return false;
			auto range = _parser::getBasicValueBorders(_parser::STRING);
			if (range.first) {
				std::string str(range.first, range.second);
				o.emplace<JString>(std::move(str));
				return true;
			}
			return false;	
		}

		int readKeyValue(char c, JObject& map);

		int readMap(Value& o)
		{
			auto& map = o.emplace<JObject>();
			GET_NEXT_NON_SPACE(char c);

			if (c == '}') { //empty map
				return 1;
			}

			while (true)
			{
				int res = readKeyValue(c, map);
				if (res != 1) return res;

				GET_NEXT_NON_SPACE(c); // считываем запятую
				if (c == '}') {
					return 1;
				} else if (c != ',') {
					return 0;
				}
				GET_NEXT_NON_SPACE(c);
			}
		}

		int readArray(Value& o)
		{
			auto& arr = o.emplace<JArray>();

			GET_NEXT_NON_SPACE(char c);

			if (c == ']') {
				return 1;
			}

			while (true)
			{
				int codeRes = 0;
				auto& temp = *arr->emplace_back();
				switch (c)
				{
				case '{':
					codeRes = readMap(temp);
					break;
				case '[':
					codeRes = readArray(temp);
					break;
				default:
					if (isString(c, temp) || isWord(c, temp) || isNumber(c, temp)) codeRes = 1;
					break;
				}
				if (codeRes != 1) return 0;

				GET_NEXT_NON_SPACE(c); // считываем запятую
				if (c == ']') {
					arr->shrink_to_fit();
					return 1;
				} else if (c != ',') {
					return 0;
				}
				GET_NEXT_NON_SPACE(c);
			}
		}

		int readKeyValue(char c, JObject& map)
		{
			Value key, o;

			if (!isString(c, key)) return 0;

			GET_NEXT_NON_SPACE(c);
			if (c != ':') return 0;


			GET_NEXT_NON_SPACE(c); 
			int res = 0;
			if (c == '{') {
				res = readMap(o);	
			} else if (c == '[') {
				res = readArray(o);
			} else if (isString(c, o) || isWord(c, o) || isNumber(c, o)) {
				res = 1;
			}

			map->try_emplace(std::move(*key.getAs<JString>()), std::move(o));
			return res;
		}
	}

	std::pair<JSON::Value, int> parse() {
		std::pair<JSON::Value, int> p;
		char c = getFirstNonSpaceChar();
		int code = 0;
		switch (c)
		{
		case '{':
			code = readMap(p.first);
			break;
		case '[':
			code = readArray(p.first);
			break;
		default:
			if (isString(c, p.first) || isWord(c, p.first) || isNumber(c, p.first)) code = 1;
			break;
		}
		p.second = code;
		return p;
	}

	std::optional<Value> parseFromFile(std::string_view path) {

		_parser::init(path);

		if (_parser::isEOF()) { //empty file case
			return std::nullopt;
		}

		auto [val, code] = parse();

		_parser::_buf.reset();
		if (code == 1) return std::optional{std::move(val)};
		return std::nullopt;
	}

	std::optional<Value> parseFromString(std::string_view jsonString) {
		if (jsonString.size() > 0) {
			_parser::_eof = false;
			_parser::_pos = jsonString.data();
			_parser::_last = jsonString.data() + jsonString.size();
			auto[val, code] = parse();
			if (code == 1) return std::optional{ std::move(val) };
		}
		return std::nullopt;
	}

	std::string getDebugInfo() {
		return _parser::getLastReadLine();
	}

	namespace literals {
		std::optional<Value> operator"" _json(const char * json, std::size_t size) {
			if (size > 0) {
				_parser::_eof = false;
				_parser::_pos = json;
				_parser::_last = json + size;
				auto[val, code] = parse();
				if (code == 1) return std::optional{std::move(val)};
			}
			return std::nullopt;
		}
	}
}