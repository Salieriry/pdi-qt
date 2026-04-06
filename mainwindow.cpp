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
    ui->comboEfeitos->addItem("Binarização");
    ui->comboEfeitos->addItem("Filtro da Mediana (3x3)");
    ui->comboEfeitos->addItem("Filtro da Média (3x3)");
    ui->comboEfeitos->addItem("Filtro da Moda (Passa-baixa)");
    ui->comboEfeitos->addItem("Filtro Gaussiano");
    ui->comboEfeitos->addItem("Filtro K-Nearest (k vizinhos mais próximos)");
    ui->comboEfeitos->addItem("Rotação (Use o parâmetro para o ângulo)");
    ui->comboEfeitos->addItem("Espelhamento (1=Horiz, 2=Vert)");
    ui->comboEfeitos->addItem("Aplicar Máscara (Requer carregamento)");


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
        for (int x = 0; x < imagem.width(); ++x) {
            QColor cor = imagem.pixelColor(x, y);
            histR[cor.red()]++;
            histG[cor.green()]++;
            histB[cor.blue()]++;

            valorMaximo = std::max({valorMaximo, histR[cor.red()], histG[cor.green()], histB[cor.blue()]});
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

    // 3. Eixos
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

    // 4. Renderização no layout
    QLayout *layoutAntigo = ui->containerHistograma->layout();
    if (layoutAntigo != nullptr) {
        QLayoutItem *item;
        while ((item = layoutAntigo->takeAt(0)) != nullptr) {
            delete item->widget(); delete item;
        }
        delete layoutAntigo;
    }

    QVBoxLayout *novoLayout = new QVBoxLayout(ui->containerHistograma);
    novoLayout->addWidget(visualizadorGrafico);
    ui->containerHistograma->setLayout(novoLayout);
}

void MainWindow::on_btnCarregar_clicked()
{
    QString caminhoArquivo = QFileDialog::getOpenFileName(
        this,
        "Selecione uma Imagem",
        "",
        "Imagens (*.png *.jpg *.jpeg *.bmp)"
        );

    if (caminhoArquivo.isEmpty()) {
        return;
    }

    imagemCarregada = QImage(caminhoArquivo);

    if (imagemCarregada.isNull()) {
        QMessageBox::warning(this, "Erro", "Não foi possível carregar a imagem.");
        return;
    }

    ui->labelOriginal->setPixmap(QPixmap::fromImage(imagemCarregada));

}


void MainWindow::on_btnAplicar_clicked()
{
    if (imagemCarregada.isNull()) return;

    imagemProcessada = imagemCarregada;
    int efeitoSelecionado = ui->comboEfeitos->currentIndex();

    int param = ui->spinParametro->value();


    //regras para os filtros passa-baixa (casos 6 a 9: mediana, média, moda, gaussiano)
    if (efeitoSelecionado >= 6 && efeitoSelecionado <= 9) {
        if (param < 3) {
            param = 3; // mínimo é matriz 3x3
            ui->spinParametro->setValue(3);
        } else if (param > 9) {
            param = 9; // máximo é 9x9 por questões de processamento
            ui->spinParametro->setValue(9);
        }
    }

    // regras para o knn smoothing (caso 10)
    // O knn olha fixamente para um kernel 3x3 (9 pixels no total).
    if (efeitoSelecionado == 10) {
        if (param < 1) param = 1;
        if (param > 9) param = 9;
        ui->spinParametro->setValue(param);
    }

    // regra para binarização (caso 5)
    if (efeitoSelecionado == 5 && param == 0) {
        param = 127;
        ui->spinParametro->setValue(127);
    }

    // EFEITOS ESTRUTURAIS (que mudam o tamanho ou não dependem do laço pixel a pixel)
    if (efeitoSelecionado == 11) { // Rotação
        QTransform transformacao;
        transformacao.rotate(param);
        imagemProcessada = imagemCarregada.transformed(transformacao, Qt::SmoothTransformation);
    }
    else if (efeitoSelecionado == 12) { // Espelhamento
        bool horizontal = (param == 1);
        imagemProcessada = imagemCarregada.mirrored(horizontal, !horizontal);
    }
    // EFEITOS PIXEL A PIXEL E VIZINHANÇA
    else {
        int kernel = (param % 2 == 0) ? param + 1 : param;
        int offset = kernel / 2;

        for (int y = 0; y < imagemProcessada.height(); y++) {
            for (int x = 0; x < imagemProcessada.width(); x++) {

                QColor corAtual = imagemCarregada.pixelColor(x, y);
                int r = corAtual.red(), g = corAtual.green(), b = corAtual.blue();

                switch (efeitoSelecionado) {

                case 0: { // aumentar brilho
                    r = qBound(0, r + param, 255);
                    g = qBound(0, g + param, 255);
                    b = qBound(0, b + param, 255);
                    break;
                }
                case 1: { // requantizar
                    int niveis = (param < 2) ? 10 : param;
                    int fator = niveis - 1;
                    r = std::round((r * fator) / 255.0) * (255 / fator);
                    g = std::round((g * fator) / 255.0) * (255 / fator);
                    b = std::round((b * fator) / 255.0) * (255 / fator);
                    break;
                }
                case 2: // tons de cinza (canal R)
                    g = r; b = r; break;
                case 3: // tons de cinza (canal G)
                    r = g; b = g; break;
                case 4: // tons de cinza (canal B)
                    r = b; g = b; break;
                case 5: { // binarização
                    int cinza = qGray(r, g, b);
                    if (cinza >= param) { r = 255; g = 255; b = 255; }
                    else { r = 0; g = 0; b = 0; }
                    break;
                }

                case 6: // mediana com kernel variável
                case 7: // média com kernel variável
                case 8: // moda
                {
                    if (x >= offset && x < imagemCarregada.width() - offset && y >= offset && y < imagemCarregada.height() - offset) {
                        std::vector<int> vR, vG, vB;
                        int sR = 0, sG = 0, sB = 0;
                        std::map<int, int> freqR, freqG, freqB;

                        for (int dy = -offset; dy <= offset; dy++) {
                            for (int dx = -offset; dx <= offset; dx++) {
                                QColor viz = imagemCarregada.pixelColor(x + dx, y + dy);
                                if (efeitoSelecionado == 6) {
                                    vR.push_back(viz.red()); vG.push_back(viz.green()); vB.push_back(viz.blue());
                                } else if (efeitoSelecionado == 7) {
                                    sR += viz.red(); sG += viz.green(); sB += viz.blue();
                                } else {
                                    freqR[viz.red()]++; freqG[viz.green()]++; freqB[viz.blue()]++;
                                }
                            }
                        }

                        if (efeitoSelecionado == 6) { // mediana
                            std::sort(vR.begin(), vR.end());
                            std::sort(vG.begin(), vG.end());
                            std::sort(vB.begin(), vB.end());
                            int meio = (kernel * kernel) / 2;
                            r = vR[meio]; g = vG[meio]; b = vB[meio];
                        } else if (efeitoSelecionado == 7) { // média
                            int area = kernel * kernel;
                            r = sR / area; g = sG / area; b = sB / area;
                        } else { // moda
                            auto maxFreq = [](const std::map<int,int>& mapa) {
                                return std::max_element(mapa.begin(), mapa.end(),
                                                        [](const auto& a, const auto& b) { return a.second < b.second; })->first;
                            };
                            r = maxFreq(freqR); g = maxFreq(freqG); b = maxFreq(freqB);
                        }
                    }
                    break;
                }

                case 9: { // filtro gaussiano
                    if (x >= offset && x < imagemCarregada.width() - offset && y >= offset && y < imagemCarregada.height() - offset) {
                        double sigma = kernel / 6.0;
                        if (sigma < 0.1) sigma = 0.1;
                        double sR = 0, sG = 0, sB = 0, pesoTotal = 0;

                        for (int dy = -offset; dy <= offset; dy++) {
                            for (int dx = -offset; dx <= offset; dx++) {
                                double peso = std::exp(-(dx*dx + dy*dy) / (2 * sigma * sigma));
                                QColor viz = imagemCarregada.pixelColor(x + dx, y + dy);
                                sR += viz.red() * peso;
                                sG += viz.green() * peso;
                                sB += viz.blue() * peso;
                                pesoTotal += peso;
                            }
                        }
                        r = sR / pesoTotal; g = sG / pesoTotal; b = sB / pesoTotal;
                    }
                    break;
                }

                case 10: { // filtro knn smoothing
                    if (x > 0 && x < imagemCarregada.width() - 1 && y > 0 && y < imagemCarregada.height() - 1) {
                        std::vector<std::pair<int, QColor>> distancias;
                        int cinzaCentral = qGray(corAtual.red(), corAtual.green(), corAtual.blue());

                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                QColor viz = imagemCarregada.pixelColor(x + dx, y + dy);
                                int dist = std::abs(cinzaCentral - qGray(viz.red(), viz.green(), viz.blue()));
                                distancias.push_back({dist, viz});
                            }
                        }

                        std::sort(distancias.begin(), distancias.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

                        int limitK = std::min(param > 0 ? param : 3, 9);
                        int sR = 0, sG = 0, sB = 0;
                        for(int i = 0; i < limitK; i++) {
                            sR += distancias[i].second.red();
                            sG += distancias[i].second.green();
                            sB += distancias[i].second.blue();
                        }
                        r = sR / limitK; g = sG / limitK; b = sB / limitK;
                    }
                    break;
                }

                case 13: { // máscara
                    if (!imagemMascara.isNull()) {
                        QImage mascaraRedimensionada = imagemMascara.scaled(imagemCarregada.size(), Qt::IgnoreAspectRatio);
                        int cinzaMascara = qGray(mascaraRedimensionada.pixel(x, y));
                        if (cinzaMascara < 127) { r = 0; g = 0; b = 0; }
                    }
                    break;
                }
                }
                imagemProcessada.setPixelColor(x, y, QColor(r, g, b));
            }
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
        imagemMascara = QImage(caminho);
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

