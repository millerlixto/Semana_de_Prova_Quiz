
#include "MuPdfExtractor.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QByteArray>

#ifdef HAVE_MUPDF
extern "C" {
#include <mupdf/fitz.h>
}
#endif

// --------------------------------------------------
// Normalização de caminhos locais
// --------------------------------------------------

QString MuPdfExtractor::normalizarCaminhoLocal(
    const QString &caminhoOuUrl)
{
    QString caminho = caminhoOuUrl.trimmed();

    if (caminho.isEmpty())
        return QString();

    const QUrl url(caminho);

    if (url.isLocalFile())
        return url.toLocalFile();

    if (caminho.startsWith(
            QStringLiteral("file://"),
            Qt::CaseInsensitive)) {
        return url.toLocalFile();
    }

    return caminho;
}

// --------------------------------------------------
// Extração de texto do PDF
// --------------------------------------------------

ResultadoExtracaoPdf MuPdfExtractor::extrairTextoDeArquivo(
    const QString &caminhoArquivo)
{
    ResultadoExtracaoPdf res;

    const QString caminho =
        normalizarCaminhoLocal(caminhoArquivo);

    if (caminho.isEmpty()) {
        res.erro = QStringLiteral(
            "Caminho do arquivo não informado."
        );
        return res;
    }

    const QFileInfo info(caminho);

    if (!info.exists() || !info.isFile()) {
        res.erro = QStringLiteral(
            "Arquivo não encontrado: %1"
        ).arg(caminho);

        return res;
    }

#ifdef HAVE_MUPDF

    fz_context *ctx = fz_new_context(
        nullptr,
        nullptr,
        FZ_STORE_DEFAULT
    );

    if (!ctx) {
        res.erro = QStringLiteral(
            "Falha ao inicializar contexto MuPDF."
        );

        return res;
    }

    fz_document *doc = nullptr;

    QString textoExtraido;

    const QByteArray caminhoUtf8 =
        QFile::encodeName(caminho);

    fz_try(ctx) {

        // Registra os formatos suportados pelo MuPDF.
        fz_register_document_handlers(ctx);

        // Abre o documento.
        doc = fz_open_document(
            ctx,
            caminhoUtf8.constData()
        );

        if (!doc) {
            fz_throw(
                ctx,
                FZ_ERROR_GENERIC,
                "Não foi possível abrir o documento PDF."
            );
        }

        // Obtém o número de páginas.
        const int totalPaginas =
            fz_count_pages(ctx, doc);

        // Percorre todas as páginas.
        for (int pagina = 0;
             pagina < totalPaginas;
             ++pagina) {

            fz_page *page =
                fz_load_page(ctx, doc, pagina);

            fz_stext_page *stext = nullptr;

            // Extração estruturada da página.
            fz_try(ctx) {

                stext = fz_new_stext_page_from_page(
                    ctx,
                    page,
                    nullptr
                );

                if (!stext) {
                    fz_throw(
                        ctx,
                        FZ_ERROR_GENERIC,
                        "Falha ao extrair texto da página."
                    );
                }

                for (fz_stext_block *block =
                         stext->first_block;
                     block;
                     block = block->next) {

                    if (block->type != FZ_STEXT_BLOCK_TEXT)
                        continue;

                    for (fz_stext_line *line =
                             block->u.t.first_line;
                         line;
                         line = line->next) {

                        for (fz_stext_char *ch =
                                 line->first_char;
                             ch;
                             ch = ch->next) {

                            // Preserva caracteres Unicode.
                            const char32_t codigoUnicode =
                                static_cast<char32_t>(ch->c);

                            textoExtraido.append(
                                QString::fromUcs4(
                                    &codigoUnicode,
                                    1
                                )
                            );
                        }

                        textoExtraido.append(
                            QLatin1Char('\n')
                        );
                    }

                    textoExtraido.append(
                        QLatin1Char('\n')
                    );
                }
            }

            // Libera os recursos mesmo em caso de erro.
            fz_always(ctx) {

                if (stext) {
                    fz_drop_stext_page(
                        ctx,
                        stext
                    );
                }

                if (page) {
                    fz_drop_page(
                        ctx,
                        page
                    );
                }
            }

            fz_catch(ctx) {
                fz_rethrow(ctx);
            }

            res.paginasProcessadas++;
        }

        res.texto = textoExtraido.simplified();

        res.metodoUsado = QStringLiteral(
            "MuPDF (fitz)"
        );

        res.sucesso = !res.texto.isEmpty();

        if (!res.sucesso) {
            res.erro = QStringLiteral(
                "O PDF foi aberto pelo MuPDF, mas não "
                "contém camada de texto extraível."
            );
        }
    }

    // Libera o documento.
    fz_always(ctx) {

        if (doc) {
            fz_drop_document(ctx, doc);
            doc = nullptr;
        }
    }

    // Captura erros do MuPDF.
    fz_catch(ctx) {

        res.sucesso = false;

        res.erro = QStringLiteral(
            "Erro interno do MuPDF ao ler '%1': %2"
        ).arg(
            info.fileName(),
            QString::fromUtf8(
                fz_caught_message(ctx)
            )
        );
    }

    fz_drop_context(ctx);

    return res;

#else

    // --------------------------------------------------
    // Fallback sem MuPDF
    // --------------------------------------------------

    QFile file(caminho);

    if (!file.open(QIODevice::ReadOnly)) {

        res.erro = QStringLiteral(
            "Não foi possível abrir o arquivo '%1'."
        ).arg(info.fileName());

        return res;
    }

    const QByteArray raw = file.readAll();

    file.close();

    if (!raw.startsWith("%PDF-")) {

        res.erro = QStringLiteral(
            "O arquivo informado não possui assinatura PDF válida."
        );

        return res;
    }

    res.metodoUsado = QStringLiteral(
        "Verificação local (MuPDF desabilitado)"
    );

    res.sucesso = false;

    res.erro = QStringLiteral(
        "A extração real de texto requer a biblioteca MuPDF. "
        "O projeto foi compilado sem HAVE_MUPDF."
    );

    return res;

#endif
}
