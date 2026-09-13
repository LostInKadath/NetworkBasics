add_executable(echo-server-nonblocking-epoll
	main.cpp
)

target_include_directories(echo-server-nonblocking-epoll
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.
)

target_link_libraries(echo-server-nonblocking-epoll
	PRIVATE common
)
