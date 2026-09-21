
## Configuração do experimento

Foram utilizadas quatro fotografias da mesma folha A4, com diferentes
inclinações em relação à câmera.

A folha possui proporção física de 210 x 297 mm. Após a seleção dos quatro
cantos, eles são mapeados para os vértices de uma imagem retangular de
1000 x 1414 pixels, preservando aproximadamente a proporção 297/210.

A ordem dos pontos é:

1. superior esquerdo;
2. superior direito;
3. inferior direito;
4. inferior esquerdo.

Para cada imagem, o programa salva as coordenadas utilizadas e a matriz
3x3 calculada por `cv::getPerspectiveTransform`. Dessa forma, execuções
posteriores reutilizam exatamente os mesmos pontos sem necessidade de
nova seleção manual.
