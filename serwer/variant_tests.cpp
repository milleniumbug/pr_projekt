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

static_assert(Contains<int, char, int, int, char16_t, wchar_t, char>::value, "");
static_assert(!Contains<char32_t, char, int, int, char16_t, wchar_t, char>::value, "");
static_assert(!Contains<char32_t>::value, "");
static_assert(Contains<int, int>::value, "");

static_assert(Find<char, char, int, int, char16_t, wchar_t, char>::value == 0, "");
static_assert(Find<int, char, int, int, char16_t, wchar_t, char>::value == 1, "");
static_assert(Find<char16_t, char, int, int, char16_t, wchar_t, char>::value == 3, "");
static_assert(Find<wchar_t, char, int, int, char16_t, wchar_t, char>::value == 4, "");

static_assert(Max<>::value == 0, "");
static_assert(Max<0>::value == 0, "");
static_assert(Max<4, 42, 1, 77, 33, 12, 3, 3, 3, 0, 4>::value == 77, "");

static_assert(std::is_same<NthType<0, char, int, int, char16_t, wchar_t, char>::type, char>::value, "");
static_assert(std::is_same<NthType<1, char, int, int, char16_t, wchar_t, char>::type, int>::value, "");
static_assert(std::is_same<NthType<3, char, int, int, char16_t, wchar_t, char>::type, char16_t>::value, "");
static_assert(std::is_same<NthType<4, char, int, int, char16_t, wchar_t, char>::type, wchar_t>::value, "");

static_assert(alignof(AlignedUnion<unsigned long long, char, int>::type) == alignof(unsigned long long), "");
static_assert(alignof(AlignedUnion<int, char>::type) == alignof(int), "");
static_assert(alignof(AlignedUnion<char>::type) == alignof(char), "");
static_assert(sizeof(AlignedUnion<unsigned long long, char, int>::type) == sizeof(unsigned long long), "");
static_assert(sizeof(AlignedUnion<int, char>::type) == sizeof(int), "");
static_assert(sizeof(AlignedUnion<char>::type) == sizeof(char), "");

static_assert(!Any<>::value, "");
static_assert(!Any<false>::value, "");
static_assert(Any<true>::value, "");
static_assert(!Any<false, false>::value, "");
static_assert(Any<true, false>::value, "");
static_assert(Any<false, true>::value, "");
static_assert(Any<true, true>::value, "");

static_assert(All<>::value, "");
static_assert(!All<false>::value, "");
static_assert(All<true>::value, "");
static_assert(!All<true, false>::value, "");
static_assert(!All<false, true>::value, "");
static_assert(All<true, true>::value, "");

void test_construction()
{
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> u((PrintAtDestruction<0>()));
	Variant<PrintAtDestruction<0>, PrintAtDestruction<1>> v((PrintAtDestruction<1>()));
}

void test_overload_set()
{
	auto f = make_overload_set(
		[](int){ std::cout << "int\n"; },
		[](char){ std::cout << "char\n"; },
		[](std::vector<int>){ std::cout << "std::vector<int>\n"; });

	f(4);
}

void test_type_switch()
{
	Variant<int, char, std::vector<int>> v(4);
	v.type_switch(
		[](int& x){ std::cout << "int " << x << "\n"; x = 42; std::cout << "int " << x << "\n"; },
		[](char x){ std::cout << "char\n"; },
		[](std::vector<int> x){ std::cout << "std::vector<int>\n"; });
	const auto& u = v;
	v.type_switch(
		[](int& x){ std::cout << "int " << x << "\n"; },
		[](char x){ std::cout << "char\n"; },
		[](std::vector<int> x){ std::cout << "std::vector<int>\n"; });
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
	TEST(test_type_switch);
	std::cout << std::flush;
	exit(0);
}