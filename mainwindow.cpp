#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboEfeitos->addItem("Aumentar Brilho");
    ui->comboEfeitos->addItem("Requantizar");

}

MainWindow::~MainWindow()
{
    delete ui;
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

    QImage imagemProcessada = imagemCarregada;

    int efeitoSelecionado = ui->comboEfeitos->currentIndex();

    for (int y = 0; y < imagemProcessada.height(); y++) {
        for (int x = 0; x < imagemProcessada.width(); x++) {

            QColor corAtual = imagemProcessada.pixelColor(x, y);
            int r = corAtual.red();
            int g = corAtual.green();
            int b = corAtual.blue();

            switch (efeitoSelecionado) {

            case 0: {
                int C = 60;
                r = qBound(0, r + C, 255);
                g = qBound(0, g + C, 255);
                b = qBound(0, b + C, 255);
                break;
            }

            case 1: {
                r = std::round((r * 9.0) / 255.0) * (255/9);
                g = std::round((g * 9.0) / 255.0) * (255/9);
                b = std::round((b * 9.0) / 255.0) * (255/9);
                break;
            }
            }

            imagemProcessada.setPixelColor(x, y, QColor(r, g, b));
        }
    }

    ui->labelProcessada->setPixmap(QPixmap::fromImage(imagemProcessada));
}

