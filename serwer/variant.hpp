#ifndef VARIANT_HPP
#define VARIANT_HPP

#include <functional>
#include <typeinfo>
#include <utility>
#include <type_traits>

struct Default
{

};

namespace detail
{

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
static void call_lvalue(const void* fptr, const void* argptr)
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
OverloadSet<typename std::remove_reference<Args>::type...> make_overload_set(Args&&... args)
{
	return OverloadSet<typename std::remove_reference<Args>::type...>(std::forward<Args>(args)...);
}

template<typename T, typename F>
struct BinderLast
{
	template<typename... Args>
	auto operator()(Args&&... args) const
	{
		return (*fun)(std::forward<Args>(args)..., *val);
	}

	F* fun;
	T* val;
};

template<typename T, typename F>
BinderLast<T, F> bind_last(F& f, T& val)
{
	BinderLast<T, F> b;
	b.fun = &f;
	b.val = &val;
	return b;
}

template<typename T, typename F>
struct BinderFirst
{
	template<typename... Args>
	auto operator()(Args&&... args) const
	{
		return (*fun)(*val, std::forward<Args>(args)...);
	}

	F* fun;
	T* val;
};

template<typename T, typename F>
BinderFirst<T, F> bind_first(F& f, T& val)
{
	BinderFirst<T, F> b;
	b.fun = &f;
	b.val = &val;
	return b;
}

template<unsigned I> struct choice : choice<I+1>{};
template<> struct choice<1>{};
 
struct otherwise{ otherwise(...){} };

struct select_overload : choice<0>{};

template<typename Function,
         typename... Args>
auto call_with_default(choice<0>, Function f, Args&&... args) -> decltype(f(std::forward<Args>(args)...), void())
{
	return f(std::forward<Args>(args)...);
}

template<typename Function,
         typename... Args>
void call_with_default(choice<1>, Function f, Args&&... args)
{
	return f(Default());
}

template<typename... Args>
auto make_overload_set_with_default(Args&&... args)
{
	return
	[ovset = make_overload_set(std::forward<Args>(args)...)]
	(auto&&... args)
	{
		call_with_default(select_overload(), ovset, std::forward<decltype(args)>(args)...);
	};
}

}

// usage: TYPE(std::pair<int, int>)
#define TYPE(...) detail::IdentityAlias<__VA_ARGS__>
#define DISPATCH(functiontype, what, index, ...) \
[&]() -> detail::SignatureReturnType< functiontype >::type \
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
	typename detail::AlignedUnion<Args...>::type storage;

	template<typename T>
	T& as_type()
	{
		return *static_cast<T*>(raw());
	}

	template<typename T>
	const T& as_type() const
	{
		return *static_cast<const T*>(raw());
	}

public:
	template<typename T, typename = typename std::enable_if<detail::Contains<T, Args...>::value>::type>
	Variant(T&& value) :
		selector(no_value)
	{
		::new(raw()) T(std::forward<T>(value));
		selector = detail::Find<T, Args...>::value;
	}

	// TODO: Make it provide strong exception guarantee
	template<typename T, typename = typename std::enable_if<detail::Contains<T, Args...>::value>::type>
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
			(&detail::VariantOps<Args>::copy_construct)...,
			other.selector,
			raw(), other.raw()
		);
		selector = other.selector;
	}

	Variant(Variant&& other) noexcept(noexcept(detail::All<std::is_nothrow_move_constructible<Args>::value...>::value))
	{
		DISPATCH(
			TYPE( void(void*, void*) ),
			(&detail::VariantOps<Args>::move_construct)...,
			other.selector,
			raw(), other.raw()
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
	Variant& operator=(Variant&& other) noexcept(noexcept(detail::All<std::is_nothrow_move_constructible<Args>::value...>::value))
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
				(&detail::VariantOps<Args>::destroy)...,
				selector,
				raw()
			);
			selector = no_value;
		}
	}

	const std::type_info& type() const
	{
		return DISPATCH(
			TYPE( const std::type_info&() ),
			(&detail::VariantOps<Args>::typeinfo)...,
			selector,
			/* no arguments */
		);
	}

	std::size_t which() const
	{
		return selector;
	}

	bool empty() const
	{
		return selector != no_value;
	}

	void* raw()
	{
		return static_cast<void*>(&storage);
	}

	const void* raw() const
	{
		return static_cast<const void*>(&storage);
	}
};

namespace detail
{

template<typename V>
struct IsVariant : std::false_type
{

};

template<typename... Args>
struct IsVariant<Variant<Args...>> : std::true_type
{

};

}

template<typename... VariantArgs, typename Function>
void dispatch(Function fn, Variant<VariantArgs...>& var)
{
	DISPATCH(
		TYPE( void(const void*, const void*) ),
		(&detail::call_lvalue<Function, VariantArgs>)...,
		var.which(),
		static_cast<void*>(&fn), var.raw()
	);
}


template<typename... VariantArgs, typename Function>
void dispatch(Function fn, const Variant<VariantArgs...>& var)
{
	DISPATCH(
		TYPE( void(const void*, const void*) ),
		(&detail::call_lvalue<Function, typename std::add_const<VariantArgs>::type>)...,
		var.which(),
		static_cast<void*>(&fn), var.raw()
	);
}

template<typename... V, typename... VariantArgs, typename Function>
void dispatch(Function fn, Variant<VariantArgs...>& var, V&&... vars)
{
	dispatch([&](auto&& x)
	{
		dispatch(bind_last(fn, x), var);
	}, std::forward<V>(vars)...);
}

template<typename... V, typename... VariantArgs, typename Function>
void dispatch(Function fn, const Variant<VariantArgs...>& var, V&&... vars)
{
	dispatch([&](auto&& x)
	{
		dispatch(bind_last(fn, x), var);
	}, std::forward<V>(vars)...);
}

template<typename... Args>
auto functions(Args&&... args)
{
	return detail::make_overload_set_with_default(std::forward<Args>(args)...);
}


#undef DISPATCH
#undef TYPE

#endif