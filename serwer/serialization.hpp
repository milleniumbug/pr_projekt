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
         typename = typename std::enable_if<std::is_arithmetic<T>::value>::type,
         SfinaeDisambiguator<0> = nullptr>
void serialize_to(std::vector<char>& buffer, T data)
{
	T changed = endian_change(data, Endian::native, Endian::network);
	std::copy(reinterpret_cast<const char*>(&changed), 
	          reinterpret_cast<const char*>(&changed) + sizeof(T),
	          std::back_inserter(buffer));
}

template<typename T,
         typename = typename std::enable_if<std::is_enum<T>::value>::type,
         SfinaeDisambiguator<1> = nullptr>
void serialize_to(std::vector<char>& buffer, T data)
{
	serialize_to(buffer, static_cast<typename std::underlying_type<T>::type>(data));
}

template<typename RandomAccessIterator>
std::vector<char> skonstruuj_odpowiedz(RandomAccessIterator begin, RandomAccessIterator end, IPv4Address ip, int port)
{
	RandomAccessIterator next_end = begin+sizeof(wersja_serwera);
	std::uint32_t wersja_klienta;
	std::copy(begin, next_end, reinterpret_cast<char*>(&wersja_klienta));
	wersja_klienta = endian_change(wersja_klienta, Endian::network, Endian::native);

	std::vector<char> odpowiedz;
	serialize_to(odpowiedz, wersja_serwera);
	if(wersja_klienta != wersja_serwera)
	{
		serialize_to(odpowiedz, RodzajKomunikatu::niekompatybilna_wersja);
		return odpowiedz;
	}
	return odpowiedz;
}

#endif