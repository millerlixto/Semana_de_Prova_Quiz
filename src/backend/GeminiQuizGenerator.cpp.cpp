#include "GeminiQuizGenerator.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QRegularExpression>

namespace {
const QStringList NIVEIS = {
    QStringLiteral("Muito Fácil"),
    QStringLiteral("Fácil"),
    QStringLiteral("Difícil")
};

const QStringList MODALIDADES = {
    QStringLiteral("Múltipla escolha tradicional"),
    QStringLiteral("Certo ou Errado"),
    QStringLiteral("Foco negativo"),
    QStringLiteral("Asserção e Razão"),
    QStringLiteral("Lacuna"),
    QStringLiteral("Associação / Correspondência"),
    QStringLiteral("Ordenação ou Seriação"),
    QStringLiteral("Somatório"),
    QStringLiteral("Numéricas / Resposta curta fechada")
};
} // namespace

GeminiQuizGenerator::GeminiQuizGenerator(QObject *parent) : QObject(parent) {}

QString GeminiQuizGenerator::gerarSvgTematico(const QString &tema, const QString &modalidade, int seed) {
    const int cx = 55 + ((seed * 29) % 80);
    const int rx = 185 + ((seed * 17) % 65);
    const int cy = 42 + ((seed * 13) % 24);
    const QString cor1 = (seed % 2 == 0) ? QStringLiteral("#8ab4f8") : QStringLiteral("#c58af9");
    const QString cor2 = (seed % 3 == 0) ? QStringLiteral("#81c995") : QStringLiteral("#f28b82");

    return QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 360 120' width='360' height='120'>"
        "<rect x='0' y='0' width='360' height='120' rx='14' fill='#1e1f24'/>"
        "<rect x='14' y='14' width='332' height='92' rx='10' fill='#282a30' stroke='#3c4043' stroke-width='1.5'/>"
        "<circle cx='%1' cy='%2' r='16' fill='%3' opacity='0.85'/>"
        "<rect x='%4' y='32' width='78' height='36' rx='8' fill='%5' opacity='0.85'/>"
        "<line x1='%6' y1='%2' x2='%4' y2='50' stroke='#e8eaed' stroke-width='2' stroke-dasharray='4 3'/>"
        "<text x='180' y='98' text-anchor='middle' fill='#bdc1c6' font-family='sans-serif' font-size='11'>%7 • %8</text>"
        "</svg>"
    )
        .arg(cx)
        .arg(cy)
        .arg(cor1)
        .arg(rx)
        .arg(cor2)
        .arg(cx + 16)
        .arg(tema.toHtmlEscaped())
        .arg(modalidade.toHtmlEscaped());
}

QString GeminiQuizGenerator::montarPromptEstrito(const QString &textoPdf, const QString &nomeArquivo) {
    const QString limitado = textoPdf.left(28000);
    return QStringLiteral(
        "Você é um gerador de quiz estritamente fiel ao documento '%1'. "
        "Gere EXATAMENTE 120 questões únicas (40 em 'Muito Fácil', 40 em 'Fácil', 40 em 'Difícil') "
        "baseadas EXCLUSIVAMENTE no conteúdo abaixo, sem adicionar informação externa. "
        "Distribua as 9 modalidades (Múltipla escolha tradicional, Certo ou Errado, Foco negativo, "
        "Asserção e Razão, Lacuna, Associação / Correspondência, Ordenação ou Seriação, Somatório, "
        "Numéricas / Resposta curta fechada), com alternativas equilibradas em tamanho e explicações objetivas.\n\n"
        "Responda APENAS com um JSON válido no formato:\n"
        "{\"questoes\":[{\"nivel\":\"Muito Fácil\",\"modalidade\":\"...\",\"tema\":\"...\","
        "\"enunciado\":\"...\",\"tipoEntrada\":\"opcoes\",\"opcoes\":[\"a) ...\",\"b) ...\",\"c) ...\",\"d) ...\"],"
        "\"respostaCorreta\":\"a) ...\",\"somaEsperada\":0,\"itensSomatorio\":[],"
        "\"explicacaoAcerto\":\"...\",\"explicacaoErro\":\"...\"}]}\n\n"
        "CONTEÚDO DO PDF:\n%2"
    ).arg(nomeArquivo, limitado);
}

QVector<QuestaoGerada> GeminiQuizGenerator::parsearJsonQuestoes(const QString &jsonBruto, const QString &nomeArquivo) {
    QString limpo = jsonBruto.trimmed();
    if (limpo.startsWith(QStringLiteral("```"))) {
        limpo.remove(QRegularExpression(QStringLiteral("^```[a-zA-Z]*\\s*")));
        limpo.remove(QRegularExpression(QStringLiteral("\\s*```$")));
    }

    const QJsonDocument doc = QJsonDocument::fromJson(limpo.toUtf8());
    QVector<QuestaoGerada> saida;
    if (!doc.isObject()) return saida;

    const QJsonArray arr = doc.object().value(QStringLiteral("questoes")).toArray();
    int idSeq = 1;

    for (const QJsonValue &v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject obj = v.toObject();

        QuestaoGerada q;
        q.id = idSeq++;
        q.nivel = obj.value(QStringLiteral("nivel")).toString(QStringLiteral("Muito Fácil"));
        q.modalidade = obj.value(QStringLiteral("modalidade")).toString(QStringLiteral("Múltipla escolha tradicional"));
        q.tema = obj.value(QStringLiteral("tema")).toString(nomeArquivo);
        q.enunciado = obj.value(QStringLiteral("enunciado")).toString().trimmed();
        q.tipoEntrada = obj.value(QStringLiteral("tipoEntrada")).toString(QStringLiteral("opcoes"));
        q.respostaCorreta = obj.value(QStringLiteral("respostaCorreta")).toString().trimmed();
        q.somaEsperada = obj.value(QStringLiteral("somaEsperada")).toInt(0);
        q.explicacaoAcerto = obj.value(QStringLiteral("explicacaoAcerto")).toString().trimmed();
        q.explicacaoErro = obj.value(QStringLiteral("explicacaoErro")).toString().trimmed();

        for (const QJsonValue &op : obj.value(QStringLiteral("opcoes")).toArray()) {
            q.opcoes << op.toString();
        }
        for (const QJsonValue &it : obj.value(QStringLiteral("itensSomatorio")).toArray()) {
            q.itensSomatorio << it.toString();
        }

        if (!q.enunciado.isEmpty() && !q.respostaCorreta.isEmpty()) {
            q.svgIlustracao = gerarSvgTematico(q.tema, q.modalidade, q.id);
            saida.append(q);
        }
    }
    return saida;
}

QVector<QuestaoGerada> GeminiQuizGenerator::gerarBancoEstritoDoTexto(const QString &textoPdf,
                                                                     const QString &nomeArquivo) {
    QVector<QuestaoGerada> banco;

    // Extrai sentenças/trechos reais do texto do PDF para construir questões estritamente fiéis ao material
    QStringList sentencas = textoPdf.split(QRegularExpression(QStringLiteral("[\\.!\\?\\n]+")), Qt::SkipEmptyParts);
    QStringList trechosLimpos;
    for (const QString &s : sentencas) {
        const QString t = s.trimmed();
        if (t.size() >= 25) {
            trechosLimpos << t.left(180);
        }
    }
    if (trechosLimpos.isEmpty()) {
        trechosLimpos << textoPdf.simplified().left(180);
    }

    const QStringList temas = {
        QStringLiteral("Conceitos do PDF"),
        QStringLiteral("Definições e Termos"),
        QStringLiteral("Processos e Relações"),
        QStringLiteral("Síntese do Documento")
    };

    int idGlobal = 1;
    for (int idxNivel = 0; idxNivel < NIVEIS.size(); ++idxNivel) {
        const QString nivel = NIVEIS[idxNivel];

        for (int i = 0; i < 40; ++i) {
            const QString modalidade = MODALIDADES[i % MODALIDADES.size()];
            const QString tema = temas[i % temas.size()];
            const QString trechoCerto = trechosLimpos[(i + idxNivel * 7) % trechosLimpos.size()];
            const QString trechoOutro1 = trechosLimpos[(i + idxNivel * 7 + 1) % trechosLimpos.size()];
            const QString trechoOutro2 = trechosLimpos[(i + idxNivel * 7 + 2) % trechosLimpos.size()];
            const QString trechoOutro3 = trechosLimpos[(i + idxNivel * 7 + 3) % trechosLimpos.size()];

            QuestaoGerada q;
            q.id = idGlobal++;
            q.nivel = nivel;
            q.modalidade = modalidade;
            q.tema = tema;
            q.svgIlustracao = gerarSvgTematico(tema, modalidade, q.id);

            if (modalidade == QStringLiteral("Certo ou Errado")) {
                q.tipoEntrada = QStringLiteral("certo_errado");
                const bool verdadeiro = ((i + idxNivel) % 2 == 0);
                q.enunciado = verdadeiro
                    ? QStringLiteral("[%1 • %2] O documento '%3' registra o seguinte trecho: \"%4\".")
                          .arg(nivel, tema, nomeArquivo, trechoCerto)
                    : QStringLiteral("[%1 • %2] O documento '%3' afirma que \"%4\" é uma informação ausente e contraditória ao trecho \"%5\".")
                          .arg(nivel, tema, nomeArquivo, trechoOutro1, trechoCerto);
                q.respostaCorreta = verdadeiro ? QStringLiteral("Certo") : QStringLiteral("Errado");
                q.explicacaoAcerto = QStringLiteral("Correto conforme '%1': o trecho consta no material analisado.").arg(nomeArquivo);
                q.explicacaoErro = QStringLiteral("Incorreto conforme '%1': verifique o trecho original \"%2\" no documento.").arg(nomeArquivo, trechoCerto);

            } else if (modalidade == QStringLiteral("Somatório")) {
                q.tipoEntrada = QStringLiteral("somatorio");
                q.enunciado = QStringLiteral("[%1 • %2] Em '%3', some os códigos das afirmações fiéis ao texto (01, 02, 04, 08):")
                                  .arg(nivel, tema, nomeArquivo);
                q.itensSomatorio = {
                    QStringLiteral("01 - O texto do documento contém: \"%1\".").arg(trechoCerto.left(75)),
                    QStringLiteral("02 - O material carregado corresponde ao documento '%1'.").arg(nomeArquivo),
                    QStringLiteral("04 - As questões deste nível seguem estritamente o PDF carregado."),
                    QStringLiteral("08 - O conteúdo foi inventado fora do documento enviado.")
                };
                q.somaEsperada = 7; // 01 + 02 + 04
                q.respostaCorreta = QStringLiteral("07");
                q.explicacaoAcerto = QStringLiteral("Soma 07 correta (01 + 02 + 04): apenas as três primeiras afirmações condizem com '%1'.").arg(nomeArquivo);
                q.explicacaoErro = QStringLiteral("Soma correta é 07 (01 + 02 + 04). A afirmação 08 não corresponde ao conteúdo do PDF.").arg(nomeArquivo);

            } else if (modalidade == QStringLiteral("Numéricas / Resposta curta fechada")) {
                q.tipoEntrada = QStringLiteral("texto_curto");
                q.enunciado = QStringLiteral("[%1 • %2] Quantas questões compõem cada nível exclusivo no banco gerado a partir de '%3'?")
                                  .arg(nivel, tema, nomeArquivo);
                q.respostaCorreta = QStringLiteral("40");
                q.explicacaoAcerto = QStringLiteral("Correto: cada nível possui exatamente 40 questões exclusivas.");
                q.explicacaoErro = QStringLiteral("Resposta esperada: 40 (são 40 questões por nível, totalizando 120).");

            } else if (modalidade == QStringLiteral("Foco negativo")) {
                q.tipoEntrada = QStringLiteral("opcoes");
                q.enunciado = QStringLiteral("[%1 • %2] Sobre '%3', assinale a alternativa INCORRETA:")
                                  .arg(nivel, tema, nomeArquivo);
                q.opcoes = {
                    QStringLiteral("a) O banco possui 40 questões no nível %1").arg(nivel),
                    QStringLiteral("b) O conteúdo base vem do documento %1").arg(nomeArquivo),
                    QStringLiteral("c) O sistema ignora o PDF e usa regras externas"),
                    QStringLiteral("d) O ciclo evita repetir questões já vistas")
                };
                q.respostaCorreta = q.opcoes[2];
                q.explicacaoAcerto = QStringLiteral("Correto! A (c) é INCORRETA porque o quiz se baseia no documento carregado.");
                q.explicacaoErro = QStringLiteral("A alternativa incorreta é (c): o banco é construído a partir do PDF/conteúdo carregado.");

            } else {
                q.tipoEntrada = QStringLiteral("opcoes");
                q.enunciado = QStringLiteral("[%1 • %2 • %3] Qual trecho pertence fielmente ao documento '%4'?")
                                  .arg(nivel, tema, modalidade, nomeArquivo);
                q.opcoes = {
                    QStringLiteral("a) %1").arg(trechoCerto.left(80)),
                    QStringLiteral("b) %1").arg(QStringLiteral("Trecho não registrado: %1").arg(trechoOutro1.left(55))),
                    QStringLiteral("c) %1").arg(QStringLiteral("Informação externa: %1").arg(trechoOutro2.left(55))),
                    QStringLiteral("d) %1").arg(QStringLiteral("Hipótese não citada: %1").arg(trechoOutro3.left(55)))
                };
                q.respostaCorreta = q.opcoes[0];
                q.explicacaoAcerto = QStringLiteral("Correto! Esse trecho aparece diretamente no documento '%1'.").arg(nomeArquivo);
                q.explicacaoErro = QStringLiteral("Incorreto. O trecho fiel ao documento '%1' é a alternativa (a): \"%2\".").arg(nomeArquivo, trechoCerto.left(90));
            }

            if (q.tipoEntrada == QStringLiteral("opcoes") && q.opcoes.size() == 4) {
                const int rot = (i + idxNivel) % 4;
                QStringList giradas;
                for (int k = 0; k < 4; ++k) {
                    const QString semPrefixo = q.opcoes[(k + rot) % 4].mid(3);
                    giradas << QStringLiteral("%1) %2").arg(QLatin1Char('a' + k)).arg(semPrefixo);
                }
                const QString alvoSemPrefixo = q.respostaCorreta.mid(3);
                for (const QString &op : std::as_const(giradas)) {
                    if (op.mid(3) == alvoSemPrefixo) {
                        q.respostaCorreta = op;
                        break;
                    }
                }
                q.opcoes = giradas;
            }

            banco.append(q);
        }
    }

    return banco;
}

void GeminiQuizGenerator::solicitarBanco120Questoes(const QString &textoPdf,
                                                     const QString &nomeArquivo,
                                                     const QString &apiKey) {
    const QString chave = apiKey.trimmed();
    if (chave.isEmpty()) {
        QVector<QuestaoGerada> local = gerarBancoEstritoDoTexto(textoPdf, nomeArquivo);
        emit bancoPronto(local, QStringLiteral("120 questões geradas localmente a partir do texto extraído de '%1'.").arg(nomeArquivo));
        return;
    }

    const QUrl endpoint(QStringLiteral("[https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=%1](https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=%1)")
                            .arg(chave));

    QNetworkRequest req(endpoint);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject partObj;
    partObj.insert(QStringLiteral("text"), montarPromptEstrito(textoPdf, nomeArquivo));

    QJsonArray partsArr;
    partsArr.append(partObj);

    QJsonObject contentObj;
    contentObj.insert(QStringLiteral("parts"), partsArr);

    QJsonArray contentsArr;
    contentsArr.append(contentObj);

    QJsonObject genConfig;
    genConfig.insert(QStringLiteral("responseMimeType"), QStringLiteral("application/json"));

    QJsonObject rootObj;
    rootObj.insert(QStringLiteral("contents"), contentsArr);
    rootObj.insert(QStringLiteral("generationConfig"), genConfig);

    QNetworkReply *reply = m_network.post(req, QJsonDocument(rootObj).toJson(QJsonDocument::Compact));

    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, textoPdf, nomeArquivo]() {
        reply->deleteLater();
        QVector<QuestaoGerada> questoes;

        if (reply->error() == QNetworkReply::NoError) {
            const QJsonDocument respDoc = QJsonDocument::fromJson(reply->readAll());
            const QJsonArray candidates = respDoc.object().value(QStringLiteral("candidates")).toArray();
            if (!candidates.isEmpty()) {
                const QJsonArray parts = candidates.first().toObject()
                                               .value(QStringLiteral("content")).toObject()
                                               .value(QStringLiteral("parts")).toArray();
                if (!parts.isEmpty()) {
                    const QString textoJson = parts.first().toObject().value(QStringLiteral("text")).toString();
                    questoes = parsearJsonQuestoes(textoJson, nomeArquivo);
                }
            }
        }

        if (questoes.size() >= 120) {
            questoes.resize(120);
            for (int i = 0; i < 120; ++i) questoes[i].id = i + 1;
            emit bancoPronto(questoes, QStringLiteral("120 questões geradas via API Gemini a partir de '%1'.").arg(nomeArquivo));
        } else {
            QVector<QuestaoGerada> fallback = gerarBancoEstritoDoTexto(textoPdf, nomeArquivo);
            emit bancoPronto(fallback, QStringLiteral("120 questões geradas do PDF '%1' (fallback estruturado aplicado).").arg(nomeArquivo));
        }
    });
}