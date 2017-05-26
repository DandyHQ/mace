mace:setfont("LiberationMono-15")

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
   local p = t.pane

   t:close()

   if p.tabs == nil then
      t = mace:newemptytab("*scratch*")
      p:addtab(t, 0)

      p.focus = t
      if mace.focus == nil then
	 mace.focus = t.main
      end
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

