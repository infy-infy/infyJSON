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

		template<unsigned int N, typename T, typename ...Types>
		struct get_type_at {
			using type = typename get_type_at<N - 1, Types...>::type;
		};

		template<typename T, typename ...Types>
		struct get_type_at<0, T, Types...> {
			using type = T;
		};

		template<unsigned int N, typename ...Types>
		using get_type_at_t = typename get_type_at<N, Types...>::type;

		template<typename T>
		struct remove_cv_ref {
			using type = std::remove_reference_t<std::remove_cv_t<T>>;
		};

		template<typename T>
		using remove_cv_ref_t = typename remove_cv_ref<T>::type;

		template<typename ClassType, typename ...ArgTypes>
		using exclude_class_default_t = typename std::enable_if_t<
			(sizeof...(ArgTypes) > 1) ||
			(sizeof...(ArgTypes) == 1 &&
			!std::is_same_v< remove_cv_ref_t<ClassType>, remove_cv_ref_t<get_type_at_t<0, ArgTypes...>> > &&
			!std::is_convertible_v<remove_cv_ref_t<get_type_at_t<0, ArgTypes...>>, remove_cv_ref_t<ClassType>>)
		>;

		template <typename T, typename = void>
		struct is_iterable : std::false_type {};
		template <typename T>
		struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()),
			decltype(std::declval<T>().end())>>
			: std::true_type {};

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

		template<typename T>
		class HeapObject final : private std::unique_ptr<T>
		{
		public:
			HeapObject() noexcept : std::unique_ptr<T>{ std::make_unique<T>() } { };
			explicit HeapObject(T* p) noexcept : std::unique_ptr<T>{ p } { };
			HeapObject(std::unique_ptr<T>&& ptr) noexcept : std::unique_ptr<T>{ std::move(ptr) } { };
			
			HeapObject(const HeapObject& arg) noexcept : std::unique_ptr<T>{ std::make_unique<T>(*arg) } {}
			
			HeapObject(HeapObject&& arg) noexcept : std::unique_ptr<T>{ arg.release() } {}

			template<typename ...Types, typename = exclude_class_default_t<HeapObject, Types...>>
			explicit HeapObject(Types&&... args) : std::unique_ptr<T>{ std::make_unique<T>(std::forward<Types>(args)...) } {}

			~HeapObject() = default;

			template<typename = std::enable_if_t<is_iterable<T>::value>>
			auto begin() {
				return std::unique_ptr<T>::get()->begin();
			}

			template<typename = std::enable_if_t<is_iterable<T>::value>>
			auto end() {
				return std::unique_ptr<T>::get()->end();
			}

			HeapObject& operator=(const HeapObject& arg) {
				std::unique_ptr<T>::operator=(std::make_unique<T>(*arg));
				return *this;
			}

			HeapObject& operator=(HeapObject&& arg) {
				std::unique_ptr<T>::reset(arg.release());
				return *this;
			}

			template<typename U, typename = exclude_class_default_t<HeapObject, U>>
			HeapObject& operator=(U&& arg) noexcept {
				*std::unique_ptr<T>::get() = std::forward<U>(arg);
				return *this;
			}

			typename std::add_lvalue_reference_t<T> operator*() const {
				return std::unique_ptr<T>::operator*();
			}

			typename std::add_lvalue_reference_t<T> value() const {
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

	using JValue = _helpers::HeapObject<Value>;
	using JString = _helpers::HeapObject<std::string>;
	using JObject = _helpers::HeapObject<std::unordered_map<std::string, JValue>>;
	using JArray = _helpers::HeapObject<std::vector<JValue>>;
	struct JNumber {};
	using JBool= _helpers::HeapObject<bool>;
	using JEmpty = std::nullptr_t;

    class Value {	
    private:
		using JInt = _helpers::HeapObject<int64_t>;
		using JDouble = _helpers::HeapObject<double>;
		using Data = std::variant<JEmpty, JObject, JArray, JString, JInt, JDouble, JBool>;
		Data _data;
		
	public:

		template<typename T>
		inline bool is() const { return std::holds_alternative<T>(_data); }

		Value() = default;

		Value(const Value& arg) : _data{ arg._data } {};

		Value(Value&& arg) : _data{ std::move(arg._data) } {};

		template<typename U, typename = _helpers::exclude_class_default_t<Value, U>>
		explicit Value(U&& arg);

		~Value() = default;
        
		template<typename T>
		Value& operator=(T&& right);
        
		bool operator==(const Value& right) const;
		bool operator!=(const Value& right) const;

		bool hasKey(const std::string& right) const;
		JValue& operator[](const std::string& right);
		JValue& operator[](const size_t right);

		const JValue& operator[](const std::string& right) const;
		const JValue& operator[](const size_t right) const;
       
		template<typename T>
		inline decltype(auto) getAs();

		template<typename T>
		inline decltype(auto) getAs() const;

		template<typename T>
		inline T getByKey(const std::string& key, const T& def = T()) {
			if (hasKey(key)) {
				return getAs<JObject>()[key]->getAs<T>();
			}
			return def;
		}

		template<typename T, typename... Types>
		auto& emplace(Types&&... args);

		void write(std::string& result);
    };

	template<typename U, typename>
	Value::Value(U&& arg) : _data{
		[](auto&& arg) -> decltype(auto) {
			using decayed_u = std::decay_t<U>;
			if constexpr (std::is_arithmetic_v<decayed_u> && !std::is_same_v<decayed_u, bool>) {
				if constexpr (std::is_integral_v<decayed_u>) {
					return JInt{ static_cast<int64_t>(std::forward<U>(arg)) };
				} else {
					return JDouble{ static_cast<double>(std::forward<U>(arg)) };
				}		
			} else if constexpr (std::is_same_v<decayed_u, char*> || std::is_same_v<decayed_u, const char*>) {
				//all char* to string
				return JString(arg);
			} else {
				//everything else
				return _helpers::HeapObject<decayed_u>{ std::forward<U>(arg) };
			}
		}(std::forward<U>(arg))
	} {}

	template<typename T>
	Value& Value::operator=(T&& right) {
		using decayed_u = std::decay_t<T>;
		if constexpr (std::is_same_v<std::decay_t<decayed_u>, Value>) {
			if (this != &right) {
				_data = std::forward<_helpers::copy_cv_reference_t<T, Value::Data>>(right._data);
			}
		} else if constexpr (std::is_arithmetic_v<decayed_u> && !std::is_same_v<decayed_u, bool>) {
			if constexpr (std::is_integral_v<decayed_u>) {
				_data = JInt{ static_cast<int64_t>(std::forward<U>(arg)) };
			}
			else {
				_data = JDouble{ static_cast<double>(std::forward<U>(arg)) };
			}
		} else if constexpr (std::is_same_v<decayed_u, char*> || std::is_same_v<decayed_u, const char*>) {
			_data = JString{ std::string(right) };
		} else {
			_data = _helpers::HeapObject<decayed_u>{ std::forward<T>(right) };
		}
		return *this;
	}

	template<typename T>
	inline decltype(auto) Value::getAs() {
		using decayed_t = std::decay_t<T>;
		if constexpr ((std::is_same_v<decayed_t, JNumber> || std::is_arithmetic_v<decayed_t>) && !std::is_same_v<decayed_t, bool>) {		
			using visiter_return_t = std::conditional_t<std::is_same_v<decayed_t, JNumber>, double, decayed_t>;
			return std::visit([](const auto& arg) -> visiter_return_t {
				using visiter_type = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<visiter_type, JDouble> || std::is_same_v<visiter_type, JInt>) {
					return static_cast<visiter_return_t>(arg.value());
				} else {
					throw std::bad_variant_access();
				}
			}, _data);
		} else if constexpr (std::is_same_v<std::string, decayed_t>) {
			return std::get<JString>(_data).value();
		} else if constexpr (std::is_same_v<decayed_t, bool>) {
			return std::get<JBool>(_data).value();
		} else {
			return std::get<decayed_t>(_data);
		}	
	}

	template<typename T>
	inline decltype(auto) Value::getAs() const {
		using decayed_t = std::decay_t<T>;
		if constexpr ((std::is_same_v<decayed_t, JNumber> || std::is_arithmetic_v<decayed_t>) && !std::is_same_v<decayed_t, bool>) {
			using visiter_return_t = std::conditional_t<std::is_same_v<decayed_t, JNumber>, double, decayed_t>;
			return std::visit([](const auto& arg) -> visiter_return_t {
				using visiter_type = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<visiter_type, JDouble> || std::is_same_v<visiter_type, JInt>) {
					return static_cast<visiter_return_t>(arg.value());
				}
				else {
					throw std::bad_variant_access();
				}
			}, _data);
		}
		else if constexpr (std::is_same_v<std::string, decayed_t>) {
			return std::get<JString>(_data).value();
		}
		else if constexpr (std::is_same_v<decayed_t, bool>) {
			return std::get<JBool>(_data).value();
		}
		else {
			return std::get<decayed_t>(_data);
		}
	}

	template<typename T, typename... Types>
	auto& Value::emplace(Types&&... args) {
		if constexpr (std::is_same_v<std::decay_t<T>, JNumber>) {
			static_assert(sizeof...(Types) == 1);
			if constexpr (std::conjunction_v<std::is_integral<std::decay_t<Types>>...>) {
				return _data.emplace<JInt>(std::forward<Types>(args)...);
			} else {
				return _data.emplace<JDouble>(std::forward<Types>(args)...);
			}
		} else {
			return _data.emplace<T>(std::forward<Types>(args)...);
		}	
	}
}


	