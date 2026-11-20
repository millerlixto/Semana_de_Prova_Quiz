#include <iostream>
#include <string>
#include "QuizEngine.h"

int main() {
    QuizEngine engine;
    engine.selecionarNivel("Muito Fácil");
    engine.iniciarRodada("Muito Fácil");

    std::cout << "Total no banco: " << engine.quantidadeBancoTotal() << "\n";
    std::cout << engine.resumoCiclo().toStdString() << "\n\n";

    while (engine.telaAtual() == "quiz") {
        std::cout << "Questao " << (engine.indiceRodada() + 1) << "/" << engine.totalRodada()
                  << " | " << engine.modalidadeAtual().toStdString()
                  << " | Tema: " << engine.temaAtual().toStdString() << "\n";
        std::cout << engine.enunciadoAtual().toStdString() << "\n";

        for (const QString &it : engine.itensSomatorioAtual()) {
            std::cout << "  " << it.toStdString() << "\n";
        }
        for (const QString &op : engine.opcoesAtuais()) {
            std::cout << "  " << op.toStdString() << "\n";
        }

        std::cout << "Resposta: ";
        std::string linha;
        std::getline(std::cin >> std::ws, linha);

        engine.confirmarResposta(QString::fromStdString(linha));
        std::cout << "[" << engine.feedbackTitulo().toStdString() << "] "
                  << engine.feedbackDetalhe().toStdString() << "\n\n";
        engine.proximo();
    }

    std::cout << "Placar: " << engine.acertosRodada() << "/" << engine.totalRodada()
              << " (" << engine.percentualFinal() << "%)\n";
    std::cout << engine.mensagemDesempenho().toStdString() << "\n";
    std::cout << engine.resumoTemas().toStdString() << "\n";

    std::cin.get();
    return 0;
}