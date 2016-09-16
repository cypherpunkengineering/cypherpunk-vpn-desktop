#pragma once

#include "config.h"

#include <ostream>

class noncopyable
{
protected:
	constexpr noncopyable() = default;
	~noncopyable() = default;
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;
};

template<typename CB>
class sentry
{
	CB cb;
public:
	sentry(const CB& callback) : cb(std::move(callback)) {}
	sentry(const sentry&) = delete;
	sentry(sentry&&) = delete;
	~sentry() noexcept { static_assert(noexcept(cb()), "must not throw exception"); cb(); }
	sentry& operator=(const sentry&) = delete;
	sentry& operator=(sentry&&) = delete;
};

template<typename CB>
static inline sentry<CB> finally(const CB& callback) { return { std::move(callback) }; }

#define CONCAT_(a,b) a##b
#define CONCAT(a,b) CONCAT_(a,b)

//#define SCOPE_EXIT const auto& CONCAT(__scope_exit,__LINE__) = scope_callback_helper() * [&]()
#define FINALLY(block) auto&& CONCAT(__scope_exit,__LINE__) = finally([&]() noexcept { block });

static inline std::ostream& operator<<(std::ostream& os, const std::exception& e)
{
	const char* what = e.what();
	const char* end = what + strlen(what);
	while (end > what && (isspace(end[-1]) || end[-1] == '.')) end--;
	os.write(what, end - what);
	return os;
}
