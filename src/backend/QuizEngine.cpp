#include "QuizEngine.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QMap>
#include <algorithm>
#include <random>

namespace {
const QStringList NIVEIS_VALIDOS = {
    QStringLiteral("Muito Fácil"),
    QStringLiteral("Fácil"),
    QStringLiteral("Difícil")
};
} // namespace

QuizEngine::QuizEngine(QObject *parent) : QObject(parent) {
    QObject::connect(&m_gerador, &GeminiQuizGenerator::bancoPronto,
                     this, &QuizEngine::aoReceberBanco);

    const QString textoBase = QStringLiteral(
        "Especificação do Quiz Interativo Ilustrado: três níveis de dificuldade "
        "(Muito Fácil, Fácil, Difícil) com 40 questões exclusivas cada, somando 120 questões. "
        "Rodadas de 10 questões sem repetição dentro do ciclo de 40 questões do nível, "
        "indicador de ciclo X/40 vistas, botão Zerar ciclo deste nível, correção imediata "
        "e tela final com placar, percentual, mensagem por faixa, resumo de temas e revisão opcional."
    );
    aoReceberBanco(GeminiQuizGenerator::gerarBancoEstritoDoTexto(textoBase, QStringLiteral("prompt-quiz-interativo.pdf")),
                   QStringLiteral("120 questões prontas (40 por nível). Selecione um PDF para gerar do seu arquivo."));
}

bool QuizEngine::carregarPdfArquivo(const QString &caminhoArquivo, const QString &apiKey) {
    const ResultadoExtracaoPdf ext = MuPdfExtractor::extrairTextoDeArquivo(caminhoArquivo);
    const QString nomeArq = QFileInfo(MuPdfExtractor::normalizarCaminhoLocal(caminhoArquivo)).fileName();

    if (!ext.sucesso || ext.texto.trimmed().isEmpty()) {
        m_statusDocumento = ext.erro.isEmpty()
            ? QStringLiteral("Falha ao ler texto do PDF.")
            : ext.erro;
        emit estadoAlterado();
        return false;
    }

    m_nomeDocumento = nomeArq.isEmpty() ? QStringLiteral("documento.pdf") : nomeArq;
    m_gerandoBanco = true;
    m_statusDocumento = QStringLiteral("PDF lido via %1 (%2 pág.). Gerando 120 questões...")
                            .arg(ext.metodoUsado)
                            .arg(ext.paginasProcessadas);
    emit estadoAlterado();

    m_gerador.solicitarBanco120Questoes(ext.texto, m_nomeDocumento, apiKey);
    return true;
}

void QuizEngine::gerarDeTexto(const QString &textoConteudo,
                              const QString &rotuloDocumento,
                              const QString &apiKey) {
    const QString limpo = textoConteudo.trimmed();
    if (limpo.isEmpty()) {
        m_statusDocumento = QStringLiteral("Informe um texto ou selecione um arquivo PDF.");
        emit estadoAlterado();
        return;
    }

    m_nomeDocumento = rotuloDocumento.trimmed().isEmpty()
        ? QStringLiteral("Conteúdo de Estudo")
        : rotuloDocumento.trimmed();
    m_gerandoBanco = true;
    m_statusDocumento = QStringLiteral("Gerando 120 questões a partir de '%1'...").arg(m_nomeDocumento);
    emit estadoAlterado();

    m_gerador.solicitarBanco120Questoes(limpo, m_nomeDocumento, apiKey);
}

void QuizEngine::aoReceberBanco(const QVector<QuestaoGerada> &questoes, const QString &resumoOrigem) {
    m_banco = questoes;
    m_vistosMuitoFacil.clear();
    m_vistosFacil.clear();
    m_vistosDificil.clear();
    m_historicoRodada.clear();
    m_rodadaIds.clear();
    m_acertosRodada = 0;
    m_respondida = false;
    m_revisaoVisivel = false;
    m_gerandoBanco = false;
    m_statusDocumento = resumoOrigem;
    emit estadoAlterado();
}

QString QuizEngine::normalizarResposta(const QString &texto) const {
    QString s = texto.normalized(QString::NormalizationForm_KD).toLower().trimmed();
    s.remove(QRegularExpression(QStringLiteral("[\\x0300-\\x036f]")));
    s.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return s;
}

int QuizEngine::vistasNoCiclo() const {
    if (m_nivelAtual == QStringLiteral("Fácil")) return m_vistosFacil.size();
    if (m_nivelAtual == QStringLiteral("Difícil")) return m_vistosDificil.size();
    return m_vistosMuitoFacil.size();
}

QString QuizEngine::resumoCiclo() const {
    return QStringLiteral("Ciclo do nível %1: %2/%3 vistas")
        .arg(m_nivelAtual)
        .arg(vistasNoCiclo())
        .arg(totalNoNivel());
}

void QuizEngine::selecionarNivel(const QString &nivel) {
    if (!NIVEIS_VALIDOS.contains(nivel)) return;
    m_nivelAtual = nivel;
    emit estadoAlterado();
}

QVector<int> QuizEngine::sortearIdsIneditos(const QString &nivel, int quantidade) {
    QSet<int> *vistos = &m_vistosMuitoFacil;
    if (nivel == QStringLiteral("Fácil")) vistos = &m_vistosFacil;
    else if (nivel == QStringLiteral("Difícil")) vistos = &m_vistosDificil;

    QVector<int> candidatos;
    for (const QuestaoGerada &q : std::as_const(m_banco)) {
        if (q.nivel == nivel && !vistos->contains(q.id)) {
            candidatos.append(q.id);
        }
    }

    if (candidatos.size() < quantidade) {
        vistos->clear();
        candidatos.clear();
        for (const QuestaoGerada &q : std::as_const(m_banco)) {
            if (q.nivel == nivel) {
                candidatos.append(q.id);
            }
        }
    }

    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(candidatos.begin(), candidatos.end(), rng);

    QVector<int> escolhidos;
    const int limite = std::min(quantidade, static_cast<int>(candidatos.size()));
    for (int i = 0; i < limite; ++i) {
        escolhidos.append(candidatos[i]);
        vistos->insert(candidatos[i]);
    }
    return escolhidos;
}

void QuizEngine::iniciarRodada(const QString &nivel) {
    if (!nivel.isEmpty() && NIVEIS_VALIDOS.contains(nivel)) {
        m_nivelAtual = nivel;
    }

    m_rodadaIds = sortearIdsIneditos(m_nivelAtual, 10);
    m_indiceRodada = 0;
    m_acertosRodada = 0;
    m_revisaoVisivel = false;
    m_historicoRodada.clear();
    m_telaAtual = QStringLiteral("quiz");
    prepararQuestaoAtual();
    emit estadoAlterado();
}

void QuizEngine::reiniciarQuiz() {
    iniciarRodada(m_nivelAtual);
}

void QuizEngine::zerarCicloNivel(const QString &nivel) {
    const QString alvo = nivel.isEmpty() ? m_nivelAtual : nivel;
    if (alvo == QStringLiteral("Fácil")) m_vistosFacil.clear();
    else if (alvo == QStringLiteral("Difícil")) m_vistosDificil.clear();
    else m_vistosMuitoFacil.clear();

    if (m_telaAtual == QStringLiteral("quiz")) {
        iniciarRodada(alvo);
    } else {
        emit estadoAlterado();
    }
}

void QuizEngine::prepararQuestaoAtual() {
    m_respondida = false;
    m_acertouUltima = false;
    m_feedbackTitulo.clear();
    m_feedbackDetalhe.clear();
}

const QuestaoGerada* QuizEngine::questaoAtualPtr() const {
    if (m_indiceRodada < 0 || m_indiceRodada >= m_rodadaIds.size()) return nullptr;
    const int idAlvo = m_rodadaIds[m_indiceRodada];
    for (const QuestaoGerada &q : m_banco) {
        if (q.id == idAlvo) return &q;
    }
    return nullptr;
}

int QuizEngine::questaoId() const {
    const QuestaoGerada *q = questaoAtualPtr();
    return q ? q->id : 0;
}

QString QuizEngine::modalidadeAtual() const {
    const QuestaoGerada *q = questaoAtualPtr();
    return q ? q->modalidade : QString();
}

QString QuizEngine::temaAtual() const {
    const QuestaoGerada *q = questaoAtualPtr();
    return q ? q->tema : QString();
}

QString QuizEngine::enunciadoAtual() const {
    const QuestaoGerada *q = questaoAtualPtr();
    return q ? q->enunciado : QString();
}

QString QuizEngine::tipoEntradaAtual() const {
    const QuestaoGerada *q = questaoAtualPtr();
    return q ? q->tipoEntrada : QStringLiteral("opcoes");
}

QStringList QuizEngine::opcoesAtuais() const {
    const QuestaoGerada *q = questaoAtualPtr();
    return q ? q->opcoes : QStringList();
}

QStringList QuizEngine::itensSomatorioAtual() const {
    const QuestaoGerada *q = questaoAtualPtr();
    return q ? q->itensSomatorio : QStringList();
}

QString QuizEngine::svgAtual() const {
    const QuestaoGerada *q = questaoAtualPtr();
    return q ? q->svgIlustracao : QString();
}

void QuizEngine::confirmarResposta(const QString &respostaUsuario) {
    if (m_respondida) return;
    const QuestaoGerada *q = questaoAtualPtr();
    if (!q) return;

    bool correta = false;
    if (q->tipoEntrada == QStringLiteral("somatorio")) {
        bool ok = false;
        const int valor = respostaUsuario.trimmed().toInt(&ok);
        correta = ok && (valor == q->somaEsperada);
    } else {
        correta = (normalizarResposta(respostaUsuario) == normalizarResposta(q->respostaCorreta));
    }

    m_respondida = true;
    m_acertouUltima = correta;

    if (correta) {
        m_acertosRodada++;
        m_feedbackTitulo = QStringLiteral("Resposta correta!");
        m_feedbackDetalhe = q->explicacaoAcerto;
    } else {
        m_feedbackTitulo = QStringLiteral("Resposta incorreta • Gabarito: %1").arg(q->respostaCorreta);
        m_feedbackDetalhe = q->explicacaoErro;
    }

    QVariantMap registro;
    registro.insert(QStringLiteral("numero"), m_indiceRodada + 1);
    registro.insert(QStringLiteral("tema"), q->tema);
    registro.insert(QStringLiteral("modalidade"), q->modalidade);
    registro.insert(QStringLiteral("enunciado"), q->enunciado);
    registro.insert(QStringLiteral("respostaUsuario"), respostaUsuario.trimmed().isEmpty() ? QStringLiteral("(vazio)") : respostaUsuario.trimmed());
    registro.insert(QStringLiteral("respostaCorreta"), q->respostaCorreta);
    registro.insert(QStringLiteral("acertou"), correta);
    registro.insert(QStringLiteral("explicacao"), correta ? q->explicacaoAcerto : q->explicacaoErro);
    m_historicoRodada.append(registro);

    emit estadoAlterado();
}

void QuizEngine::proximo() {
    if (!m_respondida) return;

    if (m_indiceRodada + 1 < m_rodadaIds.size()) {
        m_indiceRodada++;
        prepararQuestaoAtual();
        emit estadoAlterado();
    } else {
        m_telaAtual = QStringLiteral("resultado");
        emit estadoAlterado();
    }
}

double QuizEngine::percentualFinal() const {
    if (m_rodadaIds.isEmpty()) return 0.0;
    return (static_cast<double>(m_acertosRodada) / static_cast<double>(m_rodadaIds.size())) * 100.0;
}

QString QuizEngine::mensagemDesempenho() const {
    const double pct = percentualFinal();
    if (pct < 60.0) {
        return QStringLiteral("Vamos estudar mais");
    }
    if (pct < 100.0) {
        return QStringLiteral("estamos quase lá, estude mais um poco");
    }
    return QStringLiteral("otimo, estamos pronto para outro assunto");
}

QString QuizEngine::resumoTemas() const {
    QMap<QString, int> totalPorTema;
    QMap<QString, int> acertosPorTema;

    for (const QVariant &itemVar : m_historicoRodada) {
        const QVariantMap item = itemVar.toMap();
        const QString tema = item.value(QStringLiteral("tema")).toString();
        const bool ok = item.value(QStringLiteral("acertou")).toBool();
        totalPorTema[tema] += 1;
        if (ok) acertosPorTema[tema] += 1;
    }

    QStringList pontosFortes;
    QStringList pontosMelhorar;

    for (auto it = totalPorTema.cbegin(); it != totalPorTema.cend(); ++it) {
        const QString tema = it.key();
        const int tot = it.value();
        const int ok = acertosPorTema.value(tema, 0);
        const QString rotulo = QStringLiteral("%1 (%2/%3)").arg(tema).arg(ok).arg(tot);
        if (ok == tot) pontosFortes << rotulo;
        else pontosMelhorar << rotulo;
    }

    const QString forteTxt = pontosFortes.isEmpty() ? QStringLiteral("Nenhum tema 100% ainda") : pontosFortes.join(QStringLiteral(", "));
    const QString melhorarTxt = pontosMelhorar.isEmpty() ? QStringLiteral("Nenhum — tudo dominado!") : pontosMelhorar.join(QStringLiteral(", "));

    return QStringLiteral("Maior facilidade: %1 | Maior dificuldade: %2").arg(forteTxt, melhorarTxt);
}

void QuizEngine::alternarRevisao() {
    m_revisaoVisivel = !m_revisaoVisivel;
    emit estadoAlterado();
}

void QuizEngine::voltarInicio() {
    m_telaAtual = QStringLiteral("inicio");
    m_revisaoVisivel = false;
    emit estadoAlterado();
}
