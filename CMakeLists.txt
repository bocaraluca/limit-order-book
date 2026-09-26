cmake_minimum_required(VERSION 3.10)

project(MatchingEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(MSVC)
    add_compile_options(/O2)
else()
    add_compile_options(-O3)
endif()

add_library(order_book STATIC
    src/OrderBook.cpp
)

target_include_directories(order_book PUBLIC include)

add_executable(matching_engine src/main.cpp)
target_link_libraries(matching_engine PRIVATE order_book)

enable_testing()

add_executable(order_book_tests tests/OrderBookTests.cpp)
target_link_libraries(order_book_tests PRIVATE order_book)
add_test(NAME order_book_tests COMMAND order_book_tests)