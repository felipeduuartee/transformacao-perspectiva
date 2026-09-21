# Transformação perspectiva de documentos

Trabalho da disciplina de Visão Computacional.

A aplicação corrige a perspectiva de fotografias de uma folha A4.
O usuário seleciona os quatro cantos do documento e o programa calcula
uma transformação perspectiva com `cv::getPerspectiveTransform`, aplicada
à imagem por meio de `cv::warpPerspective`.

Foram utilizadas quatro fotografias da mesma folha A4, capturadas com
diferentes inclinações em relação à câmera.

## Dependências

Ubuntu:

    sudo apt install build-essential cmake libopencv-dev

## Compilação

    cmake -S . -B build
    cmake --build build -j

## Execução

    ./build/corrige_perspectiva imagens/originais resultados

Na primeira execução, os quatro cantos da folha devem ser selecionados
nesta ordem:

1. superior esquerdo;
2. superior direito;
3. inferior direito;
4. inferior esquerdo.

Pressione `Enter` para confirmar a seleção ou `R` para recomeçar.

As coordenadas escolhidas são armazenadas em `resultados/pontos`.
Nas execuções seguintes, esses pontos são reutilizados automaticamente.

Para selecionar novamente os quatro cantos:

    ./build/corrige_perspectiva imagens/originais resultados --refazer

## Configuração do experimento

As quatro imagens de entrada possuem resolução de 1152 x 1536 pixels.

A folha utilizada é A4, com dimensões físicas de 210 x 297 mm.
Os quatro cantos selecionados são mapeados para os vértices:

    (0, 0)
    (999, 0)
    (999, 1413)
    (0, 1413)

produzindo imagens de saída com 1000 x 1414 pixels e preservando
aproximadamente a proporção física 297/210 da folha.

Para cada fotografia são armazenados os quatro pontos de origem, os
quatro pontos de destino e a matriz de transformação perspectiva 3 x 3.
Dessa forma, os resultados apresentados podem ser reproduzidos sem uma
nova seleção manual.

## Saídas

- `resultados/pontos/`: quatro coordenadas selecionadas em cada imagem;
- `resultados/matrizes/`: matrizes de transformação e parâmetros usados;
- `resultados/marcadas/`: imagens mostrando os pontos selecionados;
- `resultados/corrigidas/`: documentos após a transformação perspectiva;
- `resultados/comparacoes/`: comparação entre original e corrigida;
- `resultados/execucao.txt`: saída da execução final.
