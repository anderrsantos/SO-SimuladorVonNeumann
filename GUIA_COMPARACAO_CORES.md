# Guia: Comparação Single-Core vs Multicore

## ✅ Implementação Completa

Agora o simulador roda com **1, 2 e 4 cores** e compara os resultados automaticamente!

---

## 🚀 Como Usar

### Executar Todas as Comparações

```bash
cd build
../gerar_resultados.sh
```

**O script executa:**
- ✅ 4 políticas (FCFS, RR, Priority, SJN)
- ✅ 3 configurações de cores (1, 2, 4 cores)
- ✅ Total: **12 execuções** (4 políticas × 3 configurações)

---

## 📁 Estrutura de Arquivos Gerados

```
output/
├── comparison/
│   ├── all_policies_1cores.csv          ⭐ Comparação políticas (1 core)
│   ├── all_policies_2cores.csv          ⭐ Comparação políticas (2 cores)
│   ├── all_policies_4cores.csv          ⭐ Comparação políticas (4 cores)
│   ├── core_comparison_all_policies.csv ⭐ Comparação cores (todas políticas)
│   ├── cores_comparison_fcfs.csv         ⭐ Comparação cores (FCFS)
│   ├── cores_comparison_rr.csv           ⭐ Comparação cores (RR)
│   ├── cores_comparison_priority.csv     ⭐ Comparação cores (Priority)
│   ├── cores_comparison_sjn.csv          ⭐ Comparação cores (SJN)
│   ├── speedup_analysis.csv              ⭐ Análise de speedup
│   └── policies_summary.txt              📄 Resumo
│
└── policies/
    ├── fcfs_1cores/
    │   ├── metrics.csv
    │   ├── policy_metrics.csv
    │   ├── core_comparison.csv
    │   └── temporal_metrics.csv
    ├── fcfs_2cores/
    │   └── [mesmos arquivos]
    ├── fcfs_4cores/
    │   └── [mesmos arquivos]
    └── [rr, priority, sjn]_[1,2,4]cores/...
```

---

## 📊 Arquivos de Comparação

### 1. Comparação de Políticas por Número de Cores

**Arquivos:**
- `output/comparison/all_policies_1cores.csv`
- `output/comparison/all_policies_2cores.csv`
- `output/comparison/all_policies_4cores.csv`

**Conteúdo:** Compara todas as políticas para um número específico de cores.

**Exemplo:**
```csv
policy,avg_waiting_time,avg_turnaround_time,cpu_utilization,throughput,efficiency
FCFS,10.5,25.3,85.2,0.15,0.18
Round-Robin,8.2,22.1,90.5,0.18,0.20
Priority,7.8,20.5,88.3,0.19,0.22
SJN,9.1,23.4,87.1,0.17,0.20
```

---

### 2. Comparação de Cores por Política

**Arquivos:**
- `output/comparison/cores_comparison_fcfs.csv`
- `output/comparison/cores_comparison_rr.csv`
- `output/comparison/cores_comparison_priority.csv`
- `output/comparison/cores_comparison_sjn.csv`

**Conteúdo:** Compara 1, 2 e 4 cores para uma política específica.

**Exemplo:**
```csv
num_cores,avg_waiting_time,avg_turnaround_time,cpu_utilization,throughput,speedup
1,15.2,35.4,95.0,0.10,1.00
2,8.5,22.1,88.5,0.18,1.80
4,5.2,18.3,82.0,0.25,2.50
```

---

### 3. Comparação Agregada (Todas Políticas e Cores)

**Arquivo:** `output/comparison/core_comparison_all_policies.csv`

**Conteúdo:** Todas as políticas e números de cores em um único arquivo.

---

### 4. Análise de Speedup

**Arquivo:** `output/comparison/speedup_analysis.csv`

**Conteúdo:** Speedup calculado comparando com 1 core como baseline.

**Exemplo:**
```csv
policy,num_cores,throughput,speedup_vs_1core
fcfs,1,0.10,1.00
fcfs,2,0.18,1.80
fcfs,4,0.25,2.50
```

**Speedup = throughput_multicore / throughput_singlecore**

---

## 🔍 Como Analisar os Resultados

### 1. Comparar Políticas (mesmo número de cores)

Abra `all_policies_4cores.csv` para ver qual política performa melhor com 4 cores.

### 2. Comparar Cores (mesma política)

Abra `cores_comparison_fcfs.csv` para ver como FCFS se comporta com 1, 2 e 4 cores.

### 3. Analisar Speedup

Abra `speedup_analysis.csv` para ver o ganho de performance ao aumentar o número de cores.

**Speedup ideal:**
- 2 cores → speedup ~2.0
- 4 cores → speedup ~4.0

**Speedup real pode ser menor devido a:**
- Overhead de sincronização
- Contenção de recursos
- Paralelismo limitado

---

## 📈 Exemplo de Uso

```bash
# 1. Gerar todos os resultados
cd build
../gerar_resultados.sh

# 2. Ver comparação de políticas com 4 cores
cat output/comparison/all_policies_4cores.csv

# 3. Ver como FCFS se comporta com diferentes números de cores
cat output/comparison/cores_comparison_fcfs.csv

# 4. Ver análise de speedup
cat output/comparison/speedup_analysis.csv
```

---

## ✅ Métricas Geradas

Para cada combinação (política × cores):

1. **Métricas por processo** (`metrics.csv`)
2. **Métricas agregadas** (`policy_metrics.csv`)
   - Tempo médio de espera
   - Tempo médio de retorno
   - Utilização da CPU
   - Throughput
   - Eficiência
3. **Comparação multicore** (`core_comparison.csv`)
4. **Evolução temporal** (`temporal_metrics.csv`)

---

## 🎯 Vantagens

1. ✅ **Comparação completa**: Todas as combinações testadas
2. ✅ **Speedup real**: Calculado comparando com 1 core
3. ✅ **Organizado**: Resultados separados por política e cores
4. ✅ **Análise fácil**: Arquivos CSV prontos para gráficos

---

**Agora você tem dados completos para comparar Single-Core vs Multicore!** 🎉

