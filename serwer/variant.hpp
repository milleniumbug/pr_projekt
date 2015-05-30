#ifndef VARIANT_HPP
#define VARIANT_HPP

#include <functional>
#include <typeinfo>
#include <utility>
#include <type_traits>

template<typename T>
struct Identity
{
	typedef T type;
};

template<typename T>
using IdentityAlias = T;

template<typename Type, typename... Args>
struct Contains;

template<typename Type>
struct Contains<Type> : std::false_type
{
	
};

template<typename Type, typename Head, typename... Tail>
struct Contains<Type, Head, Tail...> : std::conditional<
	(std::is_same<Type, Head>::value || Contains<Type, Tail...>::value),
	std::true_type,
	std::false_type
>::type
{
	
};

template<typename Type, std::size_t Index, typename... Args>
struct FindImpl;

template<typename Type, std::size_t Index>
struct FindImpl<Type, Index>;

template<typename Type, std::size_t Index, typename Head, typename... Tail>
struct FindImpl<Type, Index, Head, Tail...> : std::conditional<
	(std::is_same<Type, Head>::value),
	std::integral_constant<std::size_t, Index>,
	FindImpl<Type, Index+1, Tail...>
>::type
{

};

template<typename Type, typename... Args>
struct Find : FindImpl<Type, 0, Args...>
{

};

template<std::size_t Index, std::size_t Current, typename... Args>
struct NthTypeImpl;

template<std::size_t Index, std::size_t Current>
struct NthTypeImpl<Index, Current>;

template<std::size_t Index, std::size_t Current, typename Head, typename... Tail>
struct NthTypeImpl<Index, Current, Head, Tail...> : std::conditional<
	Index == Current,
	Identity<Head>,
	NthTypeImpl<Index, Current+1, Tail...>
>::type
{

};

template<std::size_t Index, typename... Args>
struct NthType : NthTypeImpl<Index, 0, Args...>
{

};

template<std::size_t M, std::size_t... Args>
struct MaxImpl;

template<std::size_t M>
struct MaxImpl<M> : std::integral_constant<std::size_t, M>
{

};

template<std::size_t M, std::size_t Head, std::size_t... Tail>
struct MaxImpl<M, Head, Tail...> : MaxImpl<(M < Head) ? Head : M, Tail...>
{

};

template<std::size_t... Args>
struct Max : MaxImpl<0, Args...>
{

};

template<bool... Args>
struct Any;

template<>
struct Any<> : std::false_type
{

};

template<bool Head, bool... Tail>
struct Any<Head, Tail...> : std::conditional<
	Head,
	std::true_type,
	Any<Tail...>
>::type
{

};

template<bool... Args>
struct All;

template<>
struct All<> : std::true_type
{

};

template<bool Head, bool... Tail>
struct All<Head, Tail...> : std::conditional<
	Head,
	All<Tail...>,
	std::false_type
>::type
{

};

template<typename Function>
struct SignatureReturnType
{

};

template<typename R, typename... Args>
struct SignatureReturnType<R(Args...)>
{
	typedef R type;
};

template<typename... Args>
struct AlignedUnion
{
	static const std::size_t alignment_value = Max<alignof(Args)...>::value;
	using type alignas(alignment_value) = char[Max<sizeof(Args)...>::value];
};

template<typename T>
struct VariantOps
{
	static void destroy(void* what)
	{
		static_cast<T*>(what)->~T();
	}

	static void copy_construct(void* to, const void* arg)
	{
		::new(to) T(*static_cast<const T*>(arg));
	}

	static void move_construct(void* to, void* arg)
	{
		::new(to) T(std::move(*static_cast<T*>(arg)));
	}

	static const std::type_info& typeinfo()
	{
		return typeid(T);
	}
};

template<typename Function, typename Arg>
static void call_lvalue(void* fptr, void* argptr)
{
	auto& f = *(Function*)fptr;
	auto& arg = *(Arg*)argptr;
	f(arg);
}

template<typename... Functions>
struct OverloadSet;

template<typename Head>
struct OverloadSet<Head> : Head
{
	using Head::operator();

	template<typename ArgsH>
	OverloadSet(ArgsH&& argh) :
		Head(argh)
	{

	}
};

template<typename Head, typename... Tail>
struct OverloadSet<Head, Tail...> : Head, OverloadSet<Tail...>
{
	using OverloadSet<Tail...>::operator();
	using Head::operator();

	template<typename ArgsH, typename... ArgsT>
	OverloadSet(ArgsH&& argh, ArgsT&&... argt) :
		Head(argh),
		OverloadSet<Tail...>(std::forward<ArgsT>(argt)...)
	{

	}
};

template<typename... Args>
OverloadSet<Args...> make_overload_set(Args&&... args)
{
	return OverloadSet<Args...>(std::forward<Args>(args)...);
}

#include <iostream>

// usage: TYPE(std::pair<int, int>)
#define TYPE(...) IdentityAlias<__VA_ARGS__>
#define DISPATCH(functiontype, what, index, ...) \
[&]() -> SignatureReturnType< functiontype >::type \
{ \
	using DISPATCH_function = functiontype; \
	using DISPATCH_functionptr = DISPATCH_function*; \
	static const DISPATCH_functionptr DISPATCH_funs[] = { what }; \
	return DISPATCH_funs[index](__VA_ARGS__); \
}()

template<typename... Args>
class Variant
{
private:
	static const std::size_t no_value = static_cast<std::size_t>(-1);
	std::size_t selector;
	typename AlignedUnion<Args...>::type storage;

	void* as_voidptr()
	{
		return static_cast<void*>(&storage);
	}

	const void* as_voidptr() const
	{
		return static_cast<const void*>(&storage);
	}

	template<typename T>
	T& as_type()
	{
		return *static_cast<T*>(as_voidptr());
	}

	template<typename T>
	const T& as_type() const
	{
		return *static_cast<const T*>(as_voidptr());
	}

public:
	template<typename T, typename = typename std::enable_if<Contains<T, Args...>::value>::type>
	Variant(T&& value) :
		selector(no_value)
	{
		::new(as_voidptr()) T(std::forward<T>(value));
		selector = Find<T, Args...>::value;
	}

	// TODO: Make it provide strong exception guarantee
	template<typename T, typename = typename std::enable_if<Contains<T, Args...>::value>::type>
	Variant& operator=(T&& value)
	{
		this->~Variant(); // doesn't throw
		::new(this) Variant(std::forward<T>(value)); // FAIL: if it throws, the object is in unusable state
	}

	Variant(const Variant& other) :
		selector(no_value)
	{
		DISPATCH(
			TYPE( void(void*, const void*) ),
			(&VariantOps<Args>::copy_construct)...,
			other.selector,
			as_voidptr(), other.as_voidptr()
		);
		selector = other.selector;
	}

	Variant(Variant&& other) noexcept(noexcept(All<std::is_nothrow_move_constructible<Args>::value...>::value))
	{
		DISPATCH(
			TYPE( void(void*, void*) ),
			(&VariantOps<Args>::move_construct)...,
			other.selector,
			as_voidptr(), other.as_voidptr()
		);
		selector = other.selector; // doesn't throw
		other.selector = no_value; // doesn't throw
	}

	Variant& operator=(const Variant& other)
	{
		Variant tmp(other);
		std::swap(*this, tmp);
		return *this;
	}

	// TODO: Make it provide strong exception guarantee
	Variant& operator=(Variant&& other) noexcept(noexcept(All<std::is_nothrow_move_constructible<Args>::value...>::value))
	{
		this->~Variant(); // doesn't throw
		::new(static_cast<void*>(this)) Variant(std::move(other)); // FAIL: if it throws, the object is in unusable state
		return *this;
	}

	~Variant()
	{
		if(selector != no_value)
		{
			DISPATCH(
				TYPE( void(void*) ),
				(&VariantOps<Args>::destroy)...,
				selector,
				as_voidptr()
			);
			selector = no_value;
		}
	}

	const std::type_info& type() const
	{
		return DISPATCH(
			TYPE( const std::type_info&() ),
			(&VariantOps<Args>::typeinfo)...,
			selector,
			/* no arguments */
		);
	}

	template<typename... Functions>
	void type_switch(Functions&&... f)
	{
		auto ovset = make_overload_set( std::forward<Functions>(f)... );
		DISPATCH(
			TYPE( void(void*, void*) ),
			(&call_lvalue<decltype(ovset), Args>)...,
			selector,
			static_cast<void*>(&ovset), as_voidptr()
		);
	}

	template<typename... Functions>
	void type_switch(Functions&&... f) const
	{
		auto ovset = make_overload_set( std::forward<Functions>(f)... );
		DISPATCH(
			TYPE( void(void*, void*) ),
			(&call_lvalue<decltype(ovset), typename std::add_const<Args>::type>)...,
			selector,
			static_cast<void*>(&ovset), as_voidptr()
		);
	}
};

#undef DISPATCH
#undef TYPE

#endif