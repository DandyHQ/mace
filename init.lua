
mace:setfont("LiberationMono-15")

function test()
   tab = mace.focus.tab
   
   print("lua function test called with tab", tab)

   main = tab.main
   print("main textbox is", main);
   print("has cursor at", main.cursor);
   
   seq = main.sequence
   print("main sequence is", seq);
   
   str = seq:get(10, 10)

   print("got", str:len(), ":", str)

   print("insert message")

   str = "\nhello there\n"
   seq:insert(0, str)

   main.cursor = main.cursor + str:len()
   
   print("go through selections")

   sel = mace.selections
   print("sel = ", sel)
   while sel ~= nil do
      print("have selection", sel)

      print("start = ", sel.start)
      print("len   = ", sel.len)
      print("tb = ", sel.textbox)

      seq = sel.textbox.sequence
      str = seq:get(sel.start, sel.len)

      print("str = ", str)

      sel = sel.next
   end

   print("test finished")
end

function quit()
   mace:quit()
end

function eval()
   sel = mace.selections
   while sel ~= nil do
      seq = sel.textbox.sequence
      str = seq:get(sel.start, sel.len)

      f = load(str)
      f()
      
      sel = sel.next
   end
end

function getfilename(tab)
   action = tab.action
   seq = action.sequence

   filename = ""
   i = 0
   while true do
      c = seq:get(i, 1)
      if c == ':' then
	 break
      else
	 filename = filename .. c
	 i = i + 1
      end
   end

   return filename
end

function save()
   tab = mace.focus.tab

   filename = getfilename(tab)
   if filename == nil then
      print("not saving")
      return
   end

   seq = tab.main.sequence
   i = 0
   file = assert(io.open(filename, "w"))

   while true do
      str = seq:get(i, 100)

      if str == nil or str:len() == 0 then
	 break
      end

      assert(file:write(str))
      
      i = i + str:len()
   end

   assert(file:close())
end

function openfile(filename)
   tab = mace:newfiletab(filename, filename)

   tab.next = mace.tabs
   mace.tabs = tab
   mace.focus = tab.main
end

function open()
   sel = mace.selections
   while sel ~= nil do
      seq = sel.textbox.sequence

      str = seq:get(sel.start, sel.len)

      openfile(str)
      
      sel = sel.next
   end
end

function cut()
   print("should cut")
end

function copy()
   print("should copy")
end

function paste()
   print("should paste")
end

function search()
   print("should search")
end
