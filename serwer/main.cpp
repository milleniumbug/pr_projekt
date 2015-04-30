#include "precomp.hpp"
#include "netutils.hpp"

//const struct timespec logical_tick_time = { 0, 8333333 }; // 1/120 sekundy
//const struct timespec logical_tick_time = { 2, 0 }; // do test

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

	//FD_SET(0, &do_odczytu);

	// UDP Socket
	UDPSocket socket(60000);
	FD_SET(socket.fd(), &do_odczytu);
	max_fds = std::max(socket.fd(), max_fds);

	std::vector<char> input_buffer(1024), output_buffer(1024);
	while(true)
	{
		fd_set rfds = do_odczytu;

		if(select(max_fds+1, &rfds, nullptr, nullptr, nullptr) <= 0)
			assert(false); // wth
		
		if(FD_ISSET(timer.fd(), &rfds))
		{
			std::uint64_t ticks;
			read(timer, &ticks, sizeof ticks);

			std::cout << "LOGIKA GRY!" << ticks << "\n" << std::flush;
		}

		if(FD_ISSET(socket.fd(), &rfds))
		{
			char* begin = input_buffer.data();
			char* end = begin+input_buffer.size();
			IPv4Address source(0,0,0,0);
			end = socket.receive(begin, end, source);
			*(end+1) = '\0';
			std::cout << "COS TU JEST! " << begin << "\n" << std::flush;
		}

		if(FD_ISSET(0, &rfds))
		{
			char a;
			std::cout << "stdin\n" << std::flush;
			std::cin >> a;
		}
	}
}