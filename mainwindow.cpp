#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>

#include <QtCharts>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>

#include <vector>
#include <algorithm>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboEfeitos->addItem("Aumentar Brilho");
    ui->comboEfeitos->addItem("Requantizar");
    ui->comboEfeitos->addItem("Tons de Cinza (Canal R)");
    ui->comboEfeitos->addItem("Tons de Cinza (Canal G)");
    ui->comboEfeitos->addItem("Tons de Cinza (Canal B)");
    ui->comboEfeitos->addItem("Desligar Canais RGB (1=-R, 2=-G, 3=-B, 4=Só B, 5=Só G, 6=Só R)");
    ui->comboEfeitos->addItem("Binarização");
    ui->comboEfeitos->addItem("Filtro da Mediana");
    ui->comboEfeitos->addItem("Filtro da Média");
    ui->comboEfeitos->addItem("Filtro da Moda");
    ui->comboEfeitos->addItem("Filtro Gaussiano");
    ui->comboEfeitos->addItem("Filtro K-Nearest (k vizinhos mais próximos)");
    ui->comboEfeitos->addItem("Rotação (Use o parâmetro para o ângulo)");
    ui->comboEfeitos->addItem("Espelhamento (1=Horiz, 2=Vert)");
    ui->comboEfeitos->addItem("Aplicar Máscara (Requer carregamento)");
    ui->comboEfeitos->addItem("Filtro Passa-Altas: Laplaciano (v1)");
    ui->comboEfeitos->addItem("Filtro Passa-Altas: Laplaciano (v2)");
    ui->comboEfeitos->addItem("Realce de Nitidez (Sharpening Laplaciano)");
    ui->comboEfeitos->addItem("Detector de Bordas Gradiente (Sobel)");
    ui->comboEfeitos->addItem("Equalização de Histograma");
    ui->comboEfeitos->addItem("ASCII Art (Use o parâmetro para o tamanho)");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::desenharHistograma(const QImage &imagem)
{
    int histR[256] = {0}, histG[256] = {0}, histB[256] = {0};
    int valorMaximo = 0;

    for (int y = 0; y < imagem.height(); ++y) {
        const QRgb *linha = (const QRgb *)imagem.constScanLine(y);
        for (int x = 0; x < imagem.width(); ++x) {
            QRgb cor = linha[x];
            histR[qRed(cor)]++;
            histG[qGreen(cor)]++;
            histB[qBlue(cor)]++;
            valorMaximo = std::max({valorMaximo, histR[qRed(cor)], histG[qGreen(cor)], histB[qBlue(cor)]});
        }
    }

    QLineSeries *serieR = new QLineSeries(); serieR->setName("Red"); serieR->setColor(QColor(255, 50, 50, 200));
    QLineSeries *serieG = new QLineSeries(); serieG->setName("Green"); serieG->setColor(QColor(50, 200, 50, 200));
    QLineSeries *serieB = new QLineSeries(); serieB->setName("Blue"); serieB->setColor(QColor(50, 50, 255, 200));

    for (int i = 0; i < 256; ++i) {
        serieR->append(i, histR[i]);
        serieG->append(i, histG[i]);
        serieB->append(i, histB[i]);
    }

    QChart *grafico = new QChart();
    grafico->addSeries(serieR);
    grafico->addSeries(serieG);
    grafico->addSeries(serieB);
    grafico->setTitle("Histograma RGB");
    grafico->setAnimationOptions(QChart::SeriesAnimations);

    QValueAxis *eixoX = new QValueAxis();
    eixoX->setRange(0, 255);
    eixoX->setTitleText("Intensidade da Cor");
    eixoX->setLabelFormat("%d");
    eixoX->setTickCount(6);
    grafico->addAxis(eixoX, Qt::AlignBottom);
    serieR->attachAxis(eixoX); serieG->attachAxis(eixoX); serieB->attachAxis(eixoX);

    QValueAxis *eixoY = new QValueAxis();
    eixoY->setRange(0, valorMaximo);
    eixoY->setTitleText("Frequência (Pixels)");
    grafico->addAxis(eixoY, Qt::AlignLeft);
    serieR->attachAxis(eixoY); serieG->attachAxis(eixoY); serieB->attachAxis(eixoY);

    QChartView *visualizadorGrafico = new QChartView(grafico);
    visualizadorGrafico->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->containerHistograma->layout());
    if (layout == nullptr) {
        layout = new QVBoxLayout(ui->containerHistograma);
        layout->setContentsMargins(0, 0, 0, 0);
    } else {
        QLayoutItem *itemAntigo = layout->takeAt(0);
        if (itemAntigo != nullptr) {
            delete itemAntigo->widget();
            delete itemAntigo;
        }
    }
    layout->addWidget(visualizadorGrafico);
}

void MainWindow::on_btnCarregar_clicked()
{
    QString caminhoArquivo = QFileDialog::getOpenFileName(
        this,
        "Selecione uma Imagem",
        "",
        "Imagens (*.png *.jpg *.jpeg *.bmp)"
        );

    if (caminhoArquivo.isEmpty()) return;

    imagemCarregada = QImage(caminhoArquivo).convertToFormat(QImage::Format_RGB32);

    if (imagemCarregada.isNull()) {
        QMessageBox::warning(this, "Erro", "Não foi possível carregar a imagem.");
        return;
    }

    ui->labelOriginal->setPixmap(QPixmap::fromImage(imagemCarregada));
    atualizarVisualizacaoHistograma();
}

void MainWindow::on_btnAplicar_clicked()
{
    if (imagemCarregada.isNull()) return;

    imagemProcessada = imagemCarregada;
    int efeitoSelecionado = ui->comboEfeitos->currentIndex();
    int param = ui->spinParametro->value();

    if (efeitoSelecionado >= 7 && efeitoSelecionado <= 10) {
        if (param < 3) param = 3;
        else if (param > 9) param = 9;
        ui->spinParametro->setValue(param);
    }
    if (efeitoSelecionado == 11) { 
        param = std::clamp(param, 1, 9);
        ui->spinParametro->setValue(param);
    }
    if (efeitoSelecionado == 6 && param == 0) {
        param = 127;
        ui->spinParametro->setValue(127);
    }

    int largura = imagemProcessada.width();
    int altura = imagemProcessada.height();

    if (efeitoSelecionado == 12) {
        QTransform transformacao;
        transformacao.rotate(param);
        imagemProcessada = imagemCarregada.transformed(transformacao, Qt::SmoothTransformation);
        ui->labelProcessada->setPixmap(QPixmap::fromImage(imagemProcessada));
        atualizarVisualizacaoHistograma();
        return;
    }
    else if (efeitoSelecionado == 13) {
        bool horizontal = (param == 1);
        imagemProcessada = imagemCarregada.mirrored(horizontal, !horizontal);
        ui->labelProcessada->setPixmap(QPixmap::fromImage(imagemProcessada));
        atualizarVisualizacaoHistograma();
        return;
    }
    else if (efeitoSelecionado == 19) { 
        double ideal = (double)(largura * altura) / 256.0;
        int hist[256] = {0};

        for (int y = 0; y < altura; ++y) {
            QRgb *linha = (QRgb *)imagemCarregada.scanLine(y);
            for (int x = 0; x < largura; ++x) {
                hist[qGray(linha[x])]++;
            }
        }

        int mapaEqualizacao[256] = {0};
        int somaAcumulada = 0;
        for (int i = 0; i < 256; ++i) {
            somaAcumulada += hist[i];
            int novoTom = std::round(somaAcumulada / ideal) - 1;
            mapaEqualizacao[i] = std::clamp(novoTom, 0, 255);
        }

        for (int y = 0; y < altura; ++y) {
            QRgb *linhaOrig = (QRgb *)imagemCarregada.scanLine(y);
            QRgb *linhaProc = (QRgb *)imagemProcessada.scanLine(y);
            for (int x = 0; x < largura; ++x) {
                int cinzaNovo = mapaEqualizacao[qGray(linhaOrig[x])];
                linhaProc[x] = qRgb(cinzaNovo, cinzaNovo, cinzaNovo);
            }
        }
        ui->labelProcessada->setPixmap(QPixmap::fromImage(imagemProcessada));
        atualizarVisualizacaoHistograma();
        return;
    }
    else if (efeitoSelecionado == 20) { 
        int tamanhoBloco = (param > 3) ? param : 8;
        int fatorRes = 3;

        imagemProcessada = QImage(largura * fatorRes, altura * fatorRes, QImage::Format_ARGB32);
        QColor corFundo = this->palette().color(QPalette::Window);
        QColor corTexto = this->palette().color(QPalette::WindowText);
        imagemProcessada.fill(corFundo);

        QPainter painter(&imagemProcessada);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QFont fonte("Courier", tamanhoBloco * fatorRes, QFont::Bold);
        fonte.setStyleHint(QFont::Monospace);
        painter.setFont(fonte);
        painter.setPen(corTexto);

        QString caracteresAscii = " .:-=+*#%@";
        int numCaracteres = caracteresAscii.length();

        int gx[3][3] = {{-1,  0,  1}, {-2,  0,  2}, {-1,  0,  1}};
        int gy[3][3] = {{-1, -2, -1}, { 0,  0,  0}, { 1,  2,  1}};

        for (int y = 0; y < altura; y += tamanhoBloco) {
            for (int x = 0; x < largura; x += tamanhoBloco) {
                int somaGradiente = 0;
                int contador = 0;

                for (int dy = 0; dy < tamanhoBloco && y + dy < altura - 1; ++dy) {
                    for (int dx = 0; dx < tamanhoBloco && x + dx < largura - 1; ++dx) {
                        int px = x + dx;
                        int py = y + dy;

                        if (px > 0 && py > 0) {
                            int somaX = 0, somaY = 0;
                            for (int sy = -1; sy <= 1; sy++) {
                                QRgb *linhaViz = (QRgb *)imagemCarregada.scanLine(py + sy);
                                for (int sx = -1; sx <= 1; sx++) {
                                    int cinza = qGray(linhaViz[px + sx]);
                                    somaX += cinza * gx[sy + 1][sx + 1];
                                    somaY += cinza * gy[sy + 1][sx + 1];
                                }
                            }
                            int gradiente = std::min(255, (int)std::sqrt(somaX * somaX + somaY * somaY));
                            somaGradiente += gradiente;
                            contador++;
                        }
                    }
                }

                int mediaGradiente = (contador > 0) ? (somaGradiente / contador) : 0;
                int indice = (mediaGradiente * (numCaracteres - 1)) / 255;
                QChar caractere = caracteresAscii[indice];

                if (caractere != ' ') {
                    painter.drawText(x * fatorRes, (y + tamanhoBloco) * fatorRes, QString(caractere));
                }
            }
        }
        ui->labelProcessada->setPixmap(QPixmap::fromImage(imagemProcessada));
        atualizarVisualizacaoHistograma();
        return;
    }

    int kernel = (param % 2 == 0) ? param + 1 : param;
    int offset = kernel / 2;

    switch (efeitoSelecionado) {

    case 0: case 1: case 2: case 3: case 4: case 5: case 6:
    {
        for (int y = 0; y < altura; y++) {
            QRgb *linhaOrig = (QRgb *)imagemCarregada.scanLine(y);
            QRgb *linhaProc = (QRgb *)imagemProcessada.scanLine(y);

            for (int x = 0; x < largura; x++) {
                QRgb cor = linhaOrig[x];
                int r = qRed(cor), g = qGreen(cor), b = qBlue(cor);

                if (efeitoSelecionado == 0) {
                    r = qBound(0, r + param, 255);
                    g = qBound(0, g + param, 255);
                    b = qBound(0, b + param, 255);
                } else if (efeitoSelecionado == 1) {
                    int fator = (param < 2 ? 10 : param) - 1;
                    r = std::round((r * fator) / 255.0) * (255 / fator);
                    g = std::round((g * fator) / 255.0) * (255 / fator);
                    b = std::round((b * fator) / 255.0) * (255 / fator);
                } else if (efeitoSelecionado == 2) { g = r; b = r; }
                else if (efeitoSelecionado == 3) { r = g; b = g; }
                else if (efeitoSelecionado == 4) { r = b; g = b; }
                else if (efeitoSelecionado == 5) {
                    if (param == 1) r = 0;
                    else if (param == 2) g = 0;
                    else if (param == 3) b = 0;
                    else if (param == 4) { r = 0; g = 0; }
                    else if (param == 5) { r = 0; b = 0; }
                    else if (param == 6) { g = 0; b = 0; }
                } else if (efeitoSelecionado == 6) {
                    int cinza = qGray(r, g, b);
                    r = g = b = (cinza >= param) ? 255 : 0;
                }
                linhaProc[x] = qRgb(r, g, b);
            }
        }
        break;
    }

    case 7:
    case 8:
    {
        for (int y = offset; y < altura - offset; y++) {
            QRgb *linhaProc = (QRgb *)imagemProcessada.scanLine(y);
            for (int x = offset; x < largura - offset; x++) {
                std::vector<int> vR, vG, vB;
                int sR = 0, sG = 0, sB = 0;

                for (int dy = -offset; dy <= offset; dy++) {
                    QRgb *linhaViz = (QRgb *)imagemCarregada.scanLine(y + dy);
                    for (int dx = -offset; dx <= offset; dx++) {
                        QRgb viz = linhaViz[x + dx];
                        if (efeitoSelecionado == 7) {
                            vR.push_back(qRed(viz));
                            vG.push_back(qGreen(viz));
                            vB.push_back(qBlue(viz));
                        } else {
                            sR += qRed(viz); sG += qGreen(viz); sB += qBlue(viz);
                        }
                    }
                }

                if (efeitoSelecionado == 7) {
                    std::sort(vR.begin(), vR.end());
                    std::sort(vG.begin(), vG.end());
                    std::sort(vB.begin(), vB.end());
                    int meio = (kernel * kernel) / 2;
                    linhaProc[x] = qRgb(vR[meio], vG[meio], vB[meio]);
                } else {
                    int area = kernel * kernel;
                    linhaProc[x] = qRgb(sR / area, sG / area, sB / area);
                }
            }
        }
        break;
    }

    case 9:
    {
        for (int y = offset; y < altura - offset; y++) {
            QRgb *linhaProc = (QRgb *)imagemProcessada.scanLine(y);
            for (int x = offset; x < largura - offset; x++) {
                int freqR[256] = {0}, freqG[256] = {0}, freqB[256] = {0};

                for (int dy = -offset; dy <= offset; dy++) {
                    QRgb *linhaViz = (QRgb *)imagemCarregada.scanLine(y + dy);
                    for (int dx = -offset; dx <= offset; dx++) {
                        QRgb viz = linhaViz[x + dx];
                        freqR[qRed(viz)]++;
                        freqG[qGreen(viz)]++;
                        freqB[qBlue(viz)]++;
                    }
                }

                int maxR = 0, maxG = 0, maxB = 0;
                int idR = 0, idG = 0, idB = 0;
                for(int i = 0; i < 256; i++) {
                    if(freqR[i] > maxR) { maxR = freqR[i]; idR = i; }
                    if(freqG[i] > maxG) { maxG = freqG[i]; idG = i; }
                    if(freqB[i] > maxB) { maxB = freqB[i]; idB = i; }
                }
                linhaProc[x] = qRgb(idR, idG, idB);
            }
        }
        break;
    }

    case 10:
    {
        double sigma = std::max(0.1, kernel / 6.0);
        std::vector<double> pesos(kernel * kernel);
        double pesoTotal = 0;

        for (int dy = -offset; dy <= offset; dy++) {
            for (int dx = -offset; dx <= offset; dx++) {
                double peso = std::exp(-(dx*dx + dy*dy) / (2 * sigma * sigma));
                pesos[(dy + offset) * kernel + (dx + offset)] = peso;
                pesoTotal += peso;
            }
        }

        for (int y = offset; y < altura - offset; y++) {
            QRgb *linhaProc = (QRgb *)imagemProcessada.scanLine(y);
            for (int x = offset; x < largura - offset; x++) {
                double sR = 0, sG = 0, sB = 0;

                for (int dy = -offset; dy <= offset; dy++) {
                    QRgb *linhaViz = (QRgb *)imagemCarregada.scanLine(y + dy);
                    for (int dx = -offset; dx <= offset; dx++) {
                        QRgb viz = linhaViz[x + dx];
                        double p = pesos[(dy + offset) * kernel + (dx + offset)];
                        sR += qRed(viz) * p;
                        sG += qGreen(viz) * p;
                        sB += qBlue(viz) * p;
                    }
                }
                linhaProc[x] = qRgb(qBound(0, (int)(sR / pesoTotal), 255),
                                    qBound(0, (int)(sG / pesoTotal), 255),
                                    qBound(0, (int)(sB / pesoTotal), 255));
            }
        }
        break;
    }

    case 11:
    {
        for (int y = 1; y < altura - 1; y++) {
            QRgb *linhaOrig = (QRgb *)imagemCarregada.scanLine(y);
            QRgb *linhaProc = (QRgb *)imagemProcessada.scanLine(y);
            for (int x = 1; x < largura - 1; x++) {
                int cinzaCentral = qGray(linhaOrig[x]);
                std::vector<std::pair<int, QRgb>> distancias;

                for (int dy = -1; dy <= 1; dy++) {
                    QRgb *linhaViz = (QRgb *)imagemCarregada.scanLine(y + dy);
                    for (int dx = -1; dx <= 1; dx++) {
                        QRgb viz = linhaViz[x + dx];
                        int dist = std::abs(cinzaCentral - qGray(viz));
                        distancias.push_back({dist, viz});
                    }
                }

                std::sort(distancias.begin(), distancias.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

                int sR = 0, sG = 0, sB = 0;
                for(int i = 0; i < param; i++) {
                    sR += qRed(distancias[i].second);
                    sG += qGreen(distancias[i].second);
                    sB += qBlue(distancias[i].second);
                }
                linhaProc[x] = qRgb(sR / param, sG / param, sB / param);
            }
        }
        break;
    }

    case 14:
    {
        if (!imagemMascara.isNull()) {
            QImage mascaraRed = imagemMascara.scaled(largura, altura, Qt::IgnoreAspectRatio);
            for (int y = 0; y < altura; y++) {
                QRgb *linhaOrig = (QRgb *)imagemCarregada.scanLine(y);
                QRgb *linhaProc = (QRgb *)imagemProcessada.scanLine(y);
                QRgb *linhaMasc = (QRgb *)mascaraRed.scanLine(y);

                for (int x = 0; x < largura; x++) {
                    if (qGray(linhaMasc[x]) < 127) {
                        linhaProc[x] = qRgb(0, 0, 0);
                    } else {
                        linhaProc[x] = linhaOrig[x];
                    }
                }
            }
        }
        break;
    }

    case 15: 
    case 16: 
    case 17: 
    {
        int m15[3][3] = {{ 0, -1,  0}, {-1,  4, -1}, { 0, -1,  0}};
        int m16[3][3] = {{-1, -1, -1}, {-1,  8, -1}, {-1, -1, -1}};
        int m17[3][3] = {{-1, -1, -1}, {-1,  9, -1}, {-1, -1, -1}};

        for (int y = 1; y < altura - 1; y++) {
            QRgb *linhaProc = (QRgb *)imagemProcessada.scanLine(y);
            for (int x = 1; x < largura - 1; x++) {
                int sR = 0, sG = 0, sB = 0;

                for (int dy = -1; dy <= 1; dy++) {
                    QRgb *linhaViz = (QRgb *)imagemCarregada.scanLine(y + dy);
                    for (int dx = -1; dx <= 1; dx++) {
                        QRgb viz = linhaViz[x + dx];
                        int peso = (efeitoSelecionado == 15) ? m15[dy + 1][dx + 1] :
                                       (efeitoSelecionado == 16) ? m16[dy + 1][dx + 1] : m17[dy + 1][dx + 1];

                        sR += qRed(viz) * peso;
                        sG += qGreen(viz) * peso;
                        sB += qBlue(viz) * peso;
                    }
                }
                linhaProc[x] = qRgb(qBound(0, sR, 255), qBound(0, sG, 255), qBound(0, sB, 255));
            }
        }
        break;
    }

    case 18:
    {
        int gx[3][3] = {{-1,  0,  1}, {-2,  0,  2}, {-1,  0,  1}};
        int gy[3][3] = {{-1, -2, -1}, { 0,  0,  0}, { 1,  2,  1}};

        for (int y = 1; y < altura - 1; y++) {
            QRgb *linhaProc = (QRgb *)imagemProcessada.scanLine(y);
            for (int x = 1; x < largura - 1; x++) {
                int sXr = 0, sYr = 0;
                int sXg = 0, sYg = 0;
                int sXb = 0, sYb = 0;

                for (int dy = -1; dy <= 1; dy++) {
                    QRgb *linhaViz = (QRgb *)imagemCarregada.scanLine(y + dy);
                    for (int dx = -1; dx <= 1; dx++) {
                        QRgb viz = linhaViz[x + dx];
                        int px = gx[dy + 1][dx + 1];
                        int py = gy[dy + 1][dx + 1];

                        sXr += qRed(viz) * px; sYr += qRed(viz) * py;
                        sXg += qGreen(viz) * px; sYg += qGreen(viz) * py;
                        sXb += qBlue(viz) * px; sYb += qBlue(viz) * py;
                    }
                }

                int r = std::min(255, (int)std::sqrt(sXr * sXr + sYr * sYr));
                int g = std::min(255, (int)std::sqrt(sXg * sXg + sYg * sYg));
                int b = std::min(255, (int)std::sqrt(sXb * sXb + sYb * sYb));

                linhaProc[x] = qRgb(r, g, b);
            }
        }
        break;
    }
    }

    ui->labelProcessada->setPixmap(QPixmap::fromImage(imagemProcessada));
    atualizarVisualizacaoHistograma();
}

void MainWindow::on_btnSalvar_clicked()
{
    if (imagemProcessada.isNull()) {
        QMessageBox::warning(this, "Aviso", "Não há imagem processada para salvar!");
        return;
    }

    QString caminhoArquivo = QFileDialog::getSaveFileName(
        this,
        "Salvar Imagem Processada",
        "imagem_resultado.png",
        "Imagens (*.png *.jpg *.jpeg *.bmp)"
        );

    if (!caminhoArquivo.isEmpty()) {
        bool sucesso = imagemProcessada.save(caminhoArquivo);
        if (sucesso) {
            QMessageBox::information(this, "Sucesso", "Imagem salva com sucesso!");
        } else {
            QMessageBox::critical(this, "Erro", "Ocorreu um problema ao salvar a imagem.");
        }
    }
}

void MainWindow::on_btnCarregarMascara_clicked()
{
    QString caminho = QFileDialog::getOpenFileName(this, "Selecione a Máscara", "", "Imagens (*.png *.jpg *.bmp)");
    if (!caminho.isEmpty()) {
        imagemMascara = QImage(caminho).convertToFormat(QImage::Format_RGB32);
        QMessageBox::information(this, "Sucesso", "Máscara carregada!");
    }
}

void MainWindow::atualizarVisualizacaoHistograma()
{
    if (ui->radioHistOriginal->isChecked() && !imagemCarregada.isNull()) {
        desenharHistograma(imagemCarregada);
    } else if (ui->radioHistProcessada->isChecked() && !imagemProcessada.isNull()) {
        desenharHistograma(imagemProcessada);
    }
}

void MainWindow::on_radioHistOriginal_clicked() { atualizarVisualizacaoHistograma(); }
void MainWindow::on_radioHistProcessada_clicked() { atualizarVisualizacaoHistograma(); }