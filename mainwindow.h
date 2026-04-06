#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_btnCarregar_clicked();

    void on_btnAplicar_clicked();

    void on_btnSalvar_clicked();

    void on_btnQuestao4_clicked();

private:
    Ui::MainWindow *ui;
    QImage imagemCarregada;
    QImage imagemProcessada;
    void desenharHistograma(const QImage &imagem);
};
#endif // MAINWINDOW_H
