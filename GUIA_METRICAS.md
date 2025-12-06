# Guia de Métricas - Resultados do Simulador

## ✅ Status: Métricas Implementadas e Salvando

### O que JÁ está sendo calculado e salvo:

#### 1. **Métricas por Processo** (`output/metrics.csv`)
- ✅ Turnaround time (tempo de retorno)
- ✅ Waiting time (tempo de espera)
- ✅ Response time (tempo de resposta)
- ✅ Pipeline cycles
- ✅ Cache hits/misses
- ✅ Memory accesses
- ✅ I/O cycles

#### 2. **Métricas Agregadas por Política** (`output/policy_metrics.csv`)
- ✅ **Tempo médio de espera** (avg_waiting_time)
- ✅ **Tempo médio de retorno** (avg_turnaround_time)
- ✅ **Utilização média da CPU** (cpu_utilization %)
- ✅ **Throughput** (processos/ciclo)
- ✅ **Eficiência** (throughput / utilização)

#### 3. **Métricas Temporais** (`output/temporal_metrics.csv`)
- ✅ **Uso da CPU ao longo da execução** (cpu_usage_percent por tick)
- ✅ **Uso da memória ao longo do tempo** (memory_usage_percent por tick)
- ✅ **Throughput ao longo do tempo** (throughput_instant por tick)

#### 4. **Comparação Multicore** (`output/core_comparison.csv`)
- ✅ Espera média
- ✅ Retorno médio
- ✅ Utilização
- ✅ Throughput
- ⚠️ Speedup (requer execução separada com 1 core)

---

## 📊 Como Gerar Todos os Resultados

### Opção 1: Executar Manualmente

```bash
cd build

# Executar com cada política
./simulador fcfs      # FCFS
./simulador rr        # Round-Robin
./simulador priority  # Priority
./simulador sjn       # SJN

# Os resultados serão salvos em output/
```

### Opção 2: Usar Script Automatizado

```bash
cd build
chmod +x ../gerar_resultados.sh
../gerar_resultados.sh
```

---

## 📁 Arquivos Gerados

Após executar o simulador, os seguintes arquivos são criados em `output/`:

1. **`metrics.csv`** - Métricas detalhadas por processo
   - Colunas: pid, name, arrival, start, finish, turnaround, waiting, response, pipeline, cache_hits, cache_misses, mem_accesses, io_cycles

2. **`policy_metrics.csv`** - Métricas agregadas por política
   - Colunas: policy, avg_waiting_time, avg_turnaround_time, cpu_utilization, throughput, efficiency, num_processes, total_cycles

3. **`temporal_metrics.csv`** - Evolução temporal (para gráficos)
   - Colunas: tick, cpu_usage_percent, memory_usage_percent, throughput_instant, active_processes, completed_processes

4. **`core_comparison.csv`** - Comparação Single-Core vs Multicore
   - Colunas: num_cores, avg_waiting_time, avg_turnaround_time, cpu_utilization, throughput, speedup

5. **`metrics.json`** - Mesmas métricas em formato JSON

---

## 🔍 Métricas por Política de Escalonamento

### Para obter resultados de cada política:

1. **FCFS (First Come First Served)**
   ```bash
   ./simulador fcfs
   ```
   - Resultados em: `output/policy_metrics.csv`

2. **Round-Robin**
   ```bash
   ./simulador rr
   ```
   - Resultados em: `output/policy_metrics.csv`

3. **Priority**
   ```bash
   ./simulador priority
   ```
   - Resultados em: `output/policy_metrics.csv`

4. **SJN (Shortest Job Next)**
   ```bash
   ./simulador sjn
   ```
   - Resultados em: `output/policy_metrics.csv`

### Separar Preemptivo vs Não-Preemptivo:

- **Não-Preemptivo**: FCFS, SJN (quando implementado sem preempção)
- **Preemptivo**: RR, Priority (com quantum)

**Nota**: Atualmente todas as políticas podem ser preemptivas (dependendo do quantum). Para separar, seria necessário:
- FCFS: quantum = infinito (não preemptivo)
- RR: quantum finito (preemptivo)
- Priority: pode ser ambos (depende da implementação)
- SJN: geralmente não-preemptivo

---

## 📈 Comparação Single-Core vs Multicore

### Para fazer comparação completa:

1. **Executar com 1 core** (modificar `NCORES = 1` no main.cpp ou adicionar parâmetro)
2. **Executar com N cores** (padrão: 4 cores)
3. **Comparar resultados** nos arquivos CSV

**Métricas comparadas**:
- Espera média
- Retorno médio
- Utilização
- Throughput
- Speedup global = throughput_multicore / throughput_singlecore

---

## 📊 Gráficos (Evolução Temporal)

Os dados em `temporal_metrics.csv` podem ser usados para gerar gráficos:

### Uso da CPU ao longo da execução
- **Coluna**: `cpu_usage_percent`
- **Eixo X**: `tick`
- **Eixo Y**: `cpu_usage_percent` (0-100%)

### Uso da memória ao longo do tempo
- **Coluna**: `memory_usage_percent`
- **Eixo X**: `tick`
- **Eixo Y**: `memory_usage_percent` (0-100%)

### Throughput ao longo do tempo
- **Coluna**: `throughput_instant`
- **Eixo X**: `tick`
- **Eixo Y**: `throughput_instant` (processos/ciclo)

---

## ⚠️ Observações Importantes

1. **Speedup**: Para calcular speedup real, é necessário executar duas vezes:
   - Uma vez com `NCORES = 1`
   - Outra vez com `NCORES = 4` (ou outro valor)
   - Comparar os throughputs

2. **Preemptivo vs Não-Preemptivo**: 
   - Atualmente, todas as políticas podem ser preemptivas (dependendo do quantum)
   - Para FCFS não-preemptivo, usar quantum muito alto
   - Para RR preemptivo, usar quantum baixo

3. **Métricas Temporais**: 
   - Coletadas a cada 10 ticks (para não gerar arquivo muito grande)
   - Pode ser ajustado no `main.cpp` (linha `if (tick % 10 == 0)`)

---

## 📝 Exemplo de Uso

```bash
# 1. Compilar
cd build
cmake ..
make

# 2. Executar com FCFS
./simulador fcfs

# 3. Ver resultados
cat output/policy_metrics.csv
cat output/temporal_metrics.csv

# 4. Repetir para outras políticas
./simulador rr
./simulador priority
./simulador sjn
```

---

**Todas as métricas solicitadas estão sendo calculadas e salvas automaticamente!**

