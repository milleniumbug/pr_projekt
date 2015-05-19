#include "precomp.hpp"
#include "netutils.hpp"
#include "game.hpp"
#include "serialization.hpp"

static_assert(CHAR_BIT == 8, "char musi mieć 8 bitów");
const std::uint32_t wersja_serwera = 1;

//const struct timespec logical_tick_time = { 0, static_cast<unsigned long>(1E+9 / ticks_in_a_second) };
const struct timespec logical_tick_time = { 5, 0 }; // do testów

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

template<typename InputIterator>
void debug_output_as_hex(std::ostream& out, InputIterator begin, InputIterator end)
{
	static const char hexchars[] = "0123456789ABCDEF";
	std::for_each(begin, end, [&](char s)
	{
		auto c = reinterpret_cast<unsigned char&>(s);
		out.put(hexchars[c / 16]);
		out.put(hexchars[c % 16]);
		out.put(' ');
	});
}

int main()
{
	// for select
	int max_fds = 0;
	fd_set do_odczytu;
	FD_ZERO(&do_odczytu);

	// timer initialization
	FileDescriptor timer(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK));
	const struct itimerspec timeout = { logical_tick_time, logical_tick_time };
	timerfd_settime(timer, 0, &timeout, nullptr);
	FD_SET(timer.fd(), &do_odczytu);
	max_fds = std::max(timer.fd(), max_fds);

	// UDP Socket
	UDPSocket socket(60000);
	FD_SET(socket.fd(), &do_odczytu);
	max_fds = std::max(socket.fd(), max_fds);

	std::vector<char> input_buffer(1024);
	while(true)
	{
		fd_set rfds = do_odczytu;

		if(select(max_fds+1, &rfds, nullptr, nullptr, nullptr) <= 0)
			assert(false); // wth
		
		if(FD_ISSET(timer.fd(), &rfds))
		{
			std::uint64_t ticks;
			read(timer, &ticks, sizeof ticks);

			std::cout << "LOGIKA GRY! Przegapionych update'ow: " << ticks-1 << "\n" << std::flush;
		}

		if(FD_ISSET(socket.fd(), &rfds))
		{
			char* begin = input_buffer.data();
			char* end = begin+input_buffer.size()-1;
			IPv4Address source(0,0,0,0);
			int port;
			end = socket.receive(begin, end, source, port);
			std::cout << "ODEBRANO: ";
			debug_output_as_hex(std::cout, begin, end);
			std::cout << "\n" << std::flush;

			std::vector<char> response = skonstruuj_odpowiedz(begin, end, source, port);
			socket.send(response.data(), response.data()+response.size(), source, port);
			std::cout << "WYSLANO: ";
			debug_output_as_hex(std::cout, response.begin(), response.end());
			std::cout << "\n" << std::flush;
		}
	}
}