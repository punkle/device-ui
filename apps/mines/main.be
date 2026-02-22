import math

var root = ui.root()

# Grid settings - 8x5 with 6 mines, sized for 320x240 app area
var COLS = 8
var ROWS = 5
var MINES = 6
var CELL = 24
var GAP = 2
var OX = 5
var OY = 30

# Game state arrays (flattened 2D)
var mines = []      # true if mine
var revealed = []   # true if revealed
var flagged = []    # true if flagged
var counts = []     # adjacent mine count
var buttons = []    # LVGL button objects
var game_over = false
var cells_revealed = 0

# Status label
var status = ui.label(root, "Mines: " + str(MINES))
ui.set_pos(status, 5, 3)

# Initialize arrays
var total = COLS * ROWS
for i : 0 .. total - 1
    mines.push(false)
    revealed.push(false)
    flagged.push(false)
    counts.push(0)
end

# Place mines randomly
var placed = 0
while placed < MINES
    var idx = math.rand() % total
    if !mines[idx]
        mines[idx] = true
        placed += 1
    end
end

# Calculate neighbor counts
for r : 0 .. ROWS - 1
    for c : 0 .. COLS - 1
        var idx = r * COLS + c
        if mines[idx]
            counts[idx] = -1
        else
            var n = 0
            for dr : -1 .. 1
                for dc : -1 .. 1
                    if dr == 0 && dc == 0 continue end
                    var nr = r + dr
                    var nc = c + dc
                    if nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS
                        if mines[nr * COLS + nc]
                            n += 1
                        end
                    end
                end
            end
            counts[idx] = n
        end
    end
end

# Forward declare reveal_cell
var reveal_cell

# Reveal a cell (recursive for zeros)
reveal_cell = def (idx)
    if idx < 0 || idx >= total return end
    if revealed[idx] || flagged[idx] return end

    revealed[idx] = true
    cells_revealed += 1
    var r = idx / COLS
    var c = idx % COLS

    if mines[idx]
        # Hit a mine - game over
        ui.set_text(buttons[idx], "*")
        ui.set_style_bg_color(buttons[idx], "#FF0000")
        game_over = true
        ui.set_text(status, "BOOM! Game Over")
        ui.set_style_text_color(status, "#FF4444")
        # Reveal all mines
        for i : 0 .. total - 1
            if mines[i]
                ui.set_text(buttons[i], "*")
                ui.set_style_bg_color(buttons[i], "#FF0000")
            end
        end
        return
    end

    var cnt = counts[idx]
    if cnt > 0
        ui.set_text(buttons[idx], str(cnt))
    else
        ui.set_text(buttons[idx], " ")
    end
    ui.set_style_bg_color(buttons[idx], "#555555")

    # Flood fill for empty cells (before win check so all cells get revealed visually)
    if cnt == 0
        for dr : -1 .. 1
            for dc : -1 .. 1
                if dr == 0 && dc == 0 continue end
                var nr = r + dr
                var nc = c + dc
                if nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS
                    reveal_cell(nr * COLS + nc)
                end
            end
        end
    end

    # Check win condition after flood fill completes
    if cells_revealed >= total - MINES && !game_over
        game_over = true
        ui.set_text(status, "You Win!")
        ui.set_style_text_color(status, "#00FF00")
    end
end

# Create grid buttons
for r : 0 .. ROWS - 1
    for c : 0 .. COLS - 1
        var idx = r * COLS + c
        var btn = ui.button(root, " ")
        ui.set_pos(btn, OX + c * (CELL + GAP), OY + r * (CELL + GAP))
        ui.set_size(btn, CELL, CELL)
        ui.set_style_bg_color(btn, "#888888")
        buttons.push(btn)

        # Capture idx by value using a factory closure
        var cell_idx = idx
        ui.on_click(btn, def ()
            if game_over return end
            reveal_cell(cell_idx)
        end)
    end
end
