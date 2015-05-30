#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include <iterator>
#include <algorithm>
#include <utility>
#include <stdexcept>

enum class Endian
{
	big,
	little,
	native =
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	Endian::little
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	Endian::big
#else
#error "Unknown endian or architecture not supported"
#endif
	,
	network = Endian::big
};

template<typename T>
T endian_change(T value, Endian from, Endian to)
{
	if(to != from)
	{
		char* begin = reinterpret_cast<char*>(&value);
		char* end = begin + sizeof(T);
		std::reverse(begin, end);
	}
	return value;
}

template<int N>
struct SfinaeDisambiguatorImpl
{
	using type = SfinaeDisambiguatorImpl*;
};

template<int N>
using SfinaeDisambiguator = typename SfinaeDisambiguatorImpl<N>::type;

template<typename T,
         typename OutputIterator,
         typename = typename std::enable_if<std::is_arithmetic<T>::value>::type,
         SfinaeDisambiguator<0> = nullptr>
void serialize_to(OutputIterator output, T data)
{
	T changed = endian_change(data, Endian::native, Endian::network);
	std::copy(reinterpret_cast<const char*>(&changed), 
	          reinterpret_cast<const char*>(&changed) + sizeof(T),
	          output);
}

struct BufferTooShort : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

template<typename T,
         typename OutputIterator,
         typename = typename std::enable_if<std::is_enum<T>::value>::type,
         SfinaeDisambiguator<1> = nullptr>
void serialize_to(OutputIterator output, T data)
{
	serialize_to(output, static_cast<typename std::underlying_type<T>::type>(data));
}

template<typename T,
         typename InputIterator,
         typename = typename std::enable_if<std::is_arithmetic<T>::value>::type,
         SfinaeDisambiguator<0> = nullptr>
std::pair<T, InputIterator> deserialize_from(InputIterator begin, InputIterator end)
{
	std::pair<T, InputIterator> ret(T(), begin);
	char* output = reinterpret_cast<char*>(&ret.first);
	std::size_t i = 0;
	for(; i < sizeof(T) && begin != end; ++i)
	{
		*output++ = *begin++;
	}
	if(i != sizeof(T) && begin == end)
		throw BufferTooShort("funkcja napotkala przedwczesny koniec bufora");
	ret.first = endian_change(ret.first, Endian::network, Endian::native);
	ret.second = begin;
	return ret;
}

template<typename T,
         typename InputIterator,
         typename = typename std::enable_if<std::is_enum<T>::value>::type,
         SfinaeDisambiguator<1> = nullptr>
std::pair<T, InputIterator> deserialize_from(InputIterator begin, InputIterator end)
{
	auto retval = deserialize_from<typename std::underlying_type<T>::type>(begin, end);
	return std::pair<T, InputIterator>(static_cast<T>(retval.first), retval.second);
}

#endif