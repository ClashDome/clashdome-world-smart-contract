project(clashdomewld)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( clashdomewld clashdomewld clashdomewld.cpp )
target_include_directories( clashdomewld PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( clashdomewld ${CMAKE_SOURCE_DIR}/../ricardian )