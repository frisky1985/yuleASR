# Tests for Hash Algorithms

add_executable(TestHashAlgos
    test_hash.c
)

target_link_libraries(TestHashAlgos
    HashAlgos
)

# Add test
add_test(NAME HashAlgosTest COMMAND TestHashAlgos)
