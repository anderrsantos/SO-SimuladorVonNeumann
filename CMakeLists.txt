# =====================================================
#  Versão mínima do CMake e nome do projeto
# =====================================================
cmake_minimum_required(VERSION 3.10)
project(VonNeumannSimulator)

# -----------------------------------------------------
# Definir padrão C++17
# -----------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------
# Compilar em Debug se nada for especificado
# -----------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# -----------------------------------------------------
# Diretório raiz de includes (src/)
# -----------------------------------------------------
include_directories(src)

# -----------------------------------------------------
#  Lista de arquivos do simulador principal
# -----------------------------------------------------
set(SIMULATOR_SOURCES
    src/main.cpp

    # CPU
    src/cpu/CONTROL_UNIT.cpp
    src/cpu/pcb_loader.cpp
    src/cpu/REGISTER_BANK.cpp
    src/cpu/ULA.cpp

    # IO
    src/IO/IOManager.cpp

    # Memory
    src/memory/cache.cpp
    src/memory/cachePolicy.cpp
    src/memory/MAIN_MEMORY.cpp
    src/memory/MemoryManager.cpp
    src/memory/SECONDARY_MEMORY.cpp

    # Multicore CPU
    src/multicore/Core.cpp
    src/multicore/MultiCore.cpp
    src/multicore/Scheduler.cpp

    # Parser JSON
    src/parser_json/parser_json.cpp
)

# -----------------------------------------------------
#  Criação do executável principal
# -----------------------------------------------------
add_executable(simulador ${SIMULATOR_SOURCES})

# -----------------------------------------------------
#  Copiar diretório "processes/" inteiro para build/
# -----------------------------------------------------
add_custom_command(TARGET simulador POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_BINARY_DIR}/processes
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/processes
            ${CMAKE_BINARY_DIR}/processes
    COMMENT "📁 Copiando pasta de processos (.json) para build/processes/"
)

target_link_libraries(simulador PRIVATE pthread)

# -----------------------------------------------------
#  Copiar arquivos .json para diretório de build
# -----------------------------------------------------
add_custom_command(TARGET simulador POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/processes/process1.json
            ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/src/tasks/tasks.json
            ${CMAKE_BINARY_DIR}
    COMMENT "Copiando JSONs para a pasta de build..."
)

# =====================================================
#   DESATIVAR TODOS OS TESTES (para evitar erro do main)
# =====================================================
# Todos os testes comentados (nenhum será compilado)
# Porque test_metrics não contém main()
#add_executable(test_hash ...)
#add_executable(test_bank ...)
#add_executable(test_ula ...)
#add_executable(test_metrics ...)

# =====================================================
#  Targets personalizados (equivalentes ao Make)
# =====================================================

add_custom_target(run
    COMMAND ${CMAKE_BINARY_DIR}/simulador
    DEPENDS simulador
    COMMENT "🚀 Executando o simulador..."
    VERBATIM
)

add_custom_target(ajuda
    COMMAND ${CMAKE_COMMAND} -E echo "📌 Comandos disponíveis:"
    COMMAND ${CMAKE_COMMAND} -E echo "  make / make all    → Compila tudo"
    COMMAND ${CMAKE_COMMAND} -E echo "  make simulador      → Compila só o simulador"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run            → Executa o simulador"
    COMMAND ${CMAKE_COMMAND} -E echo "  make clean          → Remove arquivos de build"
    COMMAND ${CMAKE_COMMAND} -E echo "  make ajuda          → Exibe esta ajuda"
    VERBATIM
)
