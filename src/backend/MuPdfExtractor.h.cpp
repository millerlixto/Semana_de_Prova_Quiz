#ifndef MUPDFEXTRACTOR_H
#define MUPDFEXTRACTOR_H

#include <QString>

struct ResultadoExtracaoPdf {
    bool sucesso = false;
    QString texto;
    int paginasProcessadas = 0;
    QString metodoUsado;
    QString erro;
};

class MuPdfExtractor {
public:
    static ResultadoExtracaoPdf extrairTextoDeArquivo(const QString &caminhoArquivo);
    static QString normalizarCaminhoLocal(const QString &caminhoOuUrl);
};

#endif // MUPDFEXTRACTOR_H