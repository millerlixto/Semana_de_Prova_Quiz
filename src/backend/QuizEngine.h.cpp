#ifndef QUIZENGINE_H
#define QUIZENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QSet>

#include "MuPdfExtractor.h"
#include "GeminiQuizGenerator.h"

class QuizEngine : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString telaAtual READ telaAtual NOTIFY estadoAlterado)
    Q_PROPERTY(QString nivelAtual READ nivelAtual NOTIFY estadoAlterado)
    Q_PROPERTY(QString nomeDocumento READ nomeDocumento NOTIFY estadoAlterado)
    Q_PROPERTY(QString statusDocumento READ statusDocumento NOTIFY estadoAlterado)
    Q_PROPERTY(bool gerandoBanco READ gerandoBanco NOTIFY estadoAlterado)

    Q_PROPERTY(int vistasNoCiclo READ vistasNoCiclo NOTIFY estadoAlterado)
    Q_PROPERTY(int totalNoNivel READ totalNoNivel NOTIFY estadoAlterado)
    Q_PROPERTY(QString resumoCiclo READ resumoCiclo NOTIFY estadoAlterado)

    Q_PROPERTY(int indiceRodada READ indiceRodada NOTIFY estadoAlterado)
    Q_PROPERTY(int totalRodada READ totalRodada NOTIFY estadoAlterado)
    Q_PROPERTY(int acertosRodada READ acertosRodada NOTIFY estadoAlterado)

    Q_PROPERTY(int questaoId READ questaoId NOTIFY estadoAlterado)
    Q_PROPERTY(QString modalidadeAtual READ modalidadeAtual NOTIFY estadoAlterado)
    Q_PROPERTY(QString temaAtual READ temaAtual NOTIFY estadoAlterado)
    Q_PROPERTY(QString enunciadoAtual READ enunciadoAtual NOTIFY estadoAlterado)
    Q_PROPERTY(QString tipoEntradaAtual READ tipoEntradaAtual NOTIFY estadoAlterado)
    Q_PROPERTY(QStringList opcoesAtuais READ opcoesAtuais NOTIFY estadoAlterado)
    Q_PROPERTY(QStringList itensSomatorioAtual READ itensSomatorioAtual NOTIFY estadoAlterado)
    Q_PROPERTY(QString svgAtual READ svgAtual NOTIFY estadoAlterado)

    Q_PROPERTY(bool respondida READ respondida NOTIFY estadoAlterado)
    Q_PROPERTY(bool acertouUltima READ acertouUltima NOTIFY estadoAlterado)
    Q_PROPERTY(QString feedbackTitulo READ feedbackTitulo NOTIFY estadoAlterado)
    Q_PROPERTY(QString feedbackDetalhe READ feedbackDetalhe NOTIFY estadoAlterado)
    Q_PROPERTY(bool podeAvancar READ podeAvancar NOTIFY estadoAlterado)

    Q_PROPERTY(double percentualFinal READ percentualFinal NOTIFY estadoAlterado)
    Q_PROPERTY(QString mensagemDesempenho READ mensagemDesempenho NOTIFY estadoAlterado)
    Q_PROPERTY(QString resumoTemas READ resumoTemas NOTIFY estadoAlterado)
    Q_PROPERTY(bool revisaoVisivel READ revisaoVisivel NOTIFY estadoAlterado)
    Q_PROPERTY(QVariantList historicoRodada READ historicoRodada NOTIFY estadoAlterado)

public:
    explicit QuizEngine(QObject *parent = nullptr);

    // Carrega um arquivo PDF real (via MuPDF) ou texto colado e gera 120 questões
    Q_INVOKABLE bool carregarPdfArquivo(const QString &caminhoArquivo, const QString &apiKey = QString());
    Q_INVOKABLE void gerarDeTexto(const QString &textoConteudo,
                                  const QString &rotuloDocumento = QStringLiteral("Conteúdo de Estudo"),
                                  const QString &apiKey = QString());

    Q_INVOKABLE void selecionarNivel(const QString &nivel);
    Q_INVOKABLE void iniciarRodada(const QString &nivel = QString());
    Q_INVOKABLE void reiniciarQuiz();
    Q_INVOKABLE void zerarCicloNivel(const QString &nivel = QString());
    Q_INVOKABLE void confirmarResposta(const QString &respostaUsuario);
    Q_INVOKABLE void proximo();
    Q_INVOKABLE void alternarRevisao();
    Q_INVOKABLE void voltarInicio();

    QString telaAtual() const { return m_telaAtual; }
    QString nivelAtual() const { return m_nivelAtual; }
    QString nomeDocumento() const { return m_nomeDocumento; }
    QString statusDocumento() const { return m_statusDocumento; }
    bool gerandoBanco() const { return m_gerandoBanco; }

    int vistasNoCiclo() const;
    int totalNoNivel() const { return 40; }
    QString resumoCiclo() const;

    int indiceRodada() const { return m_indiceRodada; }
    int totalRodada() const { return m_rodadaIds.size(); }
    int acertosRodada() const { return m_acertosRodada; }

    int questaoId() const;
    QString modalidadeAtual() const;
    QString temaAtual() const;
    QString enunciadoAtual() const;
    QString tipoEntradaAtual() const;
    QStringList opcoesAtuais() const;
    QStringList itensSomatorioAtual() const;
    QString svgAtual() const;

    bool respondida() const { return m_respondida; }
    bool acertouUltima() const { return m_acertouUltima; }
    QString feedbackTitulo() const { return m_feedbackTitulo; }
    QString feedbackDetalhe() const { return m_feedbackDetalhe; }
    bool podeAvancar() const { return m_respondida; }

    double percentualFinal() const;
    QString mensagemDesempenho() const;
    QString resumoTemas() const;
    bool revisaoVisivel() const { return m_revisaoVisivel; }
    QVariantList historicoRodada() const { return m_historicoRodada; }

    const QuestaoGerada* questaoAtualPtr() const;
    int quantidadeBancoTotal() const { return m_banco.size(); }

signals:
    void estadoAlterado();

private slots:
    void aoReceberBanco(const QVector<QuestaoGerada> &questoes, const QString &resumoOrigem);

private:
    QString normalizarResposta(const QString &texto) const;
    QVector<int> sortearIdsIneditos(const QString &nivel, int quantidade);
    void prepararQuestaoAtual();

    GeminiQuizGenerator m_gerador;
    QVector<QuestaoGerada> m_banco;
    QSet<int> m_vistosMuitoFacil;
    QSet<int> m_vistosFacil;
    QSet<int> m_vistosDificil;

    QString m_telaAtual = QStringLiteral("inicio");
    QString m_nivelAtual = QStringLiteral("Muito Fácil");
    QString m_nomeDocumento = QStringLiteral("prompt-quiz-interativo.pdf");
    QString m_statusDocumento;
    bool m_gerandoBanco = false;

    QVector<int> m_rodadaIds;
    int m_indiceRodada = 0;
    int m_acertosRodada = 0;

    bool m_respondida = false;
    bool m_acertouUltima = false;
    QString m_feedbackTitulo;
    QString m_feedbackDetalhe;
    bool m_revisaoVisivel = false;
    QVariantList m_historicoRodada;
};

#endif // QUIZENGINE_H