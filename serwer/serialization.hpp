#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

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

template<typename T,
         typename OutputIterator,
         typename = typename std::enable_if<std::is_enum<T>::value>::type,
         SfinaeDisambiguator<1> = nullptr>
void serialize_to(OutputIterator output, T data)
{
	serialize_to(output, static_cast<typename std::underlying_type<T>::type>(data));
}

#endif