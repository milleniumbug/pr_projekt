#include <cstdlib>
#include <iostream>
#include <vector>
#include "variant.hpp"

namespace
{

template<std::size_t Number>
struct PrintAtDestruction
{
	int value;

	PrintAtDestruction()
	{
		value = 42;
	}

	PrintAtDestruction& operator=(PrintAtDestruction other)
	{
		std::swap(*this, other);
		return *this;
	}

	PrintAtDestruction(PrintAtDestruction&& other)
	{
		value = other.value;
		other.value = -1;
	}

	PrintAtDestruction(const PrintAtDestruction& other)
	{
		value = other.value;
	}

	~PrintAtDestruction()
	{
		if(value != -1)
			std::cerr << Number+value << "\n" << std::flush;
	}
};

static_assert(detail::Contains<int, char, int, int, char16_t, wchar_t, char>::value, "");
static_assert(!detail::Contains<char32_t, char, int, int, char16_t, wchar_t, char>::value, "");
static_assert(!detail::Contains<char32_t>::value, "");
static_assert(detail::Contains<int, int>::value, "");

static_assert(detail::Find<char, char, int, int, char16_t, wchar_t, char>::value == 0, "");
static_assert(detail::Find<int, char, int, int, char16_t, wchar_t, char>::value == 1, "");
static_assert(detail::Find<char16_t, char, int, int, char16_t, wchar_t, char>::value == 3, "");
static_assert(detail::Find<wchar_t, char, int, int, char16_t, wchar_t, char>::value == 4, "");

static_assert(detail::Max<>::value == 0, "");
static_assert(detail::Max<0>::value == 0, "");
static_assert(detail::Max<4, 42, 1, 77, 33, 12, 3, 3, 3, 0, 4>::value == 77, "");

static_assert(std::is_same<detail::NthType<0, char, int, int, char16_t, wchar_t, char>::type, char>::value, "");
static_assert(std::is_same<detail::NthType<1, char, int, int, char16_t, wchar_t, char>::type, int>::value, "");
static_assert(std::is_same<detail::NthType<3, char, int, int, char16_t, wchar_t, char>::type, char16_t>::value, "");
static_assert(std::is_same<detail::NthType<4, char, int, int, char16_t, wchar_t, char>::type, wchar_t>::value, "");

static_assert(alignof(detail::AlignedUnion<unsigned long long, char, int>::type) == alignof(unsigned long long), "");
static_assert(alignof(detail::AlignedUnion<int, char>::type) == alignof(int), "");
static_assert(alignof(detail::AlignedUnion<char>::type) == alignof(char), "");
static_assert(sizeof(detail::AlignedUnion<unsigned long long, char, int>::type) == sizeof(unsigned long long), "");
static_assert(sizeof(detail::AlignedUnion<int, char>::type) == sizeof(int), "");
static_assert(sizeof(detail::AlignedUnion<char>::type) == sizeof(char), "");

static_assert(!detail::Any<>::value, "");
static_assert(!detail::Any<false>::value, "");
static_assert(detail::Any<true>::value, "");
static_assert(!detail::Any<false, false>::value, "");
static_assert(detail::Any<true, false>::value, "");
static_assert(detail::Any<false, true>::value, "");
static_assert(detail::Any<true, true>::value, "");

static_assert(detail::All<>::value, "");
static_assert(!detail::All<false>::value, "");
static_assert(detail::All<true>::value, "");
static_assert(!detail::All<true, false>::value, "");
static_assert(!detail::All<false, true>::value, "");
static_assert(detail::All<true, true>::value, "");

static_assert(!detail::IsVariant<int>::value, "");
static_assert(detail::IsVariant<Variant<int, long>>::value, "");

void test_construction()
{
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> u((PrintAtDestruction<0>()));
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> v((PrintAtDestruction<1>()));
}

void test_overload_set()
{
	auto f = detail::make_overload_set(
		[](int){ std::cout << "int\n"; },
		[](char){ std::cout << "char\n"; },
		[](std::vector<int>){ std::cout << "std::vector<int>\n"; });

	f(4);
}

void test_overload_set_with_overload_ranker()
{
	auto ov = detail::make_overload_set(
		[](detail::choice<0>, int){ std::cout << "int\n"; },
		[](detail::choice<0>, char){ std::cout << "char\n"; },
		[](detail::choice<0>, std::vector<int>){ std::cout << "std::vector<int>\n"; },
		[](detail::otherwise, auto&&... x){ std::cout << "DEFAULT\n"; });

	auto selector = detail::select_overload();
	auto f = bind_first(ov, selector);

	f(short(42));
	f(std::vector<double>());
	f(4);
}

void test_overload_set_with_default()
{
	auto f = detail::make_overload_set_with_default(
		[](int){ std::cout << "int\n"; },
		[](char){ std::cout << "char\n"; },
		[](std::vector<int>){ std::cout << "std::vector<int>\n"; },
		[](Default){ std::cout << "DEFAULT\n"; });

	f(short(42));
	f(std::vector<double>());
	f(4);
}

void test_dispatch()
{
	Variant<int, char, std::vector<int>> v(4);
	dispatch(functions(
		[](int& x){ std::cout << "int " << x << "\n"; x = 42; std::cout << "int " << x << "\n"; },
		[](char x){ std::cout << "char\n"; },
		[](std::vector<int> x){ std::cout << "std::vector<int>\n"; }), v);
	auto& u = v;
	dispatch(functions(
		[](int& x){ std::cout << "int " << x << "\n"; },
		[](char x){ std::cout << "char\n"; },
		[](std::vector<int> x){ std::cout << "std::vector<int>\n"; }), v);
}

void test_multidispatch()
{
	Variant<int, std::vector<int>> u(4);
	Variant<int, std::vector<int>> v((std::vector<int>(2)));
	dispatch(functions(
		[](int& x, std::vector<int>& y){ std::cout << "ivYES" << x << "\n"; },
		[](std::vector<int>& x, int& y){ std::cout << "viNO" << y << "\n"; },
		[](int& x, int& y){ std::cout << "iiNO" << x << "\n"; },
		[](std::vector<int>& x, std::vector<int>& y){ std::cout << "vvNO" << "\n"; }), u, v);
}

void test_bindlast()
{
	auto f = [](int a, int b, char c, char d){ std::cout << a << b << c << d << "\n"; };
	char v = 'a';
	auto b = detail::bind_last(f, v);
	b(0, 1, 'z');
}

void test_copy_construction()
{
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> u((PrintAtDestruction<0>()));
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> v(u);
}

void test_move_construction()
{
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> u((PrintAtDestruction<0>()));
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> v(std::move(u));
}

void test_copy_assignment()
{
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> u((PrintAtDestruction<0>()));
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> v((PrintAtDestruction<1>()));
	u = v;
}

void test_move_assignment()
{
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> u((PrintAtDestruction<0>()));
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> v((PrintAtDestruction<1>()));
	u = std::move(v);
}

void test_typeinfo()
{
	Variant<int, PrintAtDestruction<1>, char, long> u(42);
	std::cout << (u.type() == typeid(int)) << "\n" << std::flush;
	Variant<int, PrintAtDestruction<1>, char, long> v((PrintAtDestruction<1>()));
	std::cout << (v.type() == typeid(PrintAtDestruction<1>)) << "\n" << std::flush;
}

}

#define TEST(t) do { std::cout << #t << "\n"; t(); } while(false)
void tests()
{
	TEST(test_construction);
	TEST(test_copy_construction);
	TEST(test_move_construction);
	TEST(test_copy_assignment);
	TEST(test_move_assignment);
	TEST(test_typeinfo);
	TEST(test_overload_set);
	TEST(test_overload_set_with_overload_ranker);
	TEST(test_overload_set_with_default);
	TEST(test_bindlast);
	TEST(test_dispatch);
	TEST(test_multidispatch);
	std::cout << std::flush;
	exit(0);
}