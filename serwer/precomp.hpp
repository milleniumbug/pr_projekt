#include <cstring>
#include <cerrno>
#include <cassert>
#include <utility>
#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
#include <unordered_set>
#include <algorithm>
#include <limits>
#include <climits>
#include <functional>
#include <iterator>
#include <cctype>
#include <type_traits>
#include <chrono>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/timerfd.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#include <WS2tcpip.h>
#endif

#include <unistd.h>
#include <fcntl.h>