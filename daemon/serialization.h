#pragma once

#include <tuple>
#include <type_traits>

#include <jsonrpc-lean/value.h>
#include <rapidjson/document.h>

// Helper classes to serialize/deserialize C++ structs to/from JSON

namespace serialization {

	#define is_serializable_struct(type) (std::tuple_size<decltype(type::_serializable_fields)>::value >= 0)

	template<typename TO, typename FROM> static inline void json_convert(TO& dst, const FROM& src);
	template<typename TO, typename FROM> static inline TO json_cast(const FROM& src);

	/*
	template<size_t iteration, size_t count, template<size_t> class body, typename... Args>
	static inline std::enable_if_t<(iteration < count)> iterate(Args&&... args)
	{
		body::call(std::forward<Args>(args)...);
		iterate<iteration + 1, count, body, Args...>(std::forward<Args>(args)...);
	}
	template<size_t iteration, size_t count, template<size_t> class body, typename... Args>
	static inline std::enable_if_t<(iteration == count)> iterate(Args&&... args) {}

	template<typename TO, typename SERIALIZABLE = decltype(TO::_serialized_fields)>
	static inline void json_convert(TO& dst, const jsonrpc::Value::Struct& src)
	{
		template<size_t i> struct body {
			static inline void call(TO& dst, const jsonrpc::Value::Struct& src) {
				constexpr auto prop = std::get<i>(TO::_serialized_fields);
				json_convert(prop.get(dst), src[prop.name]);
			}
		};
		iterate<0, std::tuple_size<SERIALIZABLE>::value, body>(dst, src);
	}

	template<typename TO, typename SERIALIZABLE = decltype(TO::_serialized_fields)>
	static inline void json_convert(TO& dst, const jsonrpc::Value& src)
	{
		json_convert(dst, src.AsStruct());
	}

	template<typename TO, typename SERIALIZABLE = decltype(TO::_serialized_fields)>
	static inline void json_convert(TO& dst, const rapidjson::Value& src)
	{
		if (src.IsObject())
		{
			template<size_t i> struct body {
				static inline void call(TO& dst, const rapidjson::Value& src) {
					constexpr auto prop = std::get<i>(TO::_serialized_fields);
					json_convert(prop.get(dst), src[prop.name]);
				}
			};
			iterate<0, std::tuple_size<SERIALIZABLE>::value, body>(dst, src);
		}
	}
	*/

	#define THIS_CLASS std::remove_cv_t< std::remove_pointer_t< std::remove_reference_t< decltype(this) > > >

	template<class C, typename T>
	struct field
	{
		typedef T type;
		typedef C class_type;
		T C::* const member;
		const char* const name;

		constexpr field(T C::* member, const char* name) : member(member), name(name) {}

		T& get(C& obj) const { return obj.*member; }
		const T& get(const C& obj) const { return obj.*member; }
	};
	template<class C, typename T>
	constexpr field<C, T> make_field(T C::* member, const char* name) { return field<C, T>(member, name); }

#define IMPLEMENT_SERIALIZATION(classname, ...) \
	typedef classname _this_class; \
	typedef void _is_serializable; \
	constexpr static auto _get_serialized_fields() { return std::make_tuple( __VA_ARGS__ ); } \
	classname(const jsonrpc::Value::Struct& json) { ::serialization::json_convert(*this, json); } \
	classname(const jsonrpc::Value& json) { ::serialization::json_convert(*this, json); } \
	classname(const rapidjson::Value& json) { ::serialization::json_convert(*this, json); }
#define FIELD_AS(fieldname, jsonname) ::serialization::make_field(&_this_class::fieldname, jsonname)
#define FIELD(name) FIELD_AS(name, #name)

	/*
	template<class C, typename T, size_t index, size_t size>
	struct serialize_member_n
	{
		static inline void Serialize(const C& obj, JsonObject& data, const T& tuple)
		{
			std::get<index>(tuple)::Serialize(obj, data);
			serialize_member_n<T, C, index + 1, size>::Serialize(obj, data, tuple);
		}
	};
	template<class C, typename T, size_t size>
	struct serialize_member_n<C, size, size>
	{

	};

	template<class C>
	static inline void serialize_members(const C& obj, JsonObject& data)
	{
		constexpr const auto fields = C::_serialized_fields;
		std::tuple<int, int, float> f;
		//std::get<
	}
	*/

	template<typename T> struct get_fields { typedef decltype(T::_get_serialized_fields()) type;  constexpr static auto fields = T::_get_serialized_fields(); };

	template<typename TO, typename FROM, typename ENABLE = void>
	struct json_converter { };

	template<typename SAME> struct json_converter<SAME, SAME>
	{
		static inline void convert(SAME& dst, const SAME& src) { dst = src; }
		static inline void convert(SAME& dst, SAME&& src) { dst = src; }
	};

	/*
	template<typename TO> struct json_converter<TO, jsonrpc::Value::Struct, typename TO::_is_serializable>;
	template<typename FROM> struct json_converter<jsonrpc::Value::Struct, FROM, typename FROM::_is_serializable>;
	template<typename TO> struct json_converter<TO, jsonrpc::Value, typename TO::_is_serializable>;
	template<typename FROM> struct json_converter<jsonrpc::Value, FROM, typename FROM::_is_serializable>;
	template<typename TO> struct json_converter<TO, rapidjson::Value, typename TO::_is_serializable>;
	template<typename FROM> struct json_converter<rapidjson::Value, FROM, typename FROM::_is_serializable>;
	*/

#define BASIC_CONVERSION(to, from, expr) template<> struct json_converter<to, from> { static inline void convert(to& dst, const from& src) { expr; } }

	BASIC_CONVERSION(bool, jsonrpc::Value, (dst = src.AsBoolean()));
	BASIC_CONVERSION(int32_t, jsonrpc::Value, (dst = src.AsInteger32()));
	BASIC_CONVERSION(uint32_t, jsonrpc::Value, (dst = src.AsInteger32()));
	BASIC_CONVERSION(std::string, jsonrpc::Value, (dst = src.AsString()));

	BASIC_CONVERSION(bool, rapidjson::Value, (dst = src.GetBool()));
	BASIC_CONVERSION(int32_t, rapidjson::Value, (dst = src.GetInt()));
	BASIC_CONVERSION(uint32_t, rapidjson::Value, (dst = src.GetUint()));
	BASIC_CONVERSION(std::string, rapidjson::Value, (dst = std::string(src.GetString(), src.GetStringLength())));

#undef BASIC_CONVERSION

	template<typename ELEM>
	struct json_converter<std::vector<ELEM>, jsonrpc::Value::Array>
	{
		static inline void convert(std::vector<ELEM>& dst, const jsonrpc::Value::Array& src)
		{
			dst.clear();
			dst.reserve(src.size());
			for (const auto& e : src)
				dst.push_back(json_cast<ELEM>(e));
		}
	};
	template<typename ELEM>
	struct json_converter<std::vector<ELEM>, jsonrpc::Value>
	{
		static inline void convert(std::vector<ELEM>& dst, const jsonrpc::Value& src)
		{
			json_convert(dst, src.AsArray());
		}
	};
	template<typename TO>
	struct json_converter<TO, jsonrpc::Value::Struct, typename TO::_is_serializable>
	{
		template<size_t iteration, size_t count>
		static inline std::enable_if_t<(iteration < count)> iterate(TO& dst, const jsonrpc::Value::Struct& src)
		{
			constexpr auto prop = std::get<iteration>(get_fields<TO>::fields);
			if (src.find(prop.name) != src.end())
				json_convert(prop.get(dst), src.at(prop.name));
			iterate<iteration + 1, count>(dst, src);
		}
		template<size_t iteration, size_t count> static inline std::enable_if_t<(iteration == count)> iterate(TO& dst, const jsonrpc::Value::Struct& src) {}
		static inline void convert(TO& dst, const jsonrpc::Value::Struct& src)
		{
			iterate<0, std::tuple_size<get_fields<TO>::type>::value>(dst, src);
		}
	};
	template<typename FROM>
	struct json_converter<jsonrpc::Value::Struct, FROM, typename FROM::_is_serializable>
	{
		template<size_t iteration, size_t count>
		static inline std::enable_if_t<(iteration < count)> iterate(jsonrpc::Value::Struct& dst, const FROM& src)
		{
			constexpr auto prop = std::get<iteration>(get_fields<FROM>::fields);
			json_convert(dst[prop.name], prop.get(src));
			iterate<iteration + 1, count>(dst, src);
		}
		template<size_t iteration, size_t count> static inline std::enable_if_t<(iteration == count)> iterate(jsonrpc::Value::Struct& dst, const FROM& src) {}
		static inline void convert(jsonrpc::Value::Struct& dst, const FROM& src)
		{
			iterate<0, std::tuple_size<get_fields<FROM>::type>::value>(dst, src);
		}
	};
	template<typename TO>
	struct json_converter<TO, jsonrpc::Value, typename TO::_is_serializable>
	{
		static inline void convert(TO& dst, const jsonrpc::Value& src)
		{
			json_converter<TO, jsonrpc::Value::Struct>::convert(dst, src.AsStruct());
		}
	};
	template<typename FROM>
	struct json_converter<jsonrpc::Value, FROM, typename FROM::_is_serializable>
	{
		static inline void convert(jsonrpc::Value& dst, const FROM& src)
		{
			jsonrpc::Value::Struct tmp;
			json_converter<jsonrpc::Value::Struct, FROM>::convert(tmp, src);
			dst = jsonrpc::Value(std::move(tmp));
		}
	};


	template<typename ELEM>
	struct json_converter<std::vector<ELEM>, rapidjson::Value>
	{
		static inline void convert(std::vector<ELEM>& dst, const rapidjson::Value& src)
		{
			if (!src.IsArray())
				throw std::exception();
			dst.clear();
			dst.reserve(src.Size());
			for (auto it = src.Begin(); it != src.End(); ++it)
				dst.push_back(json_cast<ELEM>(*it));
		}
	};
	template<typename TO>
	struct json_converter<TO, rapidjson::Value, typename TO::_is_serializable>
	{
		template<size_t iteration, size_t count>
		static inline std::enable_if_t<(iteration < count)> iterate(TO& dst, const rapidjson::Value& src)
		{
			constexpr auto prop = std::get<iteration>(get_fields<TO>::fields);
			auto mem = src.FindMember(prop.name);
			if (mem != src.MemberEnd())
				json_convert(prop.get(dst), mem->value);
			iterate<iteration + 1, count>(dst, src);
		}
		template<size_t iteration, size_t count> static inline std::enable_if_t<(iteration == count)> iterate(TO& dst, const rapidjson::Value& src) {}
		static inline void convert(TO& dst, const rapidjson::Value& src)
		{
			if (!src.IsObject())
				throw std::exception();
			iterate<0, std::tuple_size<get_fields<TO>::type>::value>(dst, src);
		}
	};
	template<typename FROM>
	struct json_converter<rapidjson::Value, FROM, typename FROM::_is_serializable>
	{
		template<size_t iteration, size_t count>
		static inline std::enable_if_t<(iteration < count)> iterate(rapidjson::Value& dst, const FROM& src)
		{
			constexpr auto prop = std::get<iteration>(get_fields<FROM>::fields);
			rapidjson::Value val;
			json_convert(val, prop.get(src));
			dst.AddMember(prop.name, std::move(val));
			iterate<iteration + 1, count>(dst, src);
		}
		template<size_t iteration, size_t count> static inline std::enable_if_t<(iteration == count)> iterate(rapidjson::Value& dst, const FROM& src) {}
		static inline void convert(rapidjson::Value& dst, const FROM& src)
		{
			dst.SetObject();
			iterate<0, std::tuple_size<get_fields<FROM>::type>::value>(dst, src);
		}
	};
	//template<> struct json_converter<bool, JsonValue> { static inline void convert(bool& dst, const JsonValue& src) { dst = src.AsBoolean(); } };


	template<typename TO, typename FROM>
	static inline void json_convert(TO& dst, const FROM& src)
	{
		json_converter<TO, FROM>::convert(dst, src);
	}
	template<typename TO, typename FROM>
	static inline TO json_cast(const FROM& from)
	{
		TO result;
		json_converter<TO, FROM>::convert(result, from);
		return std::move(result);
	}

};
