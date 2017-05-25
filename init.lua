
print("Hello")

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
   
   print("go through selections in main")

   sel = main.selections
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

   print("go through selecitons in action")
   sel = mace.focus.tab.action.selections
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
   sel = mace.focus.selections
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

   l = seq:len()
   print("getting", l, "bytes")
   
   str = seq:get(0, l)

   print("saving", filename)

   file = assert(io.open(filename, "w"))

   assert(file:write(str))

   assert(file:close())
end

function openfile(filename)
   local t = mace:newfiletab(filename, filename)
   if t == nil then
      t = mace:newemptytab(filename)
      if t == nil then
	 return
      end
   end
   
   print("open file has tab", t)

   local pane = mace.focus.tab.pane
   pane:addtab(t, -1)
   pane.focus = t
end

function open()
   sel = mace.focus.selections
   while sel ~= nil do
      seq = sel.textbox.sequence

      str = seq:get(sel.start, sel.len)

      openfile(str)
      
      sel = sel.next
   end
end

function close()
   local t = mace.focus.tab
   t:close()
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

