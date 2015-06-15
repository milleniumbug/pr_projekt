#include "precomp.hpp"
#include "netutils.hpp"
#include "game.hpp"
#include "serialization.hpp"

static_assert(CHAR_BIT == 8, "char musi mieć 8 bitów");
const std::uint32_t wersja_serwera = 1;

const struct timespec logical_tick_timespec = { 0, static_cast<unsigned long>(1E+9 / ticks_in_a_second) };
//const struct timespec logical_tick_timespec = { 0, static_cast<unsigned long>(1E+9 / 2) }; // do testów
const struct timeval logical_tick_timeval = { 0, static_cast<unsigned long>(1E+6 / ticks_in_a_second) };
//const struct timeval logical_tick_timeval = { 0, static_cast<unsigned long>(1E+6 / 2) }; // do testów

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

template<typename T, typename... Args>
T make(Args&&... args)
{
	return T(std::forward<Args>(args)...);
}

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

const int max_timeout_time = ticks_in_a_second*10;

struct PlayerConnection
{
	IPv4Address ip;
	int port;
	Player* player;
	int timeout;

	PlayerConnection(IPv4Address ip, int port, Player* player) :
		ip(ip),
		port(port),
		player(player),
		timeout(max_timeout_time)
	{

	}

	PlayerConnection() :
		PlayerConnection(IPv4Address(0,0,0,0), 0, nullptr)
	{

	}

};

bool czy_to_ten_gracz(const PlayerConnection& c, IPv4Address ip, int port)
{
	return c.ip == ip && c.port == port;
}

bool czy_jest_polaczony(const PlayerConnection& c)
{
	return c.player	!= nullptr;
};

template<typename OutputIterator, typename RandomAccessIterator, typename Connections>
void odpowiedz_stan_serwera(OutputIterator output, RandomAccessIterator begin, RandomAccessIterator end, IPv4Address ip, int port, Connections& conns, BombermanGame& game, StanGry stan_gry)
{
	auto get_connected_players_count = [&]()
	{
		return std::count_if(conns.begin(), conns.end(), czy_jest_polaczony);
	};

	serialize_to(output, static_cast<uint8_t>(get_connected_players_count()));
	serialize_to(output, static_cast<uint8_t>(game.players.size()));
	serialize_to(output, stan_gry);
	serialize_to(output, static_cast<uint16_t>(game.current_level.width()));
	serialize_to(output, static_cast<uint16_t>(game.current_level.height()));
	auto dobry_gracz = std::bind(czy_to_ten_gracz, std::placeholders::_1, ip, port);
	auto it = std::find_if(conns.begin(), conns.end(), dobry_gracz);
	if(it != conns.end())
		serialize_to(output, static_cast<uint8_t>(1+std::distance(conns.begin(), it)));
	else
		serialize_to(output, static_cast<uint8_t>(0));
}

template<typename OutputIterator, typename RandomAccessIterator, typename Connections, typename GameStarter>
void odpowiedz_lobby(OutputIterator output, RandomAccessIterator begin, RandomAccessIterator end, IPv4Address ip, int port, RodzajKomunikatu komunikat, Connections& conns, BombermanGame& game, GameStarter rozpocznij_gre)
{
	auto dobry_gracz = std::bind(czy_to_ten_gracz, std::placeholders::_1, ip, port);
	auto get_connected_players_count = [&]()
	{
		return std::count_if(conns.begin(), conns.end(), czy_jest_polaczony);
	};
	
	if(komunikat == RodzajKomunikatu::przylacz_sie)
	{
		
		if(get_connected_players_count() < game.players.size())
		{
			if(std::find_if(conns.begin(), conns.end(), dobry_gracz) == conns.end())
			{
				auto gr_it = std::find_if_not(conns.begin(), conns.end(), czy_jest_polaczony);
				auto pos = std::distance(conns.begin(), gr_it);
				PlayerConnection conn(ip, port, &game.players[pos]);
				*gr_it = conn;
				serialize_to(output, RodzajKomunikatu::zaakceptowanie);
				odpowiedz_stan_serwera(output, begin, end, ip, port, conns, game, StanGry::oczekiwanie_na_polaczenia);
			}
			else
			{
				serialize_to(output, RodzajKomunikatu::nieznany_komunikat);
				std::string komunikat = "Już jesteś połączony";
				std::copy(komunikat.begin(), komunikat.end(), output);
				*output++ = '\0';
			}
			if(get_connected_players_count() == game.players.size())
				rozpocznij_gre();
		}
		else
		{
			serialize_to(output, RodzajKomunikatu::serwer_pelny);
			return;
		}
	}
	else if(komunikat == RodzajKomunikatu::zakoncz)
	{
		auto gr_it = std::find_if(conns.begin(), conns.end(), dobry_gracz);
		if(gr_it != conns.end())
		{
			*gr_it = PlayerConnection();
		}
	}
	else if(komunikat == RodzajKomunikatu::przeslij_info_o_serwerze)
	{
		serialize_to(output, RodzajKomunikatu::info_o_serwerze);
		odpowiedz_stan_serwera(output, begin, end, ip, port, conns, game, StanGry::oczekiwanie_na_polaczenia);
	}
	else
	{
		serialize_to(output, RodzajKomunikatu::nieznany_komunikat);
		std::string komunikat = "Gra się nie rozpoczęła";
		std::copy(komunikat.begin(), komunikat.end(), output);
		*output++ = '\0';
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

	auto dobry_gracz = std::bind(czy_to_ten_gracz, std::placeholders::_1, ip, port);

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
		odpowiedz_stan_serwera(output, begin, end, ip, port, conns, game, StanGry::gra_w_trakcie);
	}
	else if(komunikat == RodzajKomunikatu::keep_alive)
	{
		auto it = std::find_if(conns.begin(), conns.end(), dobry_gracz);
		if(it != conns.end())
			it->timeout = max_timeout_time;
		else
			gracz_mysli_ze_jest_polaczony();
	}
	else if(komunikat == RodzajKomunikatu::zakoncz)
	{
		auto it = std::find_if(conns.begin(), conns.end(), dobry_gracz);
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
		auto it = std::find_if(conns.begin(), conns.end(), dobry_gracz);
		if(it != conns.end())
		{
			bool czy_klasc_bombe = false;
			// TODO
			if(!in_range(klawisze, -4, 4))
			{
				klawisze -= 16;
				if(in_range(klawisze, -4, 4))
					czy_klasc_bombe = true;
			}
			if(in_range(klawisze, -4, 4))
			{
				it->player->set_next_input(klawisze);
				if(czy_klasc_bombe)
					it->player->ustaw_bombe();
			}
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

BombermanGame create_default_world(int ile_graczy = 4, int round_time = 5 * seconds_in_a_minute * ticks_in_a_second)
{
	constexpr int w = 13, h = 13;
	BombermanLevel default_level(w, h);
	const char data[] = 
	"  B B B B B  "
	" X X X X X X "
	"B B B B B B B"
	" X X X X X X "
	"B B B B B B B"
	" X X X X X X "
	"B B B B B B B"
	" X X X X X X "
	"B B B B B B B"
	" X X X X X X "
	"B B B B B B B"
	" X X X X X X "
	"  B B B B B  ";
	constexpr std::size_t length = (sizeof data) - 1;
	static_assert(length == w*h, "popraw planszę");
	std::transform(std::begin(data), std::begin(data)+length, default_level.begin(), [](char c) -> Entity
	{
		if(c == ' ')
			return EmptySpace();
		else if(c == 'B')
			return DestructibleWall();
		else if(c == 'X')
			return NondestructibleWall();
		assert(false);
	});
	BombermanGame world(default_level, round_time);
	std::array<Point, 4> player_positions =
	{
		Point(0, 0),
		Point(w-1, h-1),
		Point(0, h-1),
		Point(w-1, 0)
	};
	assert(in_range(ile_graczy, BombermanGame::min_graczy, BombermanGame::max_graczy));
	std::transform(player_positions.begin(), player_positions.begin()+ile_graczy, std::back_inserter(world.players), OVERLOAD_SET(make<Player>));
	return world;
}

template<typename Duration>
struct timeval timeval_decrease(const struct timeval& time, Duration how_many)
{
	using namespace std::chrono;
	seconds sec(time.tv_sec);
	microseconds usec(time.tv_usec);
	auto advanced = sec + usec - how_many;
	struct timeval retval;
	retval.tv_sec = duration_cast<seconds>(advanced % seconds(1)).count();
	retval.tv_usec = duration_cast<microseconds>(advanced % microseconds(1)).count();
	return retval;
}

bool is_positive(const struct timeval& time)
{
	return (time.tv_sec > 0 && time.tv_usec > 0) || (time.tv_sec == 0 && time.tv_usec > 0) || (time.tv_sec > 0 && time.tv_usec == 0);
}

int main()
{
	initialize_networking();

	// for select
	int max_fds = 0;
	fd_set do_odczytu;
	FD_ZERO(&do_odczytu);
	
	typedef std::chrono::steady_clock clock;
	
	// UDP Socket
	UDPSocket socket(60000);
	FD_SET(socket.fd(), &do_odczytu);
	max_fds = std::max(socket.fd(), max_fds);

	const int ile_graczy = 4;
	std::vector<char> input_buffer(65536);
	std::vector<PlayerConnection> connections(ile_graczy);
	bool gra_w_toku = false;
	long long tick_number = 0;
	struct timeval remaining_time;
	struct timeval* remaining_time_p = nullptr;
	
	BombermanGame world = create_default_world();

	auto rozpocznij_gre = [&]()
	{
		gra_w_toku = true;
		tick_number = 0;
		remaining_time = logical_tick_timeval;
		remaining_time_p = &remaining_time;
	};

	auto przerwij_gre = [&]()
	{
		gra_w_toku = false;
		tick_number = 0;
		remaining_time_p = nullptr;
	};
	while(true)
	{
		fd_set rfds = do_odczytu;

		auto before = clock::now();
		const struct timeval time_before = remaining_time;
		if(select(max_fds+1, &rfds, nullptr, nullptr, remaining_time_p) < 0)
			assert(false); // wth

		auto after = clock::now();
		// można zakładać, że remaining_time ma nieokreśloną wartość
		// dla przenośności - ustaw ją ponownie
		remaining_time = timeval_decrease(time_before, after-before);

		if(!is_positive(remaining_time))
		{
			remaining_time = logical_tick_timeval;

			++tick_number;
			std::cout << "LOGIKA GRY!" << "\r" << std::flush;
			world.refresh();
			for(auto& playerconn : connections)
				--playerconn.timeout;
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