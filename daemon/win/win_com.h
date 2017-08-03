#pragma once

#include "win.h"

#define WIN_CHECK_COM(ptr, fn, args)       CheckError(ptr->fn args, #fn _D(, CURRENT_LOCATION))


template<class T> static inline T* SafeAddRef(T* const& p)
{
	if (p) p->AddRef();
	return p;
}
template<class T> static inline void SafeRelease(T*& p)
{
	IUnknown* u = p;
	p->Release();
	p = NULL;
}


template<class Interface = IUnknown, const IID* piid = &__uuidof(Interface)>
class com_ptr
{
	Interface* ptr;
private:
	template<typename I2, const IID* piid2> friend class com_ptr;
	void Reset(Interface* p = NULL) { if (ptr) ptr->Release(); ptr = p; }
	Interface* GetWithAddRef() const { if (ptr) ptr->AddRef(); return ptr; }
	template<typename I2, const IID* piid2> I2* GetWithQueryInterface() const { if (!ptr) return NULL; I2* p = NULL; WIN_CHECK_COM(ptr, QueryInterface, (*piid2, (void**)&p)); return p; }
	com_ptr(Interface* ptr) noexcept : ptr(ptr) {}
public:
	com_ptr() noexcept : ptr(NULL) {}
	com_ptr(const com_ptr& copy) : ptr(copy.GetWithAddRef()) {}
	com_ptr(com_ptr&& move) noexcept : ptr(std::exchange(move.ptr, nullptr)) {}
	com_ptr(nullptr_t) noexcept : ptr(NULL) {}
	template<class I2, const IID* piid2> explicit com_ptr(const com_ptr<I2, piid2>& copy) : ptr(copy.GetWithQueryInterface<Interface, piid>()) {}
	~com_ptr() { Reset(); }
	com_ptr& operator=(const com_ptr& copy) { Reset(copy.GetWithAddRef()); return *this; }
	com_ptr& operator=(com_ptr&& move) { Reset(std::exchange(move.ptr, nullptr)); return *this; }
	com_ptr& operator=(nullptr_t) { Reset(); return *this; }

	operator Interface*() const noexcept { return ptr; }
	Interface* operator->() const noexcept { return ptr; }

	Interface* release() noexcept { return std::exchange(ptr, nullptr); }

	static com_ptr wrap(Interface* ptr) noexcept { return com_ptr(ptr); }
	template<class Class> static com_ptr create(IUnknown* outer = NULL, DWORD context = CLSCTX_ALL) { return create(__uuidof(Class), outer, context); }
	template<class Class> static com_ptr create(DWORD context) { return create(__uuidof(Class), NULL, context); }
	static com_ptr create(const IID& clsid, DWORD context) { return create(clsid, NULL, context); }
	static com_ptr create(const IID& clsid, IUnknown* outer = NULL, DWORD context = CLSCTX_ALL) { Interface* p = NULL; WIN_CHECK_RESULT(CoCreateInstance, (clsid, outer, context, *piid, (void**)&p)); return com_ptr(p); }
};

template<class Class, class Interface>
static inline com_ptr<Interface> com_create(IUnknown* outer = NULL, DWORD context = CLSCTX_ALL) { return com_ptr<Interface>::create<Class>(outer, context); }

template<class Interface, class OriginalPtr> com_ptr<Interface> com_cast(OriginalPtr&& pUnknown)
{
	return com_ptr<Interface>(std::forward<OriginalPtr>(pUnknown));
}
template<class Interface> com_ptr<Interface> com_cast(IUnknown* pUnknown)
{
	Interface* p = NULL; if (pUnknown) CheckError(pUnknown->QueryInterface<Interface>(&p), "QueryInterface" _D(, CURRENT_LOCATION)); return com_ptr<Interface>::wrap(p);
}

template<class T> static inline void SafeRelease(com_ptr<T>& p) { p = nullptr; }


// Helper to transform a COM function with a single last output argument into a checked function returning the output

namespace impl {
	template<typename First, typename... Args> struct GetLastType { typedef typename GetLastType<Args...>::TYPE TYPE; };
	template<typename Last> struct GetLastType<Last> { typedef Last TYPE; };

	template<class Type, typename = void> struct com_ptr_wrapper { static Type wrap(Type&& t) noexcept { return t; } };
	template<class Type> struct com_ptr_wrapper<Type*, std::enable_if_t<std::is_base_of<IUnknown, Type>::value>> { static com_ptr<Type> wrap(Type*&& t) { return com_ptr<Type>::wrap(t); } };

	template<class Interface, typename... FnArgs>
	struct ComReturnHelper
	{
		Interface* p;
		typedef HRESULT(__stdcall Interface::*FnPtr)(FnArgs...);
		typedef std::remove_reference_t<decltype(*std::declval<typename GetLastType<FnArgs...>::TYPE>())> LastArg;
		FnPtr fn;
		const char* name;
#ifdef _DEBUG
		const Location& loc;
#endif
		ComReturnHelper(Interface* p, FnPtr fn, const char* name _D(, LOCATION_DECL)) noexcept : p(p), fn(fn), name(name) _D(, loc(LOCATION_VAR)) {}
		template<typename... Args> auto operator()(Args&&... args) -> decltype(com_ptr_wrapper<LastArg>::wrap(std::declval<LastArg>()))
		{
			LastArg arg;
			CheckError((p->*fn)(std::forward<Args>(args)..., &arg), name _D(, loc));
			return com_ptr_wrapper<LastArg>::wrap(std::move(arg));
		}
	};

	template<class InterfacePtr, class Interface = decltype(&*std::declval<InterfacePtr>()), typename... Args>
	static inline ComReturnHelper<Interface, Args...> ComWrapCheckedReturnArg(const InterfacePtr& p, HRESULT(__stdcall Interface::*fn)(Args...), const char* name _D(, LOCATION_DECL))
	{
		return ComReturnHelper<Interface, Args...>(p, fn, name _D(, LOCATION_VAR));
	}
}

#define WIN_CHECK_COM_OUTPUT(ptr, fn, args) (::impl::ComWrapCheckedReturnArg(ptr, &std::decay_t<decltype(*(ptr))>::fn, #fn _D(, CURRENT_LOCATION)) args)


// Helper class to iterate over a IEnumVARIANT collection

class IEnumVARIANT_iterator : public std::iterator<std::forward_iterator_tag, const VARIANT>
{
	com_ptr<IEnumVARIANT> ev;
	difference_type fetched;
	VARIANT v;

	void next()
	{
		if (!ev) return;
		if (fetched) { VariantClear(&v); }
		if (S_OK == ev->Next(1, &v, NULL))
			++fetched;
		else
			ev = nullptr; // end of sequence
	}
public:
	IEnumVARIANT_iterator() noexcept : fetched(0) { VariantInit(&v); }
	IEnumVARIANT_iterator(const IEnumVARIANT_iterator& copy) : ev(copy.ev), fetched(copy.fetched) { VariantInit(&v); VariantCopy(&v, &copy.v); }
	IEnumVARIANT_iterator(IEnumVARIANT_iterator&& move) : ev(std::move(move.ev)), fetched(move.fetched), v(std::move(move.v)) { VariantInit(&move.v); }
	explicit IEnumVARIANT_iterator(com_ptr<IEnumVARIANT> ev) noexcept : ev(std::move(ev)), fetched(0) { VariantInit(&v); }
	explicit IEnumVARIANT_iterator(IEnumVARIANT* ev) : IEnumVARIANT_iterator(com_ptr<IEnumVARIANT>::wrap(SafeAddRef(ev))) {}
	~IEnumVARIANT_iterator() { VariantClear(&v); }

	IEnumVARIANT_iterator& operator=(const IEnumVARIANT_iterator& copy) { ev = copy.ev; fetched = copy.fetched; VariantClear(&v); VariantCopy(&v, &copy.v); return *this; }
	IEnumVARIANT_iterator& operator=(IEnumVARIANT_iterator&& move) { ev = std::move(move.ev); fetched = move.fetched; VariantClear(&v); v = std::move(move.v); VariantInit(&move.v); return *this; }

	bool operator==(const IEnumVARIANT_iterator& it) noexcept { return ev == it.ev && (!ev || fetched == it.fetched); }
	bool operator!=(const IEnumVARIANT_iterator& it) noexcept { return !(*this == it); }

	reference operator*() { if (!ev || (!fetched && (next(), !ev))) throw std::out_of_range("invalid iterator"); return v; }
	pointer operator->() { return &(**this); }

	IEnumVARIANT_iterator& operator++() { next(); return *this; }
	IEnumVARIANT_iterator& operator++(int) { IEnumVARIANT_iterator it(*this); ++(*this); return it; }
};

IEnumVARIANT_iterator begin(IEnumVARIANT* ev) noexcept { return IEnumVARIANT_iterator(ev); }
IEnumVARIANT_iterator end(IEnumVARIANT* ev) noexcept { return IEnumVARIANT_iterator(); }

