#include "MuPdfExtractor.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QRegularExpression>

#ifdef HAVE_MUPDF
extern "C" {
#include <mupdf/fitz.h>
}
#endif

QString MuPdfExtractor::normalizarCaminhoLocal(const QString &caminhoOuUrl) {
    QString limpo = caminhoOuUrl.trimmed();
    if (limpo.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        const QUrl url(limpo);
        if (url.isLocalFile()) {
            limpo = url.toLocalFile();
        } else {
            limpo = limpo.mid(7);
        }
    }
    return limpo;
}

ResultadoExtracaoPdf MuPdfExtractor::extrairTextoDeArquivo(const QString &caminhoArquivo) {
    ResultadoExtracaoPdf res;
    const QString caminho = normalizarCaminhoLocal(caminhoArquivo);

    QFileInfo info(caminho);
    if (caminho.isEmpty() || !info.exists() || !info.isFile()) {
        res.erro = QStringLiteral("Arquivo não encontrado: %1").arg(caminho);
        return res;
    }

#ifdef HAVE_MUPDF
    fz_context *ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!ctx) {
        res.erro = QStringLiteral("Falha ao inicializar contexto MuPDF.");
        return res;
    }

    fz_try(ctx) {
        fz_register_document_handlers(ctx);
        fz_document *doc = fz_open_document(ctx, caminho.toUtf8().constData());
        const int numPaginas = fz_count_pages(ctx, doc);
        QString acumulado;

        for (int p = 0; p < numPaginas; ++p) {
            fz_page *page = fz_load_page(ctx, doc, p);
            fz_stext_page *stext = fz_new_stext_page_from_page(ctx, page, nullptr);

            // Extrai blocos de texto estruturados da página
            for (fz_stext_block *block = stext->first_block; block; block = block->next) {
                if (block->type == FZ_STEXT_BLOCK_TEXT) {
                    for (fz_stext_line *line = block->u.t.first_line; line; line = line->next) {
                        for (fz_stext_char *ch = line->first_char; ch; ch = ch->next) {
                            acumulado.append(QChar(ch->c));
                        }
                        acumulado.append(QLatin1Char(' '));
                    }
                    acumulado.append(QLatin1Char('\n'));
                }
            }

            fz_drop_stext_page(ctx, stext);
            fz_drop_page(ctx, page);
            res.paginasProcessadas++;
        }

        fz_drop_document(ctx, doc);
        res.texto = acumulado.simplified();
        res.metodoUsado = QStringLiteral("MuPDF (fitz)");
        res.sucesso = !res.texto.isEmpty();
        if (!res.sucesso) {
            res.erro = QStringLiteral("O PDF foi aberto pelo MuPDF, mas não contém camada de texto extraível.");
        }
    }
    fz_catch(ctx) {
        res.erro = QStringLiteral("Erro interno do MuPDF ao ler '%1': %2")
                       .arg(info.fileName(), QString::fromUtf8(fz_caught_message(ctx)));
    }

    fz_drop_context(ctx);
    return res;
#else
    QFile file(caminho);
    if (!file.open(QIODevice::ReadOnly)) {
        res.erro = QStringLiteral("Não foi possível abrir o arquivo '%1'.").arg(info.fileName());
        return res;
    }
    const QByteArray raw = file.readAll();
    file.close();

    QString extraido;
    extraido.reserve(raw.size() / 2);
    for (int i = 0; i < raw.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(raw[i]);
        if ((c >= 32 && c <= 126) || c == '\n' || c == '\r' || c == '\t') {
            extraido.append(QChar::fromLatin1(static_cast<char>(c)));
        } else {
            extraido.append(QLatin1Char(' '));
        }
    }
    extraido.remove(QRegularExpression(QStringLiteral("(obj|endobj|stream|endstream|xref|trailer)")));
    res.texto = extraido.simplified();
    res.metodoUsado = QStringLiteral("Extrator local fallback (sem HAVE_MUPDF)");
    res.sucesso = !res.texto.isEmpty();
    if (!res.sucesso) {
        res.erro = QStringLiteral("Nenhum texto legível encontrado em '%1'.").arg(info.fileName());
    }
    return res;
#endif
}
