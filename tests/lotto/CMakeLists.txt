include_directories( ${CMAKE_SOURCE_DIR}/libraries/lotto/include )

add_executable( lotto_chain_tests lotto_chain_tests.cpp )
target_link_libraries( lotto_chain_tests bts_lotto bts_wallet bts_blockchain bitcoin_import fc ${BOOST_LIBRARIES} ${OPENSSL_LIBRARIES} ${PLATFORM_SPECIFIC_LIBS} ${crypto_library})

add_executable( lotto_rule_tests lotto_rule_tests.cpp )
target_link_libraries( lotto_rule_tests bts_lotto bts_wallet bts_blockchain bitcoin_import fc ${BOOST_LIBRARIES} ${OPENSSL_LIBRARIES} ${PLATFORM_SPECIFIC_LIBS} ${crypto_library})
