function quit()
   mace:quit()
end

function eval()
   sel = mace:selections()
   for i, sel in pairs(mace:selections()) do
      seq = sel.textbox.sequence
      str = seq:get(sel.start, sel.len)

      f, err = load(str)
      if f ~= nil then
	 f()
      else
	 print("failed to load:", str, ":", err)
      end
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
   mace.focus = t.main
end

function open()
   sel = mace:selections()
   for i, sel in pairs(mace:selections()) do
      seq = sel.textbox.sequence
      str = seq:get(sel.start, sel.len)

      openfile(str)
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

