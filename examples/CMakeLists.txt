# Examples for SAPPOROBDD2
# Based on bdd-benchmark (MIT License) by Steffan Soelvsten
# https://github.com/ssoelvsten/bdd-benchmark

# N-Queens Problem Solver
add_executable(nqueens nqueens.cpp)
target_link_libraries(nqueens PRIVATE sbdd2)
target_include_directories(nqueens PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 4x4x4 Tic-Tac-Toe Draw Counter
add_executable(tictactoe tictactoe.cpp)
target_link_libraries(tictactoe PRIVATE sbdd2)
target_include_directories(tictactoe PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Hamiltonian Cycle Counter (Grid Graph)
add_executable(hamiltonian hamiltonian.cpp)
target_link_libraries(hamiltonian PRIVATE sbdd2)
target_include_directories(hamiltonian PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Game of Life - Garden of Eden Counter
add_executable(gameoflife gameoflife.cpp)
target_link_libraries(gameoflife PRIVATE sbdd2)
target_include_directories(gameoflife PRIVATE ${CMAKE_SOURCE_DIR}/include)

# CNF SAT Solver
add_executable(cnf cnf.cpp)
target_link_libraries(cnf PRIVATE sbdd2)
target_include_directories(cnf PRIVATE ${CMAKE_SOURCE_DIR}/include)

# BDD Apply Benchmark (lib_bdd format)
add_executable(apply apply.cpp)
target_link_libraries(apply PRIVATE sbdd2)
target_include_directories(apply PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Relational Product Benchmark (lib_bdd format)
add_executable(relprod relprod.cpp)
target_link_libraries(relprod PRIVATE sbdd2)
target_include_directories(relprod PRIVATE ${CMAKE_SOURCE_DIR}/include)
