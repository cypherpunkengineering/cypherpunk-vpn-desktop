#pragma once

#include "config.h"
#include "debug.h"

#include <deque>
#include <functional>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>
#include <utility>

#if OS_LINUX
#include <algorithm>
#include <string.h>
#endif

class noncopyable
{
protected:
	constexpr noncopyable() noexcept = default;
	~noncopyable() noexcept = default;
	noncopyable(noncopyable&&) = default;
	noncopyable& operator=(noncopyable&&) = default;
private:
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;
};

class nonmovable
{
protected:
	constexpr nonmovable() noexcept = default;
	~nonmovable() noexcept = default;
	nonmovable(const nonmovable&) = default;
	nonmovable& operator=(const nonmovable&) = default;
private:
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
#define FINALLY(block) auto&& CONCAT(__scope_exit,__LINE__) = finally([&]() noexcept { block }); (void)CONCAT(__scope_exit,__LINE__);
#define CLEANUP FINALLY


class SystemException : public std::system_error, public DebugLocation
{
	const char* _from;
public:
	inline SystemException(int code, const char* from _D(, const Location& location)) : std::system_error(code, std::system_category(), prefix(from _D(, location))), DebugLocation(_D(location)), _from(from) {}
	inline SystemException(int code _D(, const Location& location)) : std::system_error(code, std::system_category(), prefix(nullptr _D(, location))), DebugLocation(_D(location)), _from(nullptr) {}

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
static inline std::ostream& operator<<(std::ostream& os, const std::error_code& error)
{
	return os << error.message();
}

class streamstring
{
	std::ostringstream _str;
public:
	operator std::string() const { return _str.str(); }
	template<typename T> streamstring& operator<<(T&& var) { _str << std::forward<T>(var); return *this; }
};


template<typename IT, typename SEP, typename CB>
static inline size_t SplitToIterators(const IT& begin, const IT& end, const SEP& sep, int max_splits, const CB& cb)
{
	IT pos = begin, last = pos;
	size_t count = 0;
	while ((max_splits < 0 || count < (size_t)max_splits) && (pos = std::find(last, end, sep)) != end)
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
template<typename IT, typename SEP, typename CB>
static inline size_t SplitToIterators(const IT& begin, const IT& end, const SEP& sep, const CB& cb)
{
	return SplitToIterators(begin, end, sep, -1, cb);
}

template<typename CB>
static inline size_t SplitToIterators(const std::string& text, char sep, int max_splits, const CB& cb)
{
	return SplitToIterators(text.begin(), text.end(), sep, cb);
}
template<typename CB>
static inline size_t SplitToIterators(const std::string& text, char sep, const CB& cb)
{
	return SplitToIterators(text, sep, -1, cb);
}

template<typename CB>
static inline size_t SplitToStrings(const std::string& text, char sep, int max_splits, const CB& cb)
{
	return SplitToIterators(text, sep, [&](const std::string::const_iterator& b, const std::string::const_iterator& e) { cb(std::string(b, e)); });
}
template<typename CB>
static inline size_t SplitToStrings(const std::string& text, char sep, const CB& cb)
{
	return SplitToStrings(text, sep, -1, cb);
}

static inline std::vector<std::string> SplitToVector(const std::string& text, char sep, int max_splits = -1)
{
	std::vector<std::string> result;
	SplitToIterators(text, sep, max_splits, [&](const std::string::const_iterator& b, const std::string::const_iterator& e) { result.emplace_back(b, e); });
	return result;
}
static inline std::deque<std::string> SplitToDeque(const std::string& text, char sep, int max_splits = -1)
{
	std::deque<std::string> result;
	SplitToIterators(text, sep, max_splits, [&](const std::string::const_iterator& b, const std::string::const_iterator& e) { result.emplace_back(b, e); });
	return result;
}

//template<typename C>
//static inline std::enable_if_t<std::is_base_of<std::enable_shared_from_this<C>, C>::value, std::shared_ptr<C>>
//shared_if_available(C* instance) { return instance->shared_from_this(); }
//template<typename C>
//static inline std::enable_if_t<std::is_base_of<std::enable_shared_from_this<C>, C>::value, std::shared_ptr<const C>>
//shared_if_available(const C* instance) { return instance->shared_from_this(); }
//template<typename C>
//static inline std::enable_if_t<!std::is_base_of<std::enable_shared_from_this<C>, C>::value, C*>
//shared_if_available(C* instance) { return instance; }
//template<typename C>
//static inline std::enable_if_t<!std::is_base_of<std::enable_shared_from_this<C>, C>::value, const C*>
//shared_if_available(const C* instance) { return instance; }

template<typename C, typename P, typename R, typename... Args>
static inline std::function<R(Args...)> bind_this(R (C::*fn)(Args...), P instance)
{
	return [instance = std::move(instance), fn](Args&&... args) { return ((*instance).*fn)(std::forward<Args>(args)...); };
}
template<typename C, typename P, typename... Args>
static inline std::function<void(Args...)> bind_this(void (C::*fn)(Args...), P instance)
{
	return [instance = std::move(instance), fn](Args&&... args) { ((*instance).*fn)(std::forward<Args>(args)...); };
}

template<typename C, typename P, typename R, typename... Args>
static inline std::function<R(Args...)> weak_bind_this(R (C::*fn)(Args...), P instance, R value)
{
	return [instance = std::move(instance), fn, value = std::move(value)](Args&&... args) { auto self = instance.lock(); if (self) return ((*self).*fn)(std::forward<Args>(args)...); else return std::move(value); };
}
template<typename C, typename P, typename... Args>
static inline std::function<void(Args...)> weak_bind_this(void (C::*fn)(Args...), P instance)
{
	return [instance = std::move(instance), fn](Args&&... args) { auto self = instance.lock(); if (self) ((*self).*fn)(std::forward<Args>(args)...); };
}

template<typename PTR, typename LAMBDA, typename VALUE = void> class bound_ptr_lambda;

template<typename PTR, typename LAMBDA>
class bound_ptr_lambda<PTR, LAMBDA, void>
{
	PTR ptr;
	LAMBDA lambda;
public:
	bound_ptr_lambda(PTR ptr, LAMBDA lambda) : ptr(std::move(ptr)), lambda(std::move(lambda)) {}
	template<typename ...Args>
	auto operator()(Args&&... args) -> decltype(lambda(std::forward<Args>(args)...))
	{
		return lambda(std::forward<Args>(args)...);
	}
};
template<typename T, typename LAMBDA>
class bound_ptr_lambda<std::weak_ptr<T>, LAMBDA, void>
{
	std::weak_ptr<T> ptr;
	LAMBDA lambda;
public:
	bound_ptr_lambda(std::weak_ptr<T> ptr, LAMBDA lambda) : ptr(std::move(ptr)), lambda(std::move(lambda)) {}
	template<typename ...Args>
	void operator()(Args&&... args)
	{
		auto p = ptr.lock();
		if (p)
		{
			lambda(std::forward<Args>(args)...);
		}
	}
};
template<typename T, typename LAMBDA, typename VALUE>
class bound_ptr_lambda<std::weak_ptr<T>, LAMBDA, VALUE>
{
	std::weak_ptr<T> ptr;
	LAMBDA lambda;
	VALUE value;
public:
	bound_ptr_lambda(std::weak_ptr<T> ptr, LAMBDA lambda, VALUE value) : ptr(std::move(ptr)), lambda(std::move(lambda)), value(std::move(value)) {}
	template<typename ...Args>
	auto operator()(Args&&... args) -> decltype(ptr.lock() ? lambda(std::forward<Args>(args)...) : value)
	{
		auto p = ptr.lock();
		if (p)
			return lambda(std::forward<Args>(args)...);
		else
			return value;
	}
};
template<typename T, typename LAMBDA> auto bind_ptr_to_lambda(std::shared_ptr<T> ptr, LAMBDA lambda) -> bound_ptr_lambda<std::shared_ptr<T>, LAMBDA>
{
	return bound_ptr_lambda<std::shared_ptr<T>, LAMBDA>(std::move(ptr), std::move(lambda));
}
template<typename T, typename LAMBDA> auto bind_ptr_to_lambda(std::weak_ptr<T> ptr, LAMBDA lambda) -> bound_ptr_lambda<std::weak_ptr<T>, LAMBDA>
{
	return bound_ptr_lambda<std::weak_ptr<T>, LAMBDA>(std::move(ptr), std::move(lambda));
}
template<typename T, typename LAMBDA, typename VALUE> auto bind_ptr_to_lambda(std::weak_ptr<T> ptr, LAMBDA lambda, VALUE value) -> bound_ptr_lambda<std::weak_ptr<T>, LAMBDA, VALUE>
{
	return bound_ptr_lambda<std::weak_ptr<T>, LAMBDA, VALUE>(std::move(ptr), std::move(lambda), std::move(value));
}

template<class T, class U>
struct static_pointer_cast_if_needed_t { static std::shared_ptr<T> call(const std::shared_ptr<U>& p) { return std::static_pointer_cast<T>(p); } };
template<class T>
struct static_pointer_cast_if_needed_t<T, T> { static std::shared_ptr<T> call(const std::shared_ptr<T>& p) { return p; } static std::shared_ptr<T> call(std::shared_ptr<T>&& p) { return std::move(p); } };


// Return the type of the current class.
#define THIS_TYPE std::decay_t<decltype(*this)>

// Return a member function pointer.
#define THIS_FUNCTION(name) &THIS_TYPE::name

// Return a shared_ptr to 'this' obtained from shared_from_this().
#define SHARED_THIS static_pointer_cast_if_needed_t<THIS_TYPE, std::decay_t<decltype(*(this->shared_from_this().get()))>>::call(this->shared_from_this())

// Wrap a member function so that it carries a shared_ptr obtained from shared_from_this().
#define SHARED_CALLBACK(name) ::bind_this(THIS_FUNCTION(name), SHARED_THIS)

// Wrap a 'this'-bound lambda so it also carries a shared_ptr reference to 'this', keeping the object alive.
#define SHARED_LAMBDA(lambda) ::bind_ptr_to_lambda(SHARED_THIS, lambda)

// Return a shared_ptr pointing to a member but using the lifetime of shared_from_this.
#define SHARED_MEMBER(name) std::shared_ptr<decltype(name)>(SHARED_THIS, &name)

// Return a weak_ptr to 'this' obtained from shared_from_this().
#define WEAK_THIS std::weak_ptr<THIS_TYPE>(SHARED_THIS)

// Wrap a member function so that it carries a weak_ptr obtained from shared_from_this(), optionally returning a fallback value if the pointer has expired.
#define WEAK_CALLBACK(name, ...) ::weak_bind_this(THIS_FUNCTION(name), WEAK_THIS ,## __VA_ARGS__)

// Wrap a 'this'-bound lambda so it also carries a weak_ptr reference to this, and only invokes after successfully locking the weak_ptr.
#define WEAK_LAMBDA(lambda, ...) ::bind_ptr_to_lambda(WEAK_THIS, lambda ,## __VA_ARGS__)

// Return a naked member function callback bound to 'this', without lifetime management
#define THIS_CALLBACK(name) ::bind_this(THIS_FUNCTION(name), this)



