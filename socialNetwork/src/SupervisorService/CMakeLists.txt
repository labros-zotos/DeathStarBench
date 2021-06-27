add_executable(
    SupervisorService
    SupervisorService.cpp
)

target_include_directories(
    SupervisorService PRIVATE
)

target_link_libraries(
    SupervisorService
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    Boost::system
    Boost::thread
    Boost::log
    Boost::log_setup
    ${PQXX_LIB}
    ${PQ_LIB}
)

install(TARGETS SupervisorService DESTINATION ./)