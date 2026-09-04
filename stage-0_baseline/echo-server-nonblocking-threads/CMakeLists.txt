add_executable(echo-server-nonblocking-threads
	main.cpp
)

target_include_directories(echo-server-nonblocking-threads
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.
)

target_link_libraries(echo-server-nonblocking-threads
	PRIVATE common
)
