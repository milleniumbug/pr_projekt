#include "precomp.hpp"
#include "netutils.hpp"
#include "game.hpp"
#include "serialization.hpp"

static_assert(CHAR_BIT == 8, "char musi mieć 8 bitów");
const std::uint32_t wersja_serwera = 1;

//const struct timespec logical_tick_time = { 0, static_cast<unsigned long>(1E+9 / ticks_in_a_second) };
const struct timespec logical_tick_time = { 5, 0 }; // do testów

#define OVERLOAD_SET(name) ([&](auto&&... args) -> decltype(auto) { return name(std::forward<decltype(args)>(args)...); })

enum class RodzajKomunikatu : unsigned char
{
	przylacz_sie = 0x01,
	zaakceptowanie = 0x02,
	przeslij_stan_gry = 0x03,
	przeslanie_stanu_gry = 0x04,
	przeslij_info_o_serwerze = 0x05,
	info_o_serwerze = 0x06,
	keep_alive = 0x07,
	zakoncz = 0x08,
	akcja_gracza = 0x09,
	nieznany_komunikat = 0x71,
	niekompatybilna_wersja = 0x72,
	gra_juz_rozpoczeta = 0x73,
	serwer_pelny = 0x74
};

enum class StanGry : unsigned char
{
	oczekiwanie_na_polaczenia = 0x01,
	gra_w_trakcie = 0x02
};

template<typename RandomAccessIterator, typename Function>
std::vector<char> skonstruuj_odpowiedz(RandomAccessIterator begin, RandomAccessIterator end, IPv4Address ip, int port, Function obsluzenie_pozostalych_przypadkow)
{
	try
	{
		std::vector<char> odpowiedz;
		auto output = std::back_inserter(odpowiedz);
		serialize_to(output, wersja_serwera);

		std::uint32_t wersja_klienta;
		std::tie(wersja_klienta, begin) = deserialize_from<decltype(wersja_klienta)>(begin, end);

		if(wersja_klienta != wersja_serwera)
		{
			serialize_to(output, RodzajKomunikatu::niekompatybilna_wersja);
			return odpowiedz;
		}

		RodzajKomunikatu komunikat;
		std::tie(komunikat, begin) = deserialize_from<decltype(komunikat)>(begin, end);
		obsluzenie_pozostalych_przypadkow(output, begin, end, ip, port, komunikat);
		return odpowiedz;
	}
	catch(const BufferTooShort& ex)
	{
		std::vector<char> odpowiedz;
		auto output = std::back_inserter(odpowiedz);
		serialize_to(output, wersja_serwera);

		serialize_to(output, RodzajKomunikatu::nieznany_komunikat);
		std::string komunikat = "Błędne zapytanie";
		std::copy(komunikat.begin(), komunikat.end(), output);
		*output++ = '\0';
		return odpowiedz;
	}
}

struct PlayerConnection
{
	IPv4Address ip;
	int port;
	Player* player;
	int timeout;
};

const int max_timeout_time = ticks_in_a_second*10;

template<typename OutputIterator, typename RandomAccessIterator, typename Connections>
void odpowiedz_stan_serwera(OutputIterator output, RandomAccessIterator begin, RandomAccessIterator end, Connections& conns, BombermanGame& game, StanGry stan_gry)
{
	serialize_to(output, static_cast<uint8_t>(conns.size()));
	serialize_to(output, static_cast<uint8_t>(game.players.size()));
	serialize_to(output, stan_gry);
	serialize_to(output, static_cast<uint16_t>(game.current_level.width()));
	serialize_to(output, static_cast<uint16_t>(game.current_level.height()));
}

template<typename OutputIterator, typename RandomAccessIterator, typename Connections, typename GameStarter>
void odpowiedz_lobby(OutputIterator output, RandomAccessIterator begin, RandomAccessIterator end, IPv4Address ip, int port, RodzajKomunikatu komunikat, Connections& conns, BombermanGame& game, GameStarter rozpocznij_gre)
{
	if(komunikat == RodzajKomunikatu::przylacz_sie)
	{
		if(conns.size() < game.players.size())
		{
			auto pos = conns.size();
			PlayerConnection conn;
			conn.ip = ip;
			conn.port = port;
			conn.player = &game.players[pos];
			conn.timeout = ticks_in_a_second*10;
			conns.push_back(conn);
			serialize_to(output, RodzajKomunikatu::zaakceptowanie);
			odpowiedz_stan_serwera(output, begin, end, conns, game, StanGry::oczekiwanie_na_polaczenia);

			if(conns.size() == game.players.size())
				rozpocznij_gre();
		}
		else
		{
			serialize_to(output, RodzajKomunikatu::serwer_pelny);
			return;
		}
	}
	else if(komunikat == RodzajKomunikatu::przeslij_info_o_serwerze)
	{
		serialize_to(output, RodzajKomunikatu::info_o_serwerze);
		odpowiedz_stan_serwera(output, begin, end, conns, game, StanGry::oczekiwanie_na_polaczenia);
	}
}

template<typename OutputIterator, typename RandomAccessIterator, typename Connections>
void odpowiedz_gra_w_toku(OutputIterator output, RandomAccessIterator begin, RandomAccessIterator end, IPv4Address ip, int port, RodzajKomunikatu komunikat, Connections& conns, BombermanGame& game)
{
	auto gracz_mysli_ze_jest_polaczony = [&]()
	{
		serialize_to(output, RodzajKomunikatu::nieznany_komunikat);
		std::string komunikat = "Gracz nie ";
		std::copy(komunikat.begin(), komunikat.end(), output);
		*output++ = '\0';
	};
	auto czy_to_ten_gracz = [&](PlayerConnection& c)
	{
		return c.ip == ip && c.port == port;
	};


	if(komunikat == RodzajKomunikatu::przylacz_sie)
	{
		serialize_to(output, RodzajKomunikatu::gra_juz_rozpoczeta);
		return;
	}
	else if(komunikat == RodzajKomunikatu::przeslij_stan_gry)
	{
		serialize_to(output, RodzajKomunikatu::przeslanie_stanu_gry);
		serialize_to(output, game);
	}
	else if(komunikat == RodzajKomunikatu::przeslij_info_o_serwerze)
	{
		serialize_to(output, RodzajKomunikatu::info_o_serwerze);
		odpowiedz_stan_serwera(output, begin, end, conns, game, StanGry::gra_w_trakcie);
	}
	else if(komunikat == RodzajKomunikatu::keep_alive)
	{
		auto it = std::find_if(conns.begin(), conns.end(), czy_to_ten_gracz);
		if(it != conns.end())
			it->timeout = max_timeout_time;
		else
			gracz_mysli_ze_jest_polaczony();
	}
	else if(komunikat == RodzajKomunikatu::zakoncz)
	{
		auto it = std::find_if(conns.begin(), conns.end(), czy_to_ten_gracz);
		if(it != conns.end())
		{
			// TODO: usprawnij może jakoś wychodzenie
			it->player->hurt();
		}
		else
			gracz_mysli_ze_jest_polaczony();
	}
	else if(komunikat == RodzajKomunikatu::akcja_gracza)
	{
		int8_t klawisze;
		std::tie(klawisze, begin) = deserialize_from<decltype(klawisze)>(begin, end);
		auto it = std::find_if(conns.begin(), conns.end(), czy_to_ten_gracz);
		if(it != conns.end())
		{
			// TODO
			//it->player->set_next_input(?)
		}
		else
			gracz_mysli_ze_jest_polaczony();
	}
	else
	{
		serialize_to(output, RodzajKomunikatu::nieznany_komunikat);
		std::string komunikat = "Błędne zapytanie";
		std::copy(komunikat.begin(), komunikat.end(), output);
		*output++ = '\0';
	}
}

template<typename InputIterator>
void debug_output_as_hex(std::ostream& out, InputIterator begin, InputIterator end)
{
	static const char hexchars[] = "0123456789ABCDEF";
	static const int hexbase = 16;
	std::for_each(begin, end, [&](char s)
	{
		auto c = reinterpret_cast<unsigned char&>(s);
		out.put(hexchars[c / hexbase]);
		out.put(hexchars[c % hexbase]);
		out.put(' ');
	});
}

template<typename InputIterator>
void debug_output(std::ostream& out, InputIterator begin, InputIterator end)
{
	static const char hexchars[] = "0123456789ABCDEF";
	static const int hexbase = 16;
	std::for_each(begin, end, [&](char s)
	{
		auto c = reinterpret_cast<unsigned char&>(s);
		if(!isgraph(c))
		{
			out.put(hexchars[c / hexbase]);
			out.put(hexchars[c % hexbase]);
			out.put(' ');
		}
		else
		{
			out.put(s);
			out.put(' ');
		}
	});
}

struct Blackhole
{
	template<typename... Args>
	void operator()(Args&&... args)
	{

	}
};

int main()
{
	// for select
	int max_fds = 0;
	fd_set do_odczytu;
	FD_ZERO(&do_odczytu);

	// timer initialization
	FileDescriptor timer(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK));
	FD_SET(timer.fd(), &do_odczytu);
	max_fds = std::max(timer.fd(), max_fds);

	// UDP Socket
	UDPSocket socket(60000);
	FD_SET(socket.fd(), &do_odczytu);
	max_fds = std::max(socket.fd(), max_fds);

	std::vector<char> input_buffer(65536);
	std::vector<PlayerConnection> connections;
	bool gra_w_toku = false;
	long long tick_number = 0;
	BombermanLevel default_level(13, 13);
	BombermanGame world(default_level);
	world.players.resize(4);

	auto rozpocznij_gre = [&]()
	{
		gra_w_toku = true;
		tick_number = 0;
		const struct itimerspec timeout = { logical_tick_time, logical_tick_time };
		timerfd_settime(timer, 0, &timeout, nullptr);
	};

	auto przerwij_gre = [&]()
	{
		gra_w_toku = false;
		tick_number = 0;
		const struct itimerspec timeout = { 0 };
		timerfd_settime(timer, 0, nullptr, nullptr);
	};
	while(true)
	{
		fd_set rfds = do_odczytu;

		if(select(max_fds+1, &rfds, nullptr, nullptr, nullptr) <= 0)
			assert(false); // wth
		
		if(FD_ISSET(timer.fd(), &rfds))
		{
			std::uint64_t ticks;
			read(timer, &ticks, sizeof ticks);
			++tick_number;

			std::cout << "LOGIKA GRY! Przegapionych update'ow: " << ticks-1 << ", tick numer: " << tick_number << "\r" << std::flush;
			world.refresh();
		}

		if(FD_ISSET(socket.fd(), &rfds))
		{
			char* begin = input_buffer.data();
			char* end = begin+input_buffer.size()-1;
			IPv4Address source;
			int port;
			end = socket.receive(begin, end, source, port);
			std::cout << "\nODEBRANO:\n";
			debug_output(std::cout, begin, end);
			std::cout << "\n";
			debug_output_as_hex(std::cout, begin, end);
			std::cout << "\n" << std::flush;

			std::vector<char> response;
			if(gra_w_toku)
			{
				using namespace std::placeholders;
				auto handler = std::bind(
					OVERLOAD_SET(odpowiedz_gra_w_toku),
					_1, _2, _3, _4, _5, _6,
					std::ref(connections),
					std::ref(world));
				response = skonstruuj_odpowiedz(begin, end, source, port, handler);
			}
			else
			{
				using namespace std::placeholders;
				auto handler = std::bind(
					OVERLOAD_SET(odpowiedz_lobby),
					_1, _2, _3, _4, _5, _6,
					std::ref(connections),
					std::ref(world),
					std::ref(rozpocznij_gre));
				response = skonstruuj_odpowiedz(begin, end, source, port, handler);
			}
			socket.send(response.data(), response.data()+response.size(), source, port);
			std::cout << "WYSLANO:\n";
			debug_output(std::cout, response.begin(), response.end());
			std::cout << "\n";
			debug_output_as_hex(std::cout, response.begin(), response.end());
			std::cout << "\n" << std::flush;
		}
	}
}