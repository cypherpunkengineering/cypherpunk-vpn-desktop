#pragma once

#include "config.h"

#include <ostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

class noncopyable
{
protected:
	constexpr noncopyable() noexcept = default;
	~noncopyable() noexcept = default;
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;
};

class nonmovable
{
protected:
	constexpr nonmovable() noexcept = default;
	~nonmovable() noexcept = default;
	nonmovable(nonmovable&&) = delete;
	nonmovable& operator=(const nonmovable&&) = delete;
};


template<typename CB>
class sentry : private noncopyable, private nonmovable
{
	CB cb;
public:
	sentry(const CB& callback) : cb(std::move(callback)) {}
	//sentry(const sentry&) = delete;
	//sentry(sentry&&) = delete;
	~sentry() noexcept { static_assert(noexcept(cb()), "must not throw exception"); cb(); }
	//sentry& operator=(const sentry&) = delete;
	//sentry& operator=(sentry&&) = delete;
};

template<typename CB>
static inline sentry<CB> finally(const CB& callback) { return { std::move(callback) }; }

#define CONCAT_(a,b) a##b
#define CONCAT(a,b) CONCAT_(a,b)

//#define SCOPE_EXIT const auto& CONCAT(__scope_exit,__LINE__) = scope_callback_helper() * [&]()
#define FINALLY(block) auto&& CONCAT(__scope_exit,__LINE__) = finally([&]() noexcept { block });


class SystemException : public std::system_error, public DebugLocation
{
	const char* _from;
public:
	inline SystemException(int code, const char* from _D(, const Location& location)) : std::system_error(code, std::system_category(), prefix(from, location)), DebugLocation(_D(location)), _from(from) {}
	inline SystemException(int code _D(, const Location& location)) : std::system_error(code, std::system_category(), prefix(nullptr, location)), DebugLocation(_D(location)), _from(nullptr) {}

	int value() const { return code().value(); }
	const DebugLocation& location() const { return *this; }
	const char* from() const { return _from; }

private:
	static inline std::string prefix(const char* from _D(, const Location& location))
	{
		std::ostringstream os;
		if (from)
			os << from << " failed";
		else
			os << "Exception thrown";
#ifdef _DEBUG
		if (location.path())
		{
			os << " at " << location.file();
			if (location.line())
				os << ':' << std::to_string(location.line());
		}
#endif
		return os.str();
	}
};


static inline std::ostream& operator<<(std::ostream& os, const std::exception& e)
{
	const char* what = e.what();
	const char* end = what + strlen(what);
	while (end > what && (isspace(end[-1]) || end[-1] == '.')) end--;
	os.write(what, end - what);
	return os;
}


template<typename IT, typename SEP, typename CB>
static inline size_t SplitToIterators(const IT& begin, const IT& end, const SEP& sep, const CB& cb)
{
	IT pos = begin, last = pos;
	size_t count = 0;
	while ((pos = std::find(last, end, sep)) != end)
	{
		cb(last, pos);
		last = pos;
		++last;
		++count;
	}
	cb(last, end);
	++count;
	return count;
}

template<typename CB>
static inline size_t SplitToIterators(const std::string& text, char sep, const CB& cb)
{
	return SplitToIterators(text.begin(), text.end(), sep, cb);
}

template<typename CB>
static inline size_t SplitToStrings(const std::string& text, char sep, const CB& cb)
{
	return SplitToIterators(text, sep, [&](const std::string::const_iterator& b, const std::string::const_iterator& e) { return std::string(b, e); });
}

static inline std::vector<std::string> SplitToVector(const std::string& text, char sep)
{
	std::vector<std::string> result;
	SplitToIterators(text, sep, [&](const std::string::const_iterator& b, const std::string::const_iterator& e) { result.emplace_back(b, e); });
	return std::move(result);
}
