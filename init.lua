fonts = {
   "/usr/X11R6/lib/X11/fonts/TTF/DejaVuSans.ttf",
   "/usr/share/fonts/dejavu/DejaVuSans.ttf"
}

for key, value in pairs(fonts) do
   e = mace.loadfont(value, 15)
   if e == 0 then
      break
   end
end

if e ~= 0 then
   error("Failed to load a font!")
end

function eval(textbox, s)
   io.write("lua should eval ", s, " in textbox: ", textbox, "\n")
end

