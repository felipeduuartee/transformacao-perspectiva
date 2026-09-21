#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace fs = std::filesystem;

namespace {

struct EstadoClique {
    cv::Mat base;
    std::vector<cv::Point2f> pontos;
    double escala = 1.0;
    std::string janela;
};

bool ehImagem(const fs::path& caminho)
{
    std::string ext = caminho.extension().string();

    std::transform(
        ext.begin(),
        ext.end(),
        ext.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );

    return ext == ".jpg" ||
           ext == ".jpeg" ||
           ext == ".png" ||
           ext == ".bmp" ||
           ext == ".tif" ||
           ext == ".tiff";
}

std::vector<fs::path> listaImagens(const fs::path& diretorio)
{
    if (!fs::is_directory(diretorio))
        throw std::runtime_error("Diretorio de imagens inexistente.");

    std::vector<fs::path> imagens;

    for (const auto& item : fs::directory_iterator(diretorio)) {
        if (item.is_regular_file() && ehImagem(item.path()))
            imagens.push_back(item.path());
    }

    std::sort(imagens.begin(), imagens.end());

    if (imagens.empty())
        throw std::runtime_error("Nenhuma imagem encontrada.");

    return imagens;
}

double distancia(
    const cv::Point2f& a,
    const cv::Point2f& b
)
{
    double dx = static_cast<double>(a.x - b.x);
    double dy = static_cast<double>(a.y - b.y);

    return std::sqrt(dx * dx + dy * dy);
}

void redesenhaSelecao(EstadoClique& estado)
{
    cv::Mat exibicao = estado.base.clone();

    const std::vector<std::string> nomes = {
        "1 TL",
        "2 TR",
        "3 BR",
        "4 BL"
    };

    for (std::size_t i = 0; i < estado.pontos.size(); ++i) {
        cv::Point pontoTela(
            cvRound(estado.pontos[i].x * estado.escala),
            cvRound(estado.pontos[i].y * estado.escala)
        );

        cv::circle(
            exibicao,
            pontoTela,
            7,
            cv::Scalar(0, 255, 0),
            2,
            cv::LINE_AA
        );

        cv::putText(
            exibicao,
            nomes[i],
            pontoTela + cv::Point(10, -10),
            cv::FONT_HERSHEY_SIMPLEX,
            0.7,
            cv::Scalar(0, 255, 0),
            2,
            cv::LINE_AA
        );
    }

    cv::imshow(estado.janela, exibicao);
}

void callbackMouse(
    int evento,
    int x,
    int y,
    int,
    void* dados
)
{
    if (evento != cv::EVENT_LBUTTONDOWN)
        return;

    auto* estado = static_cast<EstadoClique*>(dados);

    if (estado->pontos.size() >= 4)
        return;

    estado->pontos.emplace_back(
        static_cast<float>(x / estado->escala),
        static_cast<float>(y / estado->escala)
    );

    redesenhaSelecao(*estado);
}

std::vector<cv::Point2f> selecionaPontos(
    const cv::Mat& imagem,
    const std::string& nome
)
{
    EstadoClique estado;

    const double limiteLargura = 1200.0;
    const double limiteAltura = 800.0;

    double escalaX = limiteLargura / imagem.cols;
    double escalaY = limiteAltura / imagem.rows;

    estado.escala = std::min({
        1.0,
        escalaX,
        escalaY
    });

    cv::resize(
        imagem,
        estado.base,
        cv::Size(),
        estado.escala,
        estado.escala,
        cv::INTER_AREA
    );

    estado.janela = "Selecao - " + nome;

    std::cout
        << "\n" << nome << "\n"
        << "Clique nos quatro cantos nesta ordem:\n"
        << "  1. superior esquerdo\n"
        << "  2. superior direito\n"
        << "  3. inferior direito\n"
        << "  4. inferior esquerdo\n"
        << "\nENTER: confirmar\n"
        << "R: recomecar\n"
        << "ESC: cancelar\n\n";

    cv::namedWindow(estado.janela, cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback(
        estado.janela,
        callbackMouse,
        &estado
    );

    redesenhaSelecao(estado);

    while (true) {
        int tecla = cv::waitKey(0);

        if (tecla == 27) {
            cv::destroyWindow(estado.janela);
            throw std::runtime_error("Execucao cancelada.");
        }

        if (tecla == 'r' || tecla == 'R') {
            estado.pontos.clear();
            redesenhaSelecao(estado);
            continue;
        }

        if ((tecla == 13 || tecla == 10) &&
            estado.pontos.size() == 4) {
            break;
        }

        if (tecla == 13 || tecla == 10) {
            std::cout
                << "Selecione os quatro pontos antes de confirmar.\n";
        }
    }

    cv::destroyWindow(estado.janela);

    return estado.pontos;
}

void salvaPontos(
    const fs::path& arquivo,
    const std::vector<cv::Point2f>& pontos
)
{
    cv::FileStorage fsSaida(
        arquivo.string(),
        cv::FileStorage::WRITE
    );

    if (!fsSaida.isOpened())
        throw std::runtime_error("Nao foi possivel salvar os pontos.");

    fsSaida << "ordem"
            << "superior_esquerdo, superior_direito, "
               "inferior_direito, inferior_esquerdo";

    fsSaida << "pontos" << pontos;
}

std::vector<cv::Point2f> carregaPontos(
    const fs::path& arquivo
)
{
    cv::FileStorage fsEntrada(
        arquivo.string(),
        cv::FileStorage::READ
    );

    if (!fsEntrada.isOpened())
        throw std::runtime_error("Nao foi possivel ler os pontos.");

    std::vector<cv::Point2f> pontos;
    fsEntrada["pontos"] >> pontos;

    if (pontos.size() != 4)
        throw std::runtime_error(
            "Arquivo de pontos nao contem quatro coordenadas."
        );

    return pontos;
}

cv::Mat marcaPontos(
    const cv::Mat& imagem,
    const std::vector<cv::Point2f>& pontos
)
{
    cv::Mat saida = imagem.clone();

    const std::vector<std::string> nomes = {
        "1",
        "2",
        "3",
        "4"
    };

    for (std::size_t i = 0; i < pontos.size(); ++i) {
        cv::Point p(
            cvRound(pontos[i].x),
            cvRound(pontos[i].y)
        );

        cv::circle(
            saida,
            p,
            12,
            cv::Scalar(0, 255, 0),
            4,
            cv::LINE_AA
        );

        cv::putText(
            saida,
            nomes[i],
            p + cv::Point(15, -15),
            cv::FONT_HERSHEY_SIMPLEX,
            1.2,
            cv::Scalar(0, 255, 0),
            3,
            cv::LINE_AA
        );
    }

    return saida;
}

cv::Mat criaComparacao(
    const cv::Mat& original,
    const cv::Mat& corrigida
)
{
    const int altura = 700;

    cv::Mat originalRed;
    cv::Mat corrigidaRed;

    double escalaOriginal =
        static_cast<double>(altura) / original.rows;

    double escalaCorrigida =
        static_cast<double>(altura) / corrigida.rows;

    cv::resize(
        original,
        originalRed,
        cv::Size(),
        escalaOriginal,
        escalaOriginal,
        cv::INTER_AREA
    );

    cv::resize(
        corrigida,
        corrigidaRed,
        cv::Size(),
        escalaCorrigida,
        escalaCorrigida,
        cv::INTER_AREA
    );

    cv::putText(
        originalRed,
        "ORIGINAL",
        cv::Point(20, 45),
        cv::FONT_HERSHEY_SIMPLEX,
        1.1,
        cv::Scalar(0, 255, 255),
        3,
        cv::LINE_AA
    );

    cv::putText(
        corrigidaRed,
        "CORRIGIDA",
        cv::Point(20, 45),
        cv::FONT_HERSHEY_SIMPLEX,
        1.1,
        cv::Scalar(0, 255, 255),
        3,
        cv::LINE_AA
    );

    cv::Mat comparacao;
    cv::hconcat(originalRed, corrigidaRed, comparacao);

    return comparacao;
}

void processaImagem(
    const fs::path& caminho,
    const fs::path& diretorioSaida,
    bool refazer
)
{
    cv::Mat imagem = cv::imread(
        caminho.string(),
        cv::IMREAD_COLOR
    );

    if (imagem.empty())
        throw std::runtime_error(
            "Falha ao abrir " + caminho.string()
        );

    std::string base = caminho.stem().string();

    fs::path dirPontos = diretorioSaida / "pontos";
    fs::path dirCorrigidas = diretorioSaida / "corrigidas";
    fs::path dirMarcadas = diretorioSaida / "marcadas";
    fs::path dirMatrizes = diretorioSaida / "matrizes";
    fs::path dirComparacoes = diretorioSaida / "comparacoes";

    fs::create_directories(dirPontos);
    fs::create_directories(dirCorrigidas);
    fs::create_directories(dirMarcadas);
    fs::create_directories(dirMatrizes);
    fs::create_directories(dirComparacoes);

    fs::path arquivoPontos =
        dirPontos / (base + ".yml");

    std::vector<cv::Point2f> origem;

    if (!refazer && fs::exists(arquivoPontos)) {
        origem = carregaPontos(arquivoPontos);

        std::cout
            << base
            << ": reutilizando pontos salvos.\n";
    }
    else {
        origem = selecionaPontos(imagem, caminho.filename().string());
        salvaPontos(arquivoPontos, origem);
    }

    // A folha utilizada no experimento e A4: 210 x 297 mm.
    // A saida preserva essa proporcao fisica independentemente
    // da deformacao perspectiva observada na fotografia.
    constexpr int larguraSaida = 1000;
    const int alturaSaida = cvRound(
        larguraSaida * (297.0 / 210.0)
    );

    std::vector<cv::Point2f> destino = {
        cv::Point2f(0.0f, 0.0f),
        cv::Point2f(
            static_cast<float>(larguraSaida - 1),
            0.0f
        ),
        cv::Point2f(
            static_cast<float>(larguraSaida - 1),
            static_cast<float>(alturaSaida - 1)
        ),
        cv::Point2f(
            0.0f,
            static_cast<float>(alturaSaida - 1)
        )
    };

    cv::Mat matriz = cv::getPerspectiveTransform(
        origem,
        destino
    );

    cv::Mat corrigida;

    cv::warpPerspective(
        imagem,
        corrigida,
        matriz,
        cv::Size(larguraSaida, alturaSaida),
        cv::INTER_LINEAR,
        cv::BORDER_CONSTANT
    );

    cv::Mat marcada = marcaPontos(
        imagem,
        origem
    );

    cv::Mat comparacao = criaComparacao(
        imagem,
        corrigida
    );

    if (!cv::imwrite(
            (dirCorrigidas / (base + "_corrigida.jpg")).string(),
            corrigida
        )) {
        throw std::runtime_error(
            "Falha ao salvar imagem corrigida."
        );
    }

    if (!cv::imwrite(
            (dirMarcadas / (base + "_pontos.jpg")).string(),
            marcada
        )) {
        throw std::runtime_error(
            "Falha ao salvar imagem marcada."
        );
    }

    if (!cv::imwrite(
            (dirComparacoes / (base + "_comparacao.jpg")).string(),
            comparacao
        )) {
        throw std::runtime_error(
            "Falha ao salvar comparacao."
        );
    }

    cv::FileStorage arquivoMatriz(
        (dirMatrizes / (base + ".yml")).string(),
        cv::FileStorage::WRITE
    );

    arquivoMatriz << "largura_original" << imagem.cols;
    arquivoMatriz << "altura_original" << imagem.rows;
    arquivoMatriz << "largura_saida" << larguraSaida;
    arquivoMatriz << "altura_saida" << alturaSaida;
    arquivoMatriz << "pontos_origem" << origem;
    arquivoMatriz << "pontos_destino" << destino;
    arquivoMatriz << "matriz_perspectiva" << matriz;

    std::cout
        << base
        << ": "
        << imagem.cols << "x" << imagem.rows
        << " -> "
        << larguraSaida << "x" << alturaSaida
        << '\n';
}

}

int main(int argc, char** argv)
{
    fs::path entrada =
        argc >= 2
            ? argv[1]
            : "imagens/originais";

    fs::path saida =
        argc >= 3
            ? argv[2]
            : "resultados";

    bool refazer =
        argc >= 4 &&
        std::string(argv[3]) == "--refazer";

    try {
        std::vector<fs::path> imagens =
            listaImagens(entrada);

        std::cout
            << "Imagens encontradas: "
            << imagens.size()
            << "\n";

        for (const fs::path& imagem : imagens) {
            processaImagem(
                imagem,
                saida,
                refazer
            );
        }

        std::cout
            << "\nResultados gravados em "
            << saida.string()
            << '\n';
    }
    catch (const std::exception& e) {
        std::cerr
            << "Erro: "
            << e.what()
            << '\n';

        return 1;
    }

    return 0;
}
