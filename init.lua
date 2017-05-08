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

function save(main, selections)
   print("should save")
end

function cut(main, selections)
   print("should cut")
end

function copy(main, selections)
   print("should copy")
end

function paste(main, selections)
   print("should paste")
end

function search(main, selections)
   print("should search")
end
