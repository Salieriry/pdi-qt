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

void MainWindow::desenharHistograma(const QImage &imagem){
    int histograma[256] = {0};
    int valorMaximo = 0;

    for (int y = 0; y < imagem.height(); ++y) {
        for (int x = 0; x < imagem.width(); ++x) {

            int tomDeCinza = qGray(imagem.pixel(x, y));

            histograma[tomDeCinza]++;

            if (histograma[tomDeCinza] > valorMaximo) {
                valorMaximo = histograma[tomDeCinza];
            }
        }
    }

    QLineSeries *serie = new QLineSeries();
    serie->setName("Quantidade de Pixels");

    for (int i = 0; i < 256; ++i) {
        serie->append(i, histograma[i]);
    }

    QChart *grafico = new QChart();
    grafico->addSeries(serie);
    grafico->setTitle("Histograma em Tons de Cinza");
    grafico->setAnimationOptions(QChart::SeriesAnimations);

    QValueAxis *eixoX = new QValueAxis();
    eixoX->setRange(0, 255);
    eixoX->setTitleText("Tons de Cinza");
    eixoX->setLabelFormat("%d");
    eixoX->setTickCount(6);
    grafico->addAxis(eixoX, Qt::AlignBottom);
    serie->attachAxis(eixoX);

    QValueAxis *eixoY = new QValueAxis();
    eixoY->setRange(0, valorMaximo);
    eixoY->setTitleText("Frequência (Pixels)");
    grafico->addAxis(eixoY, Qt::AlignLeft);
    serie->attachAxis(eixoY);

    QChartView *visualizadorGrafico = new QChartView(grafico);
    visualizadorGrafico->setRenderHint(QPainter::Antialiasing);

    QLayout *layoutAntigo = ui->containerHistograma->layout();
    if (layoutAntigo != nullptr) {
        QLayoutItem *item;
        while ((item = layoutAntigo->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
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
    if (imagemCarregada.isNull()) {
        QMessageBox::warning(this, "Aviso", "Por favor, carregue uma imagem primeiro!");
        return;
    }

    imagemProcessada = imagemCarregada;

    int efeitoSelecionado = ui->comboEfeitos->currentIndex();

    for (int y = 0; y < imagemProcessada.height(); y++) {
        for (int x = 0; x < imagemProcessada.width(); x++) {

            QColor corAtual = imagemProcessada.pixelColor(x, y);
            int r = corAtual.red();
            int g = corAtual.green();
            int b = corAtual.blue();

            switch (efeitoSelecionado) {

            case 0: { // aumentar brilho
                int C = 60;
                r = qBound(0, r + C, 255);
                g = qBound(0, g + C, 255);
                b = qBound(0, b + C, 255);
                break;
            }
            case 1: { // requantização
                r = std::round((r * 9.0) / 255.0) * (255/9);
                g = std::round((g * 9.0) / 255.0) * (255/9);
                b = std::round((b * 9.0) / 255.0) * (255/9);
                break;
            }
            case 2: { // tons de cinza R
                g = r;
                b = r;
                break;
            }
            case 3: { // tons de cinza G
                r = g;
                b = g;
                break;
            }
            case 4: { // tons de cinza B
                r = b;
                g = b;
                break;
            }
            case 5: { //binarização
                int cinza = qGray(r, g, b);

                if (cinza >= 127) {
                    r = 255; g = 255; b = 255;
                } else {
                    r = 0; g = 0; b = 0;
                }
                break;
            }
            case 6: { // filtro da mediana

                if (x > 0 && x < imagemCarregada.width() - 1 && y > 0 && y < imagemCarregada.height() - 1) {

                    std::vector<int> vizinhosR, vizinhosG, vizinhosB;

                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            QColor corVizinho = imagemCarregada.pixelColor(x + dx, y + dy);
                            vizinhosR.push_back(corVizinho.red());
                            vizinhosG.push_back(corVizinho.green());
                            vizinhosB.push_back(corVizinho.blue());
                        }
                    }

                    std::sort(vizinhosR.begin(), vizinhosR.end());
                    std::sort(vizinhosG.begin(), vizinhosG.end());
                    std::sort(vizinhosB.begin(), vizinhosB.end());

                    r = vizinhosR[4];
                    g = vizinhosG[4];
                    b = vizinhosB[4];

                } else {
                    r = corAtual.red();
                    g = corAtual.green();
                    b = corAtual.blue();
                }
                break;
            }
            case 7: { // filtro da média

                if (x > 0 && x < imagemCarregada.width() - 1 && y > 0 && y < imagemCarregada.height() - 1) {

                    int somaR = 0, somaG = 0, somaB = 0;

                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            QColor corVizinho = imagemCarregada.pixelColor(x + dx, y + dy);

                            somaR += corVizinho.red();
                            somaG += corVizinho.green();
                            somaB += corVizinho.blue();
                        }
                    }

                    r = somaR / 9;
                    g = somaG / 9;
                    b = somaB / 9;

                } else {
                    r = corAtual.red();
                    g = corAtual.green();
                    b = corAtual.blue();
                }
                break;
            }
        }
            imagemProcessada.setPixelColor(x, y, QColor(r, g, b));
        }
    }

    ui->labelProcessada->setPixmap(QPixmap::fromImage(imagemProcessada));

    desenharHistograma(imagemProcessada);
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


void MainWindow::on_btnQuestao4_clicked()
{
    int matrizOriginal[10][10] = {
        {168, 163, 187, 184, 186, 185, 188, 162, 175, 174},
        {171, 159, 186, 191, 190, 160, 103, 136, 153, 162},
        {167, 166, 187, 191, 133, 149, 153, 130, 107, 87},
        {159, 188, 196, 128, 145, 156, 134, 170, 141, 114},
        {176, 200, 102, 118, 92,  98,  76,  118, 67,  102},
        {196, 87,  79,  71,  77,  71,  63,  77,  69,  58},
        {98,  91,  63,  77,  68,  61,  102, 177, 180, 90},
        {120, 94,  68,  108, 84,  99,  91,  200, 210, 186},
        {144, 148, 104, 117, 138, 119, 169, 205, 208, 161},
        {148, 157, 153, 139, 126, 128, 150, 153, 164, 181}
    };

    QImage imgOriginal(10, 10, QImage::Format_RGB32);
    QImage imgProcessada(10, 10, QImage::Format_RGB32);

    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {

            int cinzaVelho = matrizOriginal[y][x];

            int cinzaNovo = std::round((9.0 / 255.0) * cinzaVelho);

            imgOriginal.setPixelColor(x, y, QColor(cinzaVelho, cinzaVelho, cinzaVelho));

            int cinzaParaTela = cinzaNovo * (255 / 9);
            imgProcessada.setPixelColor(x, y, QColor(cinzaParaTela, cinzaParaTela, cinzaParaTela));
        }
    }

    QImage imgOrigAmpliada = imgOriginal.scaled(300, 300, Qt::KeepAspectRatio, Qt::FastTransformation);
    QImage imgProcAmpliada = imgProcessada.scaled(300, 300, Qt::KeepAspectRatio, Qt::FastTransformation);

    ui->labelOriginal->setPixmap(QPixmap::fromImage(imgOrigAmpliada));
    ui->labelProcessada->setPixmap(QPixmap::fromImage(imgProcAmpliada));

    imagemProcessada = imgProcessada;
    desenharHistograma(imagemProcessada);
}

