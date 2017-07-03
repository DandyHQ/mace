BEGIN {
	print "/* Generated using xlib-keysyms.awk from xlib-keysyms.txt */"
	print "struct keysymuni { int32_t keysym, unicode; };"
	print "struct keysymuni keymappings[] = {"
}
{ 
	if ($1 != "#" && length($0) > 0) {
		print "{ " $1 ", 0x" substr($2, 2, 10) " },"
	}
}
END {
	print "};"
}