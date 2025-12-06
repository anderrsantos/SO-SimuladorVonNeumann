#!/bin/bash
# Script para gerar todos os resultados necessários para o relatório
# Executa o simulador com cada política e número de cores, gerando comparações

echo "=========================================="
echo "  GERANDO RESULTADOS DO SIMULADOR"
echo "  Comparação de Políticas e Cores"
echo "=========================================="
echo ""

# Criar diretório de resultados
mkdir -p output
mkdir -p output/policies
mkdir -p output/comparison
mkdir -p output/cores

# Políticas de escalonamento
POLICIES=("fcfs" "rr" "priority" "sjn")

# Números de cores para testar
CORES=(1 2 4)

echo "Executando simulações para cada política e número de cores..."
echo ""

# Vetor para armazenar métricas de todas as políticas
declare -a POLICY_FILES
declare -a CORE_COMPARISON_FILES

# Executar para cada política e cada número de cores
for policy in "${POLICIES[@]}"; do
    for cores in "${CORES[@]}"; do
        echo ">>> Executando: $policy com $cores core(s)"
        ./simulador $policy $cores > "output/policies/${policy}_${cores}cores_output.txt" 2>&1
        
        # Guardar caminho do arquivo de métricas desta política e cores
        POLICY_FILES+=("output/policies/${policy}_${cores}cores/policy_metrics.csv")
        CORE_COMPARISON_FILES+=("output/policies/${policy}_${cores}cores/core_comparison.csv")
        
        echo "  ✓ $policy ($cores cores) concluído"
    done
    echo ""
done

# Agregar todas as políticas em um único arquivo de comparação (por número de cores)
echo ">>> Gerando arquivos de comparação..."

for cores in "${CORES[@]}"; do
    COMPARISON_FILE="output/comparison/all_policies_${cores}cores.csv"
    
    # Criar cabeçalho
    echo "policy,avg_waiting_time,avg_turnaround_time,cpu_utilization,throughput,efficiency,num_processes,total_cycles" > "$COMPARISON_FILE"
    
    # Agregar dados de cada política para este número de cores
    for policy in "${POLICIES[@]}"; do
        policy_file="output/policies/${policy}_${cores}cores/policy_metrics.csv"
        if [ -f "$policy_file" ]; then
            tail -n +2 "$policy_file" >> "$COMPARISON_FILE"
        fi
    done
    
    echo "  ✓ Comparação de políticas ($cores cores): $COMPARISON_FILE"
done

# Agregar comparação Single-Core vs Multicore de todas as políticas
echo ""
echo ">>> Gerando comparação Single-Core vs Multicore agregada..."
CORE_COMPARISON_FILE="output/comparison/core_comparison_all_policies.csv"

# Criar cabeçalho
echo "policy,num_cores,avg_waiting_time,avg_turnaround_time,cpu_utilization,throughput,speedup" > "$CORE_COMPARISON_FILE"

# Agregar dados de cada política e número de cores
for policy in "${POLICIES[@]}"; do
    for cores in "${CORES[@]}"; do
        core_file="output/policies/${policy}_${cores}cores/core_comparison.csv"
        if [ -f "$core_file" ]; then
            # Ler dados e adicionar nome da política
            tail -n +2 "$core_file" | while IFS=',' read -r num_cores wait turn util thru speed; do
                echo "$policy,$num_cores,$wait,$turn,$util,$thru,$speed" >> "$CORE_COMPARISON_FILE"
            done
        fi
    done
done

echo "  ✓ Comparação Single-Core vs Multicore criada: $CORE_COMPARISON_FILE"

# Criar arquivo de comparação de cores para cada política
echo ""
echo ">>> Gerando comparação de cores por política..."
for policy in "${POLICIES[@]}"; do
    CORE_POLICY_FILE="output/comparison/cores_comparison_${policy}.csv"
    
    # Criar cabeçalho
    echo "num_cores,avg_waiting_time,avg_turnaround_time,cpu_utilization,throughput,speedup" > "$CORE_POLICY_FILE"
    
    # Agregar dados de cada número de cores para esta política
    for cores in "${CORES[@]}"; do
        core_file="output/policies/${policy}_${cores}cores/core_comparison.csv"
        if [ -f "$core_file" ]; then
            tail -n +2 "$core_file" >> "$CORE_POLICY_FILE"
        fi
    done
    
    echo "  ✓ Comparação de cores ($policy): $CORE_POLICY_FILE"
done

# Calcular speedup real (comparando com 1 core como baseline)
echo ""
echo ">>> Calculando speedup real (1 core como baseline)..."
SPEEDUP_FILE="output/comparison/speedup_analysis.csv"
echo "policy,num_cores,throughput,speedup_vs_1core" > "$SPEEDUP_FILE"

for policy in "${POLICIES[@]}"; do
    # Ler throughput de 1 core (baseline)
    baseline_file="output/policies/${policy}_1cores/core_comparison.csv"
    if [ -f "$baseline_file" ] && [ -s "$baseline_file" ]; then
        # Extrair throughput da linha de dados (coluna 5)
        baseline_line=$(tail -n +2 "$baseline_file" | head -1)
        if [ -n "$baseline_line" ]; then
            baseline_throughput=$(echo "$baseline_line" | cut -d',' -f5)
            
            # Adicionar linha para 1 core (speedup = 1.0)
            echo "$policy,1,$baseline_throughput,1.00" >> "$SPEEDUP_FILE"
            
            # Calcular speedup para 2 e 4 cores
            for cores in 2 4; do
                core_file="output/policies/${policy}_${cores}cores/core_comparison.csv"
                if [ -f "$core_file" ] && [ -s "$core_file" ]; then
                    current_line=$(tail -n +2 "$core_file" | head -1)
                    if [ -n "$current_line" ]; then
                        current_throughput=$(echo "$current_line" | cut -d',' -f5)
                        # Verificar se valores são válidos e não zero
                        if [ -n "$baseline_throughput" ] && [ -n "$current_throughput" ] && \
                           [ "$baseline_throughput" != "0" ] && [ "$baseline_throughput" != "" ] && \
                           [ "$current_throughput" != "" ]; then
                            # Calcular speedup usando awk
                            speedup=$(awk "BEGIN {printf \"%.2f\", $current_throughput / $baseline_throughput}" 2>/dev/null)
                            if [ -n "$speedup" ]; then
                                echo "$policy,$cores,$current_throughput,$speedup" >> "$SPEEDUP_FILE"
                            fi
                        fi
                    fi
                fi
            done
        fi
    fi
done

echo "  ✓ Análise de speedup criada: $SPEEDUP_FILE"

# Criar resumo estatístico
echo ""
echo ">>> Gerando resumo estatístico..."
SUMMARY_FILE="output/comparison/policies_summary.txt"
{
    echo "=========================================="
    echo "  RESUMO: COMPARAÇÃO DE POLÍTICAS E CORES"
    echo "=========================================="
    echo ""
    echo "Políticas executadas:"
    for policy in "${POLICIES[@]}"; do
        echo "  - $policy"
    done
    echo ""
    echo "Números de cores testados:"
    for cores in "${CORES[@]}"; do
        echo "  - $cores core(s)"
    done
    echo ""
    echo "Estrutura de diretórios:"
    echo "  output/policies/[política]_[cores]cores/"
    echo "    - metrics.csv (métricas por processo)"
    echo "    - policy_metrics.csv (métricas agregadas)"
    echo "    - core_comparison.csv (comparação multicore)"
    echo "    - temporal_metrics.csv (evolução temporal)"
    echo ""
    echo "Arquivos de comparação:"
    echo "  output/comparison/all_policies_[cores]cores.csv (por número de cores)"
    echo "  output/comparison/core_comparison_all_policies.csv (todas políticas e cores)"
    echo "  output/comparison/cores_comparison_[política].csv (por política)"
    echo "  output/comparison/speedup_analysis.csv (análise de speedup)"
    echo ""
} > "$SUMMARY_FILE"

echo "  ✓ Resumo criado: $SUMMARY_FILE"
echo ""

echo "=========================================="
echo "  RESULTADOS GERADOS:"
echo "=========================================="
echo ""
echo "📁 Estrutura de arquivos:"
echo "  output/policies/"
for policy in "${POLICIES[@]}"; do
    for cores in "${CORES[@]}"; do
        echo "    ├── ${policy}_${cores}cores/"
        echo "    │   ├── metrics.csv"
        echo "    │   ├── policy_metrics.csv"
        echo "    │   ├── core_comparison.csv"
        echo "    │   └── temporal_metrics.csv"
    done
done
echo ""
echo "📊 Arquivos de comparação:"
echo "  output/comparison/"
echo "    ├── all_policies_1cores.csv"
echo "    ├── all_policies_2cores.csv"
echo "    ├── all_policies_4cores.csv"
echo "    ├── core_comparison_all_policies.csv"
echo "    ├── cores_comparison_fcfs.csv"
echo "    ├── cores_comparison_rr.csv"
echo "    ├── cores_comparison_priority.csv"
echo "    ├── cores_comparison_sjn.csv"
echo "    └── speedup_analysis.csv"
echo ""
echo "=========================================="
echo "  ✅ CONCLUÍDO!"
echo "=========================================="
