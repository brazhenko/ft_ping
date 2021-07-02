cmake_minimum_required(VERSION 3.1)
project(ft_ping C)

set(CMAKE_C_STANDARD 99)

add_executable(ft_ping main.c ping.h initialize_context.c initialize_signals.c)

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(ft_ping PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(ft_ping "${CMAKE_THREAD_LIBS_INIT}")
endif()