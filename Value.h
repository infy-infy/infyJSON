//
//  infyJSON lib
//
#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace JSON {

	namespace _helpers {
		
		template<typename T>
		class HeapObject : private std::unique_ptr<T>
		{
		public:
			constexpr HeapObject() noexcept : std::unique_ptr<T>{ std::make_unique<T>() } { };
			explicit HeapObject(T* p) noexcept : std::unique_ptr<T>{ p } { };
			HeapObject(std::unique_ptr<T>&& ptr) noexcept : std::unique_ptr<T>{ std::move(ptr) } { };
			
			template<typename U>
			HeapObject(U&& arg) noexcept : std::unique_ptr<T>{ 
				[](auto&& arg) -> decltype(auto) {
					if constexpr (std::is_same_v<HeapObject, std::decay_t<U>>) {
						if constexpr (std::is_lvalue_reference_v<U>) {						//standart copy constructor
							return std::make_unique<T>(*arg);
						} else {															//standart move constructor
							return arg.release();
						}
					} else {																//constructor for everything else
						return std::make_unique<T>(std::forward<U>(arg));
					}
				}(std::forward<U>(arg))	
			} {}

			template<typename... Types>
			HeapObject(Types&&... args) noexcept : std::unique_ptr<T>{
			[](auto&&... args) -> decltype(auto) {
					return std::make_unique<T>(std::forward<Types>(args)...);
				}(std::forward<Types>(args)...)
			} {}

			constexpr HeapObject(std::nullptr_t) noexcept : std::unique_ptr<T>{ nullptr } { };

			~HeapObject() = default;

			template<typename U>
			HeapObject& operator=(U&& arg) noexcept {
				if constexpr (std::is_same_v<HeapObject, std::decay_t<U>>) {
					if constexpr (std::is_lvalue_reference_v<U>) {						//standart copy assigment
						std::unique_ptr<T>::operator=(std::make_unique<T>(*arg));
					} else {															//standart move assigment
						std::unique_ptr<T>::reset(arg.release());
					}
				} else {																//assigment for everything else
					*std::unique_ptr<T>::get() = std::forward<U>(arg);
				}
				return *this;
			}

			HeapObject& operator=(std::nullptr_t) noexcept {
				std::unique_ptr<T>::reset();
				return *this;
			}

			typename std::add_lvalue_reference_t<T> operator*() const {
				return std::unique_ptr<T>::operator*();
			}

			T* operator->() const noexcept {
				return std::unique_ptr<T>::operator->();
			}

			template<typename U>
			decltype(auto) operator[](const U& arg) {
				return std::unique_ptr<T>::get()->operator[](arg);
			}

			template<typename U>
			decltype(auto) operator[](const U& arg) const {
				return std::unique_ptr<T>::get()->operator[](arg);
			}

			bool operator==(const HeapObject& right) const {
				return *std::unique_ptr<T>::get() == *right.get();
			}

			bool operator!=(const HeapObject& right) const {
				return *std::unique_ptr<T>::get() != *right.get();
			}
		};
	}
	

	class Value;

	using ValueHObj = _helpers::HeapObject<Value>;
	using JsonStringHObj = _helpers::HeapObject<std::string>;
	using JsonObjectHObj = _helpers::HeapObject<std::unordered_map<std::string, ValueHObj>>;
	using JsonArrayHObj = _helpers::HeapObject<std::vector<ValueHObj>>;
	using JsonNumberHObj = _helpers::HeapObject<double>;
	using JsonBooleanHObj = _helpers::HeapObject<bool>;
	using JsonEmpty = std::nullptr_t;

    class Value {	
    private:
		using Data = std::variant<JsonEmpty, JsonObjectHObj, JsonArrayHObj, JsonStringHObj, JsonNumberHObj, JsonBooleanHObj>;
		Data _data;
		
	public:

		template<typename T>
		inline bool is() const { return std::holds_alternative<T>(_data); }

		Value() = default;

		template<typename T>
		Value(T&& right);

		~Value() = default;
        
		template<typename T>
		Value& operator=(T&& right);
        
		bool operator==(const Value& right) const;
		bool operator!=(const Value& right) const;

		bool hasKey(const std::string& right) const;
        ValueHObj& operator[](const std::string& right);
		ValueHObj& operator[](const size_t right);
       
		template<typename T>
		inline T& getAs();

		template<typename T>
		inline const T& getAs() const;

		template<typename T, typename... Types>
		T& emplace(Types&&... args);

    };

	

	namespace _helpers {
		template<typename T, typename U> //https://stackoverflow.com/questions/31171682/type-trait-for-copying-cv-reference-qualifiers
		struct copy_cv_reference
		{
		private:
			using R = std::remove_reference_t<T>;
			using U1 = std::conditional_t<std::is_const<R>::value, std::add_const_t<U>, U>;
			using U2 = std::conditional_t<std::is_volatile<R>::value, std::add_volatile_t<U1>, U1>;
			using U3 = std::conditional_t<std::is_lvalue_reference<T>::value, std::add_lvalue_reference_t<U2>, U2>;
			using U4 = std::conditional_t<std::is_rvalue_reference<T>::value, std::add_rvalue_reference_t<U3>, U3>;
		public:
			using type = U4;
		};

		template<typename T, typename U>
		using copy_cv_reference_t = typename copy_cv_reference<T, U>::type;
	}

	template<typename T>
	Value::Value(T&& right) : _data{
		[](auto&& arg) -> decltype(auto) {
			if constexpr (std::is_same_v<std::decay_t<T>, Value>) {												//handling default
				return std::forward<_helpers::copy_cv_reference_t<T, Value::Data>>(arg._data);
			} else if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<std::decay_t<T>, bool>) {			//all arithmetic to double
				return JsonNumberHObj{ static_cast<double>(std::forward<T>(arg)) };
			} else if constexpr (std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<std::decay_t<T>, const char*>) { //all char* to string
				return JsonStringHObj(arg);
			} else {																							//everything else
				return _helpers::HeapObject<std::decay_t<T>>{ std::forward<T>(arg) };
			}
		}(std::forward<T>(right))
	} {}

	template<typename T>
	Value& Value::operator=(T&& right) {
		if constexpr (std::is_same_v<std::decay_t<T>, Value>) {
			if (this != &right) {
				_data = std::forward<_helpers::copy_cv_reference_t<T, Value::Data>>(right._data);
			}
		} else if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<std::decay_t<T>, bool>) {
			_data = JsonNumberHObj{ static_cast<double>(std::forward<T>(right)) };
		} else if constexpr (std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<std::decay_t<T>, const char*>) {
			_data = JsonStringHObj{ std::string(right) };
		} else {
			_data = _helpers::HeapObject<std::decay_t<T>>{ std::forward<T>(right) };
		}
		return *this;
	}

	template<typename T>
	inline T& Value::getAs() {
		return std::get<std::decay_t<T>>(_data);
	}
	
	template<typename T>
	inline const T& Value::getAs() const {
		return std::get<std::decay_t<T>>(_data);
	}

	template<typename T, typename... Types>
	T& Value::emplace(Types&&... args) {
		return _data.emplace<T>(std::forward<Types>(args)...);
	}
}


	