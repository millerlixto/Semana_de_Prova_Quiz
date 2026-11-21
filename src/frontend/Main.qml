import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: root
    visible: true
    width: 412
    height: 820
    title: "Gemini Quiz Studio"
    color: "#131314"

    property string respostaSelecionada: ""

    function resetarSelecao() {
        respostaSelecionada = ""
        campoResposta.text = ""
    }

    FileDialog {
        id: seletorPdf
        title: "Selecione o PDF de Estudo"
        nameFilters: ["Arquivos PDF (*.pdf)", "Todos os arquivos (*)"]
        onAccepted: {
            quizEngine.carregarPdfArquivo(selectedFile.toString(), campoApiKey.text)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: quizEngine.telaAtual === "quiz" ? 132 : 84
            color: "#1e1f20"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: "#a8c7fa"
                    }

                    Text {
                        text: "Gemini Quiz"
                        color: "#e3e3e3"
                        font.pixelSize: 18
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Button {
                        id: btnNext
                        visible: quizEngine.telaAtual === "quiz"
                        text: quizEngine.indiceRodada + 1 >= quizEngine.totalRodada ? "Finalizar" : "Next"
                        enabled: quizEngine.podeAvancar
                        onClicked: {
                            resetarSelecao()
                            quizEngine.proximo()
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: ["Muito Fácil", "Fácil", "Difícil"]
                        delegate: Button {
                            Layout.fillWidth: true
                            text: modelData
                            highlighted: quizEngine.nivelAtual === modelData
                            onClicked: {
                                resetarSelecao()
                                quizEngine.selecionarNivel(modelData)
                                if (quizEngine.telaAtual === "quiz") {
                                    quizEngine.iniciarRodada(modelData)
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    visible: quizEngine.telaAtual === "quiz"
                    Layout.fillWidth: true

                    Text {
                        text: "Questão " + (quizEngine.indiceRodada + 1) + "/" + quizEngine.totalRodada
                              + " • " + quizEngine.modalidadeAtual
                        color: "#c4c7c5"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Text {
                        text: quizEngine.resumoCiclo
                        color: "#9aa0a6"
                        font.pixelSize: 11
                    }

                    Button {
                        text: "Zerar ciclo deste nível"
                        flat: true
                        onClicked: {
                            resetarSelecao()
                            quizEngine.zerarCicloNivel(quizEngine.nivelAtual)
                        }
                    }
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 14
                anchors.margins: 16

                // TELA INICIAL
                ColumnLayout {
                    visible: quizEngine.telaAtual === "inicio"
                    Layout.fillWidth: true
                    spacing: 12

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 18
                        color: "#1e1f20"
                        border.color: "#333537"
                        implicitHeight: colInicio.implicitHeight + 32

                        ColumnLayout {
                            id: colInicio
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 10

                            Text {
                                text: "Gerador de Quiz Interativo Ilustrado"
                                color: "#e3e3e3"
                                font.pixelSize: 20
                                font.bold: true
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            Text {
                                text: "Documento ativo: " + quizEngine.nomeDocumento
                                color: "#c4c7c5"
                                font.pixelSize: 12
                            }

                            Text {
                                text: quizEngine.statusDocumento
                                color: "#a8c7fa"
                                font.pixelSize: 12
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            TextField {
                                id: campoApiKey
                                Layout.fillWidth: true
                                echoMode: TextInput.Password
                                placeholderText: "Chave API Gemini (opcional — se vazio, gera localmente do PDF)"
                            }

                            Button {
                                text: quizEngine.gerandoBanco ? "Gerando 120 questões..." : "Selecionar Arquivo PDF de Estudo"
                                Layout.fillWidth: true
                                enabled: !quizEngine.gerandoBanco
                                onClicked: seletorPdf.open()
                            }

                            TextArea {
                                id: campoTextoExtra
                                Layout.fillWidth: true
                                Layout.preferredHeight: 86
                                placeholderText: "Ou cole/digite aqui o texto base do PDF..."
                                wrapMode: TextArea.Wrap
                                color: "#e3e3e3"
                            }

                            Button {
                                text: "Gerar 120 Questões do Texto"
                                Layout.fillWidth: true
                                enabled: !quizEngine.gerandoBanco
                                onClicked: {
                                    quizEngine.gerarDeTexto(campoTextoExtra.text, "Texto de Estudo", campoApiKey.text)
                                }
                            }

                            Button {
                                text: "Iniciar Rodada (10 Questões - " + quizEngine.nivelAtual + ")"
                                Layout.fillWidth: true
                                highlighted: true
                                enabled: !quizEngine.gerandoBanco
                                onClicked: {
                                    resetarSelecao()
                                    quizEngine.iniciarRodada(quizEngine.nivelAtual)
                                }
                            }
                        }
                    }
                }

                // TELA DO QUIZ
                ColumnLayout {
                    visible: quizEngine.telaAtual === "quiz"
                    Layout.fillWidth: true
                    spacing: 12

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 18
                        color: "#1e1f20"
                        border.color: "#333537"
                        implicitHeight: colQuestao.implicitHeight + 28

                        ColumnLayout {
                            id: colQuestao
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 10

                            Text {
                                text: "Nível: " + quizEngine.nivelAtual + "  |  Tema: " + quizEngine.temaAtual
                                color: "#a8c7fa"
                                font.pixelSize: 12
                                font.bold: true
                            }

                            Image {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 120
                                fillMode: Image.PreserveAspectFit
                                source: "data:image/svg+xml;utf8," + encodeURIComponent(quizEngine.svgAtual)
                            }

                            Text {
                                text: quizEngine.enunciadoAtual
                                color: "#e3e3e3"
                                font.pixelSize: 15
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            ColumnLayout {
                                visible: quizEngine.tipoEntradaAtual === "somatorio"
                                Layout.fillWidth: true
                                spacing: 4

                                Repeater {
                                    model: quizEngine.itensSomatorioAtual
                                    delegate: Text {
                                        text: modelData
                                        color: "#c4c7c5"
                                        font.pixelSize: 13
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }

                            ColumnLayout {
                                visible: quizEngine.tipoEntradaAtual === "opcoes"
                                Layout.fillWidth: true
                                spacing: 8

                                Repeater {
                                    model: quizEngine.opcoesAtuais
                                    delegate: Button {
                                        Layout.fillWidth: true
                                        text: modelData
                                        enabled: !quizEngine.respondida
                                        highlighted: root.respostaSelecionada === modelData
                                        onClicked: {
                                            root.respostaSelecionada = modelData
                                            quizEngine.confirmarResposta(modelData)
                                        }
                                    }
                                }
                            }

                            RowLayout {
                                visible: quizEngine.tipoEntradaAtual === "certo_errado"
                                Layout.fillWidth: true
                                spacing: 10

                                Button {
                                    Layout.fillWidth: true
                                    text: "Certo"
                                    enabled: !quizEngine.respondida
                                    onClicked: quizEngine.confirmarResposta("Certo")
                                }

                                Button {
                                    Layout.fillWidth: true
                                    text: "Errado"
                                    enabled: !quizEngine.respondida
                                    onClicked: quizEngine.confirmarResposta("Errado")
                                }
                            }

                            RowLayout {
                                visible: quizEngine.tipoEntradaAtual === "somatorio" || quizEngine.tipoEntradaAtual === "texto_curto"
                                Layout.fillWidth: true
                                spacing: 8

                                TextField {
                                    id: campoResposta
                                    Layout.fillWidth: true
                                    enabled: !quizEngine.respondida
                                    placeholderText: quizEngine.tipoEntradaAtual === "somatorio"
                                        ? "Digite a soma (ex.: 07)"
                                        : "Digite sua resposta"
                                }

                                Button {
                                    text: "Confirmar"
                                    enabled: !quizEngine.respondida && campoResposta.text.trim().length > 0
                                    onClicked: quizEngine.confirmarResposta(campoResposta.text)
                                }
                            }

                            Rectangle {
                                visible: quizEngine.respondida
                                Layout.fillWidth: true
                                radius: 12
                                color: quizEngine.acertouUltima ? "#132a1c" : "#2c1618"
                                border.color: quizEngine.acertouUltima ? "#34a853" : "#ea4335"
                                implicitHeight: colFeedback.implicitHeight + 20

                                ColumnLayout {
                                    id: colFeedback
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 4

                                    Text {
                                        text: quizEngine.feedbackTitulo
                                        color: quizEngine.acertouUltima ? "#81c995" : "#f28b82"
                                        font.bold: true
                                        font.pixelSize: 14
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                    }

                                    Text {
                                        text: quizEngine.feedbackDetalhe
                                        color: "#e3e3e3"
                                        font.pixelSize: 13
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                    }
                                }
                            }
                        }
                    }
                }

                // TELA FINAL
                ColumnLayout {
                    visible: quizEngine.telaAtual === "resultado"
                    Layout.fillWidth: true
                    spacing: 12

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 18
                        color: "#1e1f20"
                        border.color: "#333537"
                        implicitHeight: colResultado.implicitHeight + 30

                        ColumnLayout {
                            id: colResultado
                            anchors.fill: parent
                            anchors.margins: 15
                            spacing: 10

                            Text {
                                text: "Resultado da Rodada"
                                color: "#e3e3e3"
                                font.pixelSize: 20
                                font.bold: true
                            }

                            Text {
                                text: "Placar: " + quizEngine.acertosRodada + "/" + quizEngine.totalRodada
                                      + " (" + quizEngine.percentualFinal.toFixed(0) + "%)"
                                color: "#a8c7fa"
                                font.pixelSize: 18
                                font.bold: true
                            }

                            Text {
                                text: quizEngine.mensagemDesempenho
                                color: "#e3e3e3"
                                font.pixelSize: 15
                                font.bold: true
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            Text {
                                text: quizEngine.resumoTemas
                                color: "#c4c7c5"
                                font.pixelSize: 13
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            Text {
                                text: quizEngine.resumoCiclo
                                color: "#9aa0a6"
                                font.pixelSize: 12
                            }

                            Button {
                                text: "Próxima Rodada (10 Novas Questões)"
                                Layout.fillWidth: true
                                highlighted: true
                                onClicked: {
                                    resetarSelecao()
                                    quizEngine.reiniciarQuiz()
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Button {
                                    Layout.fillWidth: true
                                    text: "Trocar Nível"
                                    onClicked: {
                                        resetarSelecao()
                                        quizEngine.voltarInicio()
                                    }
                                }

                                Button {
                                    Layout.fillWidth: true
                                    text: "Zerar Ciclo deste Nível"
                                    onClicked: {
                                        resetarSelecao()
                                        quizEngine.zerarCicloNivel(quizEngine.nivelAtual)
                                    }
                                }
                            }

                            Button {
                                text: "Revisar Respostas desta Rodada"
                                Layout.fillWidth: true
                                onClicked: quizEngine.alternarRevisao()
                            }

                            ColumnLayout {
                                visible: quizEngine.revisaoVisivel
                                Layout.fillWidth: true
                                spacing: 8

                                Repeater {
                                    model: quizEngine.historicoRodada
                                    delegate: Rectangle {
                                        Layout.fillWidth: true
                                        radius: 10
                                        color: "#282a2c"
                                        implicitHeight: colRev.implicitHeight + 16

                                        ColumnLayout {
                                            id: colRev
                                            anchors.fill: parent
                                            anchors.margins: 8
                                            spacing: 3

                                            Text {
                                                text: "#" + modelData.numero + " [" + modelData.tema + "] "
                                                      + (modelData.acertou ? "Acertou" : "Errou")
                                                color: modelData.acertou ? "#81c995" : "#f28b82"
                                                font.bold: true
                                                font.pixelSize: 12
                                            }
                                            Text {
                                                text: modelData.enunciado
                                                color: "#e3e3e3"
                                                font.pixelSize: 12
                                                Layout.fillWidth: true
                                                wrapMode: Text.WordWrap
                                            }
                                            Text {
                                                text: "Sua resposta: " + modelData.respostaUsuario
                                                      + " | Correta: " + modelData.respostaCorreta
                                                color: "#c4c7c5"
                                                font.pixelSize: 11
                                                Layout.fillWidth: true
                                                wrapMode: Text.WordWrap
                                            }
                                            Text {
                                                text: modelData.explicacao
                                                color: "#9aa0a6"
                                                font.pixelSize: 11
                                                Layout.fillWidth: true
                                                wrapMode: Text.WordWrap
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
