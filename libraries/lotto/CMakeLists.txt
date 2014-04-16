include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/include" )
include_directories( "${CMAKE_SOURCE_DIR}/libraries/db/include" )
include_directories( "${CMAKE_SOURCE_DIR}/libraries/blockchain/include" )
include_directories( "${CMAKE_SOURCE_DIR}/libraries/wallet/include" )
include_directories( "${CMAKE_SOURCE_DIR}/libraries/net/include" )
include_directories( "${CMAKE_SOURCE_DIR}/libraries/client/include" )
include_directories( "${CMAKE_SOURCE_DIR}/libraries/cli/include" )

add_library( bts_lotto
             rule.cpp
             lotto_block.cpp
             lotto_outputs.cpp
             lotto_db.cpp
             lotto_transaction_validator.cpp
             lotto_rule_validator.cpp
             lotto_wallet.cpp
             lotto_cli.cpp
           )


target_link_libraries( bts_lotto bts_blockchain fc bts_cli bts_net bts_client bts_db leveldb )
