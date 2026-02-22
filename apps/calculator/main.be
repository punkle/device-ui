var root = ui.root()

# State
var display_text = "0"
var first_num = 0.0
var operator = ""
var new_input = true
var has_dot = false

# Display
var display = ui.label(root, "0")
ui.set_pos(display, 5, 5)

# Update display helper reference
var update_display = def ()
    ui.set_text(display, display_text)
end

# Handle number input
var on_number = def (n)
    if new_input
        display_text = str(n)
        new_input = false
        has_dot = false
    else
        if display_text == "0"
            display_text = str(n)
        else
            display_text = display_text + str(n)
        end
    end
    update_display()
end

# Handle decimal point
var on_dot = def ()
    if new_input
        display_text = "0."
        new_input = false
        has_dot = true
    elif !has_dot
        display_text = display_text + "."
        has_dot = true
    end
    update_display()
end

# Handle operator input
var on_operator = def (op)
    first_num = real(display_text)
    operator = op
    new_input = true
end

# Handle equals
var on_equals = def ()
    var second_num = real(display_text)
    var result = 0.0
    if operator == "+"
        result = first_num + second_num
    elif operator == "-"
        result = first_num - second_num
    elif operator == "*"
        result = first_num * second_num
    elif operator == "/"
        if second_num != 0
            result = first_num / second_num
        else
            display_text = "Err"
            update_display()
            new_input = true
            return
        end
    end
    if result == int(result)
        display_text = str(int(result))
        has_dot = false
    else
        display_text = str(result)
        has_dot = true
    end
    update_display()
    first_num = result
    new_input = true
end

# Handle clear
var on_clear = def ()
    display_text = "0"
    first_num = 0.0
    operator = ""
    new_input = true
    has_dot = false
    update_display()
end

# Button layout: rows of [label, x, y, w]
# Row 1: 7 8 9 /
# Row 2: 4 5 6 *
# Row 3: 1 2 3 -
# Row 4: C 0 . +
# Row 5: =

var bw = 58
var bh = 30
var sx = 2
var sy = 30
var gap = 3

# Row 1
var b7 = ui.button(root, "7")
ui.set_pos(b7, sx, sy)
ui.set_size(b7, bw, bh)
ui.on_click(b7, def () on_number(7) end)

var b8 = ui.button(root, "8")
ui.set_pos(b8, sx + (bw + gap), sy)
ui.set_size(b8, bw, bh)
ui.on_click(b8, def () on_number(8) end)

var b9 = ui.button(root, "9")
ui.set_pos(b9, sx + 2 * (bw + gap), sy)
ui.set_size(b9, bw, bh)
ui.on_click(b9, def () on_number(9) end)

var bdiv = ui.button(root, "/")
ui.set_pos(bdiv, sx + 3 * (bw + gap), sy)
ui.set_size(bdiv, bw, bh)
ui.on_click(bdiv, def () on_operator("/") end)

# Row 2
var r2y = sy + bh + gap

var b4 = ui.button(root, "4")
ui.set_pos(b4, sx, r2y)
ui.set_size(b4, bw, bh)
ui.on_click(b4, def () on_number(4) end)

var b5 = ui.button(root, "5")
ui.set_pos(b5, sx + (bw + gap), r2y)
ui.set_size(b5, bw, bh)
ui.on_click(b5, def () on_number(5) end)

var b6 = ui.button(root, "6")
ui.set_pos(b6, sx + 2 * (bw + gap), r2y)
ui.set_size(b6, bw, bh)
ui.on_click(b6, def () on_number(6) end)

var bmul = ui.button(root, "*")
ui.set_pos(bmul, sx + 3 * (bw + gap), r2y)
ui.set_size(bmul, bw, bh)
ui.on_click(bmul, def () on_operator("*") end)

# Row 3
var r3y = r2y + bh + gap

var b1 = ui.button(root, "1")
ui.set_pos(b1, sx, r3y)
ui.set_size(b1, bw, bh)
ui.on_click(b1, def () on_number(1) end)

var b2 = ui.button(root, "2")
ui.set_pos(b2, sx + (bw + gap), r3y)
ui.set_size(b2, bw, bh)
ui.on_click(b2, def () on_number(2) end)

var b3 = ui.button(root, "3")
ui.set_pos(b3, sx + 2 * (bw + gap), r3y)
ui.set_size(b3, bw, bh)
ui.on_click(b3, def () on_number(3) end)

var bsub = ui.button(root, "-")
ui.set_pos(bsub, sx + 3 * (bw + gap), r3y)
ui.set_size(bsub, bw, bh)
ui.on_click(bsub, def () on_operator("-") end)

# Row 4
var r4y = r3y + bh + gap

var bclr = ui.button(root, "C")
ui.set_pos(bclr, sx, r4y)
ui.set_size(bclr, bw, bh)
ui.on_click(bclr, def () on_clear() end)

var b0 = ui.button(root, "0")
ui.set_pos(b0, sx + (bw + gap), r4y)
ui.set_size(b0, bw, bh)
ui.on_click(b0, def () on_number(0) end)

var bdot = ui.button(root, ".")
ui.set_pos(bdot, sx + 2 * (bw + gap), r4y)
ui.set_size(bdot, bw, bh)
ui.on_click(bdot, def () on_dot() end)

var badd = ui.button(root, "+")
ui.set_pos(badd, sx + 3 * (bw + gap), r4y)
ui.set_size(badd, bw, bh)
ui.on_click(badd, def () on_operator("+") end)

# Row 5: = (full width)
var r5y = r4y + bh + gap

var beq = ui.button(root, "=")
ui.set_pos(beq, sx, r5y)
ui.set_size(beq, 4 * bw + 3 * gap, bh)
ui.on_click(beq, def () on_equals() end)
