# 📋 Comandos do Makefile – Simulador Von Neumann (SO)

Este documento descreve todos os comandos disponíveis no Makefile do simulador da arquitetura de Von Neumann com pipeline MIPS, escalonamento e gerenciamento de memória.

---

## 🎯 Comandos Disponíveis

---

## 🔨 Comandos Básicos

- `make` ou `make all`  
  Compila todo o simulador.

- `make simulador`  
  Compila apenas o executável principal.

- `make run`  
  Executa o simulador após compilar.

- `make clean`  
  Remove arquivos de build (`build/`), objetos (`.o`) e executáveis.

---

## 🧪 Comandos de Teste (Versão Oficial - Inglês)

| Comando | Descrição |
|--------|-----------|
| `make test_scheduler_priority` | Teste do escalonador por prioridade |
| `make test_memory_critical` | Teste crítico de memória + cache |
| `make test_pipeline_basic` | Teste do pipeline MIPS |
| `make test_integration_complete` | Teste completo da integração do sistema |
| `make test_edge_cases` | Teste de casos extremos e exceções |
| `make test_performance` | Teste de desempenho do simulador |
| `make test_stress` | Teste de stress com múltiplas operações |
| `make test_io_detailed` | Teste detalhado do subsistema de I/O |
| `make test_metrics` | Teste de métricas (ciclos, acessos, cache) |

---

## 🇧🇷 Comandos de Teste (Aliases em Português – Compila + Executa)

| Comando | Teste executado |
|---------|------------------|
| `make teste_escalonador` | Executa `test_scheduler_priority` |
| `make teste_memoria` | Executa `test_memory_critical` |
| `make teste_pipeline` | Executa `test_pipeline_basic` |
| `make teste_integracao` | Executa `test_integration_complete` |
| `make teste_borda` | Executa `test_edge_cases` |
| `make teste_performance` | Executa `test_performance` |
| `make teste_stress` | Executa `test_stress` |
| `make teste_io` | Executa `test_io_detailed` |
| `make teste_metricas` | Executa `test_metrics` |

---

## 🧪🧪 Executar Todos os Testes

- `make test-all`  
  Compila e executa **todos os testes automaticamente**, exibindo o resultado de cada um.

---

## ℹ️ Ajuda

- `make help`  
  Mostra todos os comandos disponíveis no Makefile.

---

# 📌 Exemplos de Uso

---

### 👨‍💻 Desenvolvimento

```bash
make            # Compilar o simulador
make run        # Executar o simulador
make teste_pipeline   # Testar o pipeline MIPS
