
clipboard = nil


function quit()
   mace:quit()
end

function eval()
   for i, sel in pairs(mace:selections()) do
      t = sel.textbox
      str = t:get(sel.start, sel.len)

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

   filename = ""
   i = 0
   while true do
      c = action:get(i, 1)
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
   local tab = mace.mousefocus.tab
   local m = tab.main

   filename = getfilename(tab)
   if filename == nil then
      print("not saving")
      return
   end

   l = m:len()
   
   str = m:get(0, l)

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
   
   local pane = mace.keyfocus.tab.pane
   pane:addtab(t, -1)
   pane.focus = t
   mace.keyfocus = t.main
end

function open()
   for i, sel in pairs(mace:selections()) do
      local t = sel.textbox
      str = t:get(sel.start, sel.len)

      openfile(str)
   end
end

function close()
	local t = mace.mousefocus.tab
	local p = t.pane

	t:close()

	if p.tabs == nil then
		p:close()
	end
end

function cut()
   local final = nil
   
   for i, sel in pairs(mace:selections()) do
      final = sel
   end

   if final == nil then
      return
   end

   local t = final.textbox

   clipboard = t:get(final.start, final.len)
   
   t:delete(final.start, final.len)

   t:removeselection(final)

   if final.start <= t.cursor
       and
       t.cursor <= final.start + final.len then

      t.cursor = final.start
   end
end

function copy()
   local final = nil
   
   for i, sel in pairs(mace:selections()) do
      final = sel
   end

   if final == nil then
      return
   end

   local t = final.textbox

   clipboard = t:get(final.start, final.len)
end

function paste()
   t = mace.keyfocus
   t:insert(t.cursor, clipboard)
   t.cursor = t.cursor + clipboard:len()
end

function search()
   print("search is not yet implimented.")
end

