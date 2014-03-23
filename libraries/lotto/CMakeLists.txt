include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/include" )
include_directories( "${CMAKE_SOURCE_DIR}/libraries/db/include" )
include_directories( "${CMAKE_SOURCE_DIR}/libraries/blockchain/include" )
include_directories( "${CMAKE_SOURCE_DIR}/libraries/wallet/include" )

add_library( bts_lotto
             lotto_outputs.cpp
             lotto_db.cpp
             lotto_transaction_validator.cpp
             lotto_wallet.cpp
           )


target_link_libraries( bts_lotto bts_blockchain fc bts_db leveldb )
