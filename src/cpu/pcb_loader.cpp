/*
  pcb_loader.cpp
  Implementação do carregamento de PCB via JSON.
*/
#include "pcb_loader.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool load_pcb_from_json(const std::string &path, PCB &pcb) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[pcb_loader] cannot open " << path << std::endl;
        return false;
    }

    try {
        json j;
        f >> j;

        pcb.pid            = j.value("pid", 0);
        pcb.name           = j.value("name", std::string(""));
        pcb.quantum        = j.value("quantum", 0);
        pcb.priority       = j.value("priority", 0);
        pcb.burst_estimate = j.value("burst_estimate", 0);

        // Pesos de memória
        if (j.contains("mem_weights")) {
            auto &mw = j["mem_weights"];
            pcb.memWeights.cache     = mw.value("cache", pcb.memWeights.cache);
            pcb.memWeights.primary   = mw.value("primary", pcb.memWeights.primary);
            pcb.memWeights.secondary = mw.value("secondary", pcb.memWeights.secondary);
        }

        // Zera buffers anteriores
        pcb.dataSegment.clear();
        pcb.codeSegment.clear();

        // =====================================================
        //         LÊ BLOCO PROGRAM { data[], code[], ... }
        // =====================================================
        if (j.contains("program") && j["program"].is_object()) {
            auto &prog = j["program"];

            // DATA: array de valores
            if (prog.contains("data") && prog["data"].is_array()) {
                for (auto &v : prog["data"]) {
                    uint32_t val = v.get<uint32_t>();
                    pcb.dataSegment.push_back(val);
                }
            }

            // CODE: instruções (inteiros de 32 bits)
            if (prog.contains("code") && prog["code"].is_array()) {
                for (auto &c : prog["code"]) {
                    uint32_t instr = c.get<uint32_t>();
                    pcb.codeSegment.push_back(instr);
                }
            }

            // =====================================================
            // LABELS — AGORA EM WORD INDEX (SEM *4)
            // =====================================================
            if (prog.contains("labels") && prog["labels"].is_object()) {
                for (auto it = prog["labels"].begin(); it != prog["labels"].end(); ++it) {
                    uint32_t wordIndex = it.value().get<uint32_t>();
                    pcb.labelMap[it.key()] = wordIndex;   // <-- CORREÇÃO FINAL
                }
            }

            // =====================================================
            // DATA SYMBOLS — TAMBÉM WORD INDEX
            // =====================================================
            if (prog.contains("data_symbols") && prog["data_symbols"].is_object()) {
                for (auto it = prog["data_symbols"].begin(); it != prog["data_symbols"].end(); ++it) {
                    uint32_t wordIndex = it.value().get<uint32_t>();
                    pcb.dataMap[it.key()] = wordIndex;    // <-- CORREÇÃO FINAL
                }
            }
        }

        // =====================================================
        // data_bytes e code_bytes = quantidade de palavras
        // =====================================================
        pcb.data_bytes = static_cast<uint32_t>(pcb.dataSegment.size());
        pcb.code_bytes = static_cast<uint32_t>(pcb.codeSegment.size());
        pcb.job_length = pcb.code_bytes;

        // =====================================================
        // PC INICIAL EM WORDS (SEM *4)
        // =====================================================
        pcb.initial_pc = pcb.data_bytes;   // <-- AGORA CORRETO

        return true;
    }
    catch (const std::exception &ex) {
        std::cerr << "[pcb_loader] error parsing " << path << " : " << ex.what() << "\n";
        return false;
    }
    catch (...) {
        return false;
    }
}
