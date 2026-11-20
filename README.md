# Semana_de_Prova_Quiz
O projeto é um Gerador de Quiz Interativo Ilustrado desenvolvido em C++ e QML/Qt com CMake, preparado para compilar e gerar um arquivo ".apk" Android via GitHub Actions. 
Sua proposta combina extração real de texto de documentos PDF (via MuPDF), geração de questões baseadas estritamente no material de estudo e uma interface visual inspirada no estilo do Gemini.   

**Arquitetura e divisão de responsabilidades:**

**Backend em C++ (src/backend/):** concentra toda a lógica do jogo, extração de texto de PDF com MuPDF (MuPdfExtractor), geração do banco de questões com suporte à API do Gemini e fallback local (GeminiQuizGenerator), controle de ciclo sem repetição, correção imediata, diagnóstico por tema e cálculo final (QuizEngine).  

**Entrypoint de terminal (src/terminal/main.cpp):** executável leve para testar a lógica do backend diretamente no terminal do PC, sem interface gráfica.Entrypoint,

**Frontend (src/frontend/main_frontend.cpp e src/frontend/Main.qml):** interface gráfica em QML com visual escuro estilo Gemini, seletor de arquivo PDF, seletor de níveis, exibição de SVGs, feedback imediato e tela de encerramento.   

**Build e automação (CMakeLists.txt e .github/workflows/build-android-apk.yml):** separam os alvos de compilação e automatizam a montagem do MuPDF, build Qt/CMake e empacotamento do ".apk" na nuvem.

**Regras centrais do quiz e do jogo, Banco de 120 questões isoladas:** divide o conteúdo em 3 níveis (Muito Fácil, Fácil e Difícil), com 40 questões únicas por nível e tags de tema extraídas do material.  
9 modalidades e anti-chute: distribui formatos como múltipla escolha, certo ou errado, foco negativo, asserção e razão, lacuna, associação, ordenação, somatório e resposta curta fechada, mantendo equilíbrio de tamanho nas alternativas.   
Ilustração vetorial e controle de ciclo: cada questão traz uma ilustração <svg> associada aos seus elementos, e cada rodada apresenta 10 questões inéditas dentro do ciclo de 40 do nível, com indicador X/40 vistas e botão para zerar o ciclo.   
Correção imediata e tela final: trava a entrada após responder, libera o botão Next no canto superior direito, exibe explicação na hora e, ao fim das 10 questões, mostra placar, percentual, mensagem por faixa de desempenho, resumo por tema e revisão opcional das respostas.   
