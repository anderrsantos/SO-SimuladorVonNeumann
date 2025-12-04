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
#   TESTES PRIORITÁRIOS
# =====================================================

# Teste do Escalonador
add_executable(test_scheduler_priority
    src/test/test_scheduler_priority.cpp
    src/multicore/Scheduler.cpp
    src/cpu/REGISTER_BANK.cpp
)
target_include_directories(test_scheduler_priority PRIVATE src)

# Teste de Memória
add_executable(test_memory_critical
    src/test/test_memory_critical.cpp
    src/memory/MemoryManager.cpp
    src/memory/MAIN_MEMORY.cpp
    src/memory/SECONDARY_MEMORY.cpp
    src/memory/cache.cpp
    src/memory/cachePolicy.cpp
    src/cpu/REGISTER_BANK.cpp
)
target_include_directories(test_memory_critical PRIVATE src)

# Teste do Pipeline
add_executable(test_pipeline_basic
    src/test/test_pipeline_basic.cpp
    src/multicore/Core.cpp
    src/multicore/MultiCore.cpp
    src/multicore/Scheduler.cpp
    src/memory/MemoryManager.cpp
    src/memory/MAIN_MEMORY.cpp
    src/memory/SECONDARY_MEMORY.cpp
    src/memory/cache.cpp
    src/memory/cachePolicy.cpp
    src/IO/IOManager.cpp
    src/cpu/CONTROL_UNIT.cpp
    src/cpu/pcb_loader.cpp
    src/cpu/REGISTER_BANK.cpp
    src/cpu/ULA.cpp
    src/parser_json/parser_json.cpp
)
target_include_directories(test_pipeline_basic PRIVATE src)
target_link_libraries(test_pipeline_basic PRIVATE pthread)

# Teste de Integração
add_executable(test_integration_complete
    src/test/test_integration_complete.cpp
    src/multicore/Core.cpp
    src/multicore/MultiCore.cpp
    src/multicore/Scheduler.cpp
    src/memory/MemoryManager.cpp
    src/memory/MAIN_MEMORY.cpp
    src/memory/SECONDARY_MEMORY.cpp
    src/memory/cache.cpp
    src/memory/cachePolicy.cpp
    src/IO/IOManager.cpp
    src/cpu/CONTROL_UNIT.cpp
    src/cpu/pcb_loader.cpp
    src/cpu/REGISTER_BANK.cpp
    src/cpu/ULA.cpp
    src/parser_json/parser_json.cpp
)
target_include_directories(test_integration_complete PRIVATE src)
target_link_libraries(test_integration_complete PRIVATE pthread)

# Teste de Casos de Borda
add_executable(test_edge_cases
    src/test/test_edge_cases.cpp
    src/multicore/Scheduler.cpp
    src/memory/MemoryManager.cpp
    src/memory/MAIN_MEMORY.cpp
    src/memory/SECONDARY_MEMORY.cpp
    src/memory/cache.cpp
    src/memory/cachePolicy.cpp
    src/cpu/REGISTER_BANK.cpp
)
target_include_directories(test_edge_cases PRIVATE src)

# Teste de Desempenho
add_executable(test_performance
    src/test/test_performance.cpp
    src/multicore/Core.cpp
    src/multicore/MultiCore.cpp
    src/multicore/Scheduler.cpp
    src/memory/MemoryManager.cpp
    src/memory/MAIN_MEMORY.cpp
    src/memory/SECONDARY_MEMORY.cpp
    src/memory/cache.cpp
    src/memory/cachePolicy.cpp
    src/IO/IOManager.cpp
    src/cpu/CONTROL_UNIT.cpp
    src/cpu/pcb_loader.cpp
    src/cpu/REGISTER_BANK.cpp
    src/cpu/ULA.cpp
    src/parser_json/parser_json.cpp
)
target_include_directories(test_performance PRIVATE src)
target_link_libraries(test_performance PRIVATE pthread)

# Teste de Stress
add_executable(test_stress
    src/test/test_stress.cpp
    src/multicore/Core.cpp
    src/multicore/MultiCore.cpp
    src/multicore/Scheduler.cpp
    src/memory/MemoryManager.cpp
    src/memory/MAIN_MEMORY.cpp
    src/memory/SECONDARY_MEMORY.cpp
    src/memory/cache.cpp
    src/memory/cachePolicy.cpp
    src/IO/IOManager.cpp
    src/cpu/CONTROL_UNIT.cpp
    src/cpu/pcb_loader.cpp
    src/cpu/REGISTER_BANK.cpp
    src/cpu/ULA.cpp
    src/parser_json/parser_json.cpp
)
target_include_directories(test_stress PRIVATE src)
target_link_libraries(test_stress PRIVATE pthread)

# Teste de I/O Detalhado
add_executable(test_io_detailed
    src/test/test_io_detailed.cpp
    src/IO/IOManager.cpp
    src/cpu/REGISTER_BANK.cpp
)
target_include_directories(test_io_detailed PRIVATE src)
target_link_libraries(test_io_detailed PRIVATE pthread)

# Teste de Métricas
add_executable(test_metrics
    src/test/test_metrics.cpp
    src/multicore/Core.cpp
    src/multicore/MultiCore.cpp
    src/multicore/Scheduler.cpp
    src/memory/MemoryManager.cpp
    src/memory/MAIN_MEMORY.cpp
    src/memory/SECONDARY_MEMORY.cpp
    src/memory/cache.cpp
    src/memory/cachePolicy.cpp
    src/IO/IOManager.cpp
    src/cpu/CONTROL_UNIT.cpp
    src/cpu/pcb_loader.cpp
    src/cpu/REGISTER_BANK.cpp
    src/cpu/ULA.cpp
    src/parser_json/parser_json.cpp
)
target_include_directories(test_metrics PRIVATE src)
target_link_libraries(test_metrics PRIVATE pthread)

# Target para executar todos os testes
add_custom_target(test-all
    COMMAND ${CMAKE_COMMAND} -E echo "=== Executando Testes Prioritários ==="
    COMMAND ${CMAKE_BINARY_DIR}/test_scheduler_priority || true
    COMMAND ${CMAKE_BINARY_DIR}/test_memory_critical || true
    COMMAND ${CMAKE_BINARY_DIR}/test_pipeline_basic || true
    COMMAND ${CMAKE_BINARY_DIR}/test_integration_complete || true
    COMMAND ${CMAKE_BINARY_DIR}/test_edge_cases || true
    COMMAND ${CMAKE_COMMAND} -E echo "=== Executando Testes Adicionais ==="
    COMMAND ${CMAKE_BINARY_DIR}/test_performance || true
    COMMAND ${CMAKE_BINARY_DIR}/test_stress || true
    COMMAND ${CMAKE_BINARY_DIR}/test_io_detailed || true
    COMMAND ${CMAKE_BINARY_DIR}/test_metrics || true
    DEPENDS test_scheduler_priority test_memory_critical test_pipeline_basic 
            test_integration_complete test_edge_cases test_performance 
            test_stress test_io_detailed test_metrics
    COMMENT "🧪 Executando todos os testes..."
    VERBATIM
)

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
    COMMAND ${CMAKE_COMMAND} -E echo "  make / make all              → Compila tudo"
    COMMAND ${CMAKE_COMMAND} -E echo "  make simulador               → Compila só o simulador"
    COMMAND ${CMAKE_COMMAND} -E echo "  make run                     → Executa o simulador"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test-all                → Executa todos os testes"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test_scheduler_priority → Teste do escalonador"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test_memory_critical    → Teste de memória"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test_pipeline_basic     → Teste do pipeline"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test_integration_complete → Teste de integração"
    COMMAND ${CMAKE_COMMAND} -E echo "  make test_edge_cases        → Teste de casos de borda"
    COMMAND ${CMAKE_COMMAND} -E echo "  make clean                   → Remove arquivos de build"
    COMMAND ${CMAKE_COMMAND} -E echo "  make ajuda                   → Exibe esta ajuda"
    VERBATIM
)
