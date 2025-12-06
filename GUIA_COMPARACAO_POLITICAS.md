# Guia: Comparação de Políticas de Escalonamento

## ✅ Resposta Rápida

**NÃO precisa rodar manualmente cada política!** Use o script `gerar_resultados.sh` que:
1. ✅ Roda todas as políticas automaticamente
2. ✅ Salva cada política em pasta separada (não sobrescreve)
3. ✅ Gera arquivo de comparação final com todas as políticas

---

## 🚀 Como Usar (Método Automatizado - RECOMENDADO)

### Opção 1: Script Automatizado (Mais Fácil)

```bash
cd build
chmod +x ../gerar_resultados.sh
../gerar_resultados.sh
```

**O que o script faz:**
1. Executa o simulador com cada política (FCFS, RR, Priority, SJN)
2. Salva resultados de cada política em `output/policies/[política]/`
3. Gera arquivo de comparação final: `output/comparison/all_policies_comparison.csv`

---

## 📁 Estrutura de Arquivos Gerados

```
output/
├── policies/
│   ├── fcfs/
│   │   ├── metrics.csv              # Métricas por processo (FCFS)
│   │   ├── policy_metrics.csv        # Métricas agregadas (FCFS)
│   │   └── temporal_metrics.csv      # Evolução temporal (FCFS)
│   ├── rr/
│   │   ├── metrics.csv               # Métricas por processo (RR)
│   │   ├── policy_metrics.csv        # Métricas agregadas (RR)
│   │   └── temporal_metrics.csv      # Evolução temporal (RR)
│   ├── priority/
│   │   └── ...
│   └── sjn/
│       └── ...
└── comparison/
    ├── all_policies_comparison.csv   # ⭐ COMPARAÇÃO FINAL (todas as políticas)
    └── policies_summary.txt          # Resumo
```

---

## 📊 Arquivo de Comparação Final

**Arquivo**: `output/comparison/all_policies_comparison.csv`

**Conteúdo**: Uma linha para cada política com todas as métricas

```csv
policy,avg_waiting_time,avg_turnaround_time,cpu_utilization,throughput,efficiency,num_processes,total_cycles
FCFS,10.50,25.30,85.20,0.15,0.18,5,150
Round-Robin,8.20,22.10,90.50,0.18,0.20,5,140
Priority,7.80,20.50,88.30,0.19,0.22,5,135
SJN,9.10,23.40,87.10,0.17,0.20,5,145
```

**Uso**: Abra no Excel/LibreOffice para comparar visualmente todas as políticas!

---

## 🔧 Método Manual (Se Precisar)

Se quiser rodar manualmente cada política:

```bash
cd build

# Cada execução salva em pasta separada
./simulador fcfs      # Salva em output/policies/fcfs/
./simulador rr        # Salva em output/policies/rr/
./simulador priority   # Salva em output/policies/priority/
./simulador sjn       # Salva em output/policies/sjn/
```

**Importante**: Cada política é salva em sua própria pasta, então **NÃO sobrescreve** os resultados anteriores!

---

## 📈 Como Comparar as Políticas

### 1. Usando o Arquivo de Comparação

```bash
# Ver o arquivo de comparação
cat output/comparison/all_policies_comparison.csv
```

### 2. Abrir no Excel/LibreOffice

1. Abra `output/comparison/all_policies_comparison.csv`
2. Crie gráficos comparando:
   - Tempo médio de espera por política
   - Tempo médio de retorno por política
   - Utilização da CPU por política
   - Throughput por política

### 3. Comparar Métricas Temporais

Cada política tem seu próprio `temporal_metrics.csv`:
- `output/policies/fcfs/temporal_metrics.csv`
- `output/policies/rr/temporal_metrics.csv`
- etc.

Compare os gráficos de uso de CPU e memória ao longo do tempo.

---

## ✅ Vantagens da Nova Estrutura

1. ✅ **Não sobrescreve**: Cada política tem sua própria pasta
2. ✅ **Organizado**: Fácil encontrar resultados de cada política
3. ✅ **Comparação fácil**: Arquivo único com todas as políticas
4. ✅ **Automático**: Script faz tudo de uma vez

---

## 🎯 Exemplo de Uso Completo

```bash
# 1. Compilar
cd build
cmake ..
make

# 2. Gerar todos os resultados (automático)
../gerar_resultados.sh

# 3. Ver comparação final
cat output/comparison/all_policies_comparison.csv

# 4. Ver resultados de uma política específica
cat output/policies/fcfs/policy_metrics.csv
```

---

## 📝 Resumo

| Pergunta | Resposta |
|----------|----------|
| Preciso rodar cada política manualmente? | ❌ Não! Use `gerar_resultados.sh` |
| Os resultados sobrescrevem? | ❌ Não! Cada política tem sua pasta |
| Como comparar todas as políticas? | ✅ Use `output/comparison/all_policies_comparison.csv` |
| Onde estão os resultados de cada política? | ✅ `output/policies/[política]/` |

---

**Agora você pode comparar todas as políticas facilmente!** 🎉

