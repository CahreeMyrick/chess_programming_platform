// apps/web-client/src/pages/GamePage/GamePage.js

export function renderGamePage(root) {
  root.innerHTML = `
    <header class="topbar">
      <div class="brand">
        <div class="brand-icon">IG</div>
        <div>
          <div class="brand-name">ICHIGO</div>
          <div class="brand-sub">chess engine</div>
        </div>
      </div>

      <nav>
        <a href="/" class="nav-link active">Play</a>
        <a href="/puzzles" class="nav-link">Puzzles</a>
      </nav>

      <div class="topbar-end">
        <button class="btn btn-primary btn-sm" id="new">+ New Game</button>
      </div>
    </header>

    <main class="page">
      <section class="main-col">
        <div class="card">
          <div class="card-header">
            <span class="card-label">Game</span>

            <div class="controls-row">
              <div class="field">
                <label for="playAs">Side</label>
                <select id="playAs">
                  <option value="w" selected>White</option>
                  <option value="b">Black</option>
                </select>
              </div>

              <div class="field hidden">
                <label for="difficulty">Depth</label>
                <input id="difficulty" type="range" min="1" max="5" step="1" value="2">
                <span id="diffLabel" style="font-family:var(--mono);font-size:11px;color:var(--teal);">2/5</span>
              </div>
            </div>
          </div>

          <div class="player-strip">
            <div class="player-info">
              <div class="avatar engine">♟</div>
              <div>
                <div class="player-name">Ichigo</div>
                <div class="player-role">Engine</div>
              </div>
            </div>
            <div class="clock" id="bClock">05:00</div>
          </div>

          <div class="board-wrap">
            <div id="board"></div>
          </div>

          <div class="player-strip">
            <div class="player-info">
              <div class="avatar human">♙</div>
              <div>
                <div class="player-name">You</div>
                <div class="player-role">Guest</div>
              </div>
            </div>
            <div class="clock active" id="wClock">05:00</div>
          </div>

          <div id="gameBanner" class="game-banner"></div>

          <div class="board-actions">
            <button class="btn btn-ghost btn-sm" id="flip">⇅ Flip</button>
            <button class="btn btn-ghost btn-sm" id="hintBtn">💡 Hint</button>
            <button class="btn btn-ghost btn-sm hidden" id="engineMove">Engine Move</button>
            <button class="btn btn-danger btn-sm" id="finish">End Game</button>
          </div>
        </div>

        <div class="hidden">
          <input id="ms" type="number" value="500" min="50" step="50">
          <input id="depth" type="number" min="1" step="1">
        </div>
      </section>

      <aside class="side-col">
        <div class="card">
          <div class="card-header">
            <span class="card-label">Moves</span>
            <div class="status-inline">
              <span id="spinner" class="spin" style="display:none"></span>
              <span id="thinkingText" class="status-text"></span>
            </div>
          </div>

          <div id="movesGrid" class="moves-grid">
            <span style="grid-column:1/-1;color:var(--text-3);font-size:11px;padding:3px 0;">
              No moves yet
            </span>
          </div>
        </div>

        <div class="card">
          <div class="card-header">
            <span class="card-label">Engine Log</span>
          </div>
          <div id="log" class="engine-log">—</div>
        </div>
      </aside>
    </main>
  `;
    Chessboard("board", {
    draggable: true,
    position: "start",
    pieceTheme: "./public/chessboardjs-1.0.0/img/chesspieces/wikipedia/{piece}.png"
  });
}