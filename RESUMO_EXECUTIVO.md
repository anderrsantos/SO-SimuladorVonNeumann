# Resumo Executivo - Análise do Simulador de SO

## Componentes do Sistema Operacional Implementados

### 1. ESCALONADOR DE PROCESSOS (CPU Scheduler)
**Localização**: `src/multicore/Scheduler.hpp/cpp`

**Algoritmos Implementados**:
- **FCFS** (First Come First Served): Fila FIFO simples
- **Round-Robin**: Fila FIFO com quantum (preempção por tempo)
- **Priority**: Seleção por maior prioridade (max_element)
- **SJN** (Shortest Job Next): Seleção por menor burst_estimate (min_element)

**Estados Gerenciados**: Ready, Running, Blocked, Finished

---

### 2. GERENCIADOR DE MEMÓRIA (Memory Manager)
**Localização**: `src/memory/MemoryManager.hpp/cpp`

**Mecanismos Implementados**:
- **Partições Fixas**: Memória dividida em blocos de tamanho fixo (512 palavras)
- **Tradução de Endereços**: Lógico → Físico (base + offset)
- **Hierarquia de Memória**: Cache L1 → RAM → Disco

**Cache L1** (`src/memory/cache.hpp/cpp`):
- **Política de Substituição**: FIFO (First In First Out)
- **Política de Escrita**: Write-Back com No-Write-allocate
- **Estrutura**: Hash map para acesso O(1)

**Memórias**:
- **RAM**: Vetor linear (`std::vector<uint32_t>`)
- **Disco**: Matriz 2D (endereço linear → coordenadas)

---

### 3. CPU MULTICORE COM PIPELINE MIPS
**Localização**: 
- `src/multicore/MultiCore.hpp/cpp` (gerenciador)
- `src/multicore/Core.hpp/cpp` (núcleo individual)
- `src/cpu/CONTROL_UNIT.hpp/cpp` (unidade de controle)

**Pipeline de 5 Estágios**:
1. **IF (Fetch)**: Busca instrução da memória, incrementa PC
2. **ID (Decode)**: Decodifica opcode, extrai registradores e imediato
3. **EX (Execute)**: Executa operação (ULA, branches, I/O)
4. **MEM (Memory)**: Acesso à memória (LW, SW)
5. **WB (Write Back)**: Escreve resultado no banco de registradores

**Características**:
- Execução paralela em múltiplos cores
- Preempção por quantum
- Finalização segura do pipeline (5 ciclos de flush)

**Registradores MIPS**: R0-R31 (propósito geral) + especiais (PC, MAR, IR, etc.)

---

### 4. SISTEMA DE I/O (I/O Manager)
**Localização**: `src/IO/IOManager.hpp/cpp`

**Funcionalidades**:
- Gerencia processos bloqueados aguardando I/O
- Simula dispositivos (impressora, disco, rede)
- Processa requisições com latência configurável
- Libera processos após conclusão de I/O

---

### 5. PCB (Process Control Block)
**Localização**: `src/cpu/PCB.hpp`

**Armazena**:
- Identificação (pid, name)
- Estado e contexto de CPU (registradores)
- Informações de memória (partição, segmentos)
- Métricas (cache hits/misses, ciclos, tempos)
- Segmentos de código e dados

---

## Fluxo de Execução

```
1. Inicialização
   ├─ Carrega processos de JSON
   ├─ Aloca memória (partições fixas)
   └─ Adiciona ao escalonador

2. Loop Principal (por ciclo)
   ├─ Distribui processos para cores livres
   ├─ Executa 1 ciclo do pipeline em cada core
   ├─ Processa eventos (FINISHED, BLOCKED, PREEMPTED)
   └─ Avança I/O Manager

3. Finalização
   ├─ Flush de cache
   └─ Coleta e exibe métricas
```

---

## Principais Algoritmos Utilizados

| Componente | Algoritmo/Mecanismo |
|------------|---------------------|
| Escalonador | FCFS, Round-Robin, Priority, SJN |
| Memória | Partições Fixas, Tradução de Endereços |
| Cache | FIFO (substituição), Write-Back (escrita) |
| Pipeline | 5 estágios MIPS (IF-ID-EX-MEM-WB) |
| Multicore | Distribuição round-robin de processos |

---

## Pontos Críticos para Testes

1. **Pipeline**: Validação dos 5 estágios e finalização correta
2. **Cache**: Política FIFO, write-back, eviction
3. **Escalonamento**: Todas as 4 políticas funcionando
4. **Memória**: Tradução de endereços, proteção, isolamento
5. **I/O**: Bloqueio/desbloqueio de processos
6. **Multicore**: Execução paralela sem race conditions
7. **Métricas**: Contabilização correta de hits, misses, ciclos

---

**Para análise completa, consulte `ANALISE_DETALHADA.md`**

