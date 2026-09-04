add_executable(echo-server-blocking
	main.cpp
)

target_include_directories(echo-server-blocking
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.
)

target_link_libraries(echo-server-blocking
	PRIVATE common
)
