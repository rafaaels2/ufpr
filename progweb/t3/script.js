document.addEventListener("DOMContentLoaded", function() {
    const canvasContainer = document.getElementById('canvasContainer');
    const drawButton = document.getElementById('drawButton');
    const numVerticesInput = document.getElementById('numVertices');

    let vertices = [];

    // Função para calcular a posição dos vértices do polígono
    function calculateVertices(numVertices, centerX, centerY, radius) {
        const vertices = [];
        for (let i = 0; i < numVertices; i++) {
            const angle = (i * 2 * Math.PI) / numVertices - Math.PI / 2;
            const x = centerX + radius * Math.cos(angle);
            const y = centerY + radius * Math.sin(angle);
            vertices.push({ x, y });
        }
        return vertices;
    }

    // Função para criar e posicionar os canvases
    function createPolygon(numVertices) {
        // Limpa os canvases anteriores
        canvasContainer.innerHTML = '';
        
        const centerX = 300;
        const centerY = 300;
        const radius = 200;
        vertices = calculateVertices(numVertices, centerX, centerY, radius);

        for (let i = 0; i < numVertices; i++) {
            addCanvasForEdge(i);
        }
    }

    function addCanvasForEdge(i) {
        const nextIndex = (i + 1) % vertices.length;
        const startX = vertices[i].x;
        const startY = vertices[i].y;
        const endX = vertices[nextIndex].x;
        const endY = vertices[nextIndex].y;

        const dx = endX - startX;
        const dy = endY - startY;

        const angle = Math.atan2(dy, dx);
        const length = Math.sqrt(dx * dx + dy * dy);

        // Cria o canvas para o lado do polígono
        const canvas = document.createElement('canvas');
        canvas.className = 'polygon-canvas';
        canvas.width = length;
        canvas.height = 10;

        // Desenha no canvas
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = 'green';
        ctx.fillRect(0, 0, length, 10);

        // Posiciona e rotaciona o canvas
        canvas.style.transformOrigin = '0 0';
        canvas.style.transform = `rotate(${angle}rad)`;
        canvas.style.left = `${startX}px`;
        canvas.style.top = `${startY}px`;

        // Adiciona evento de clique com botão direito para dividir a aresta
        canvas.addEventListener('contextmenu', function(event) {
            event.preventDefault(); // Previne o menu de contexto padrão
            const midX = (startX + endX) / 2;
            const midY = (startY + endY) / 2;

            // Insere o novo ponto médio no array de vértices
            vertices.splice(nextIndex, 0, { x: midX, y: midY });

            // Recria o polígono com os novos vértices
            createPolygon(vertices.length);
        });

        // Adiciona o canvas ao contêiner
        canvasContainer.appendChild(canvas);
    }

    // Adiciona evento ao botão para desenhar o polígono
    drawButton.addEventListener('click', function() {
        const numVertices = parseInt(numVerticesInput.value, 10);
        if (numVertices >= 3 && numVertices <= 8) {
            createPolygon(numVertices);
        } else {
            alert('Por favor, insira um número de 3 a 8.');
        }
    });
});
