#ifndef GEMINIQUIZGENERATOR_H
#define GEMINIQUIZGENERATOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QNetworkAccessManager>

// Estrutura compartilhada com QuizEngine
struct QuestaoGerada {
    int id = 0;
    QString nivel;        // "Muito Fácil", "Fácil", "Difícil"
    QString modalidade;   // Uma das 9 modalidades exigidas no prompt
    QString tema;         // Subtema extraído do PDF
    QString enunciado;
    QString tipoEntrada;  // "opcoes", "certo_errado", "somatorio", "texto_curto"
    QStringList opcoes;
    QString respostaCorreta;
    int somaEsperada = 0;
    QStringList itensSomatorio;
    QString explicacaoErro;
    QString explicacaoAcerto;
    QString svgIlustracao;
};

class GeminiQuizGenerator : public QObject {
    Q_OBJECT
public:
    explicit GeminiQuizGenerator(QObject *parent = nullptr);

    // Gera 120 questões (40 por nível) usando a API Gemini se apiKey não estiver vazia;
    // caso contrário ou em falha de rede, gera o banco de 120 questões baseado estritamente no texto extraído.
    void solicitarBanco120Questoes(const QString &textoPdf,
                                   const QString &nomeArquivo,
                                   const QString &apiKey = QString());

    static QVector<QuestaoGerada> gerarBancoEstritoDoTexto(const QString &textoPdf,
                                                            const QString &nomeArquivo);

signals:
    void bancoPronto(const QVector<QuestaoGerada> &questoes, const QString &resumoOrigem);
    void erroGeracao(const QString &mensagemErro);

private:
    static QString montarPromptEstrito(const QString &textoPdf, const QString &nomeArquivo);
    static QVector<QuestaoGerada> parsearJsonQuestoes(const QString &jsonBruto, const QString &nomeArquivo);
    static QString gerarSvgTematico(const QString &tema, const QString &modalidade, int seed);

    QNetworkAccessManager m_network;
};

#endif // GEMINIQUIZGENERATOR_H
