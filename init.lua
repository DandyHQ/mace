fonts = {
   "/usr/X11R6/lib/X11/fonts/TTF/DejaVuSans.ttf",
   "/usr/share/fonts/dejavu/DejaVuSans.ttf"
}

for key, value in pairs(fonts) do
   e = setfont(value, 15)
   if e == 0 then
      break
   end
end

if e ~= 0 then
   error("Failed to load a font!")
end

function test(tab, selections)
   print("lua function test called with tab", tab,
	 "and selections", selections)

   main = tab.main
   print("main textbox is", main);
   print("has cursor at", main.cursor);
   
   seq = main.sequence
   print("main sequence is", seq);
   
   str = seq:get(10, 10)

   print("got", str:len(), ":", str)

   print("insert message")

   seq:insert(0, "\nhello there\n")

   print("go through selections")

   for key, sel in pairs(selections) do
      print("start = ", sel.start)
      print("len   = ", sel.len)
      print("tb = ", sel.textbox)

      seq = sel.textbox.sequence
      str = seq:get(sel.start, sel.len)

      print("str = ", str)
   end

   print("test finished")
end

function hello(main, selections)
   print("hello, world")
end
