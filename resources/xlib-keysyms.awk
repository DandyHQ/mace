BEGIN {
	print "/* Generated using xlib-keysyms.awk from xlib-keysyms.txt */"
	print "#include <unistd.h>"
	print "int32_t keymappings[] = {"
}
{ 
	if ($1 != "#" && length($0) > 0) {
		print "[" $1 "] = 0x" substr($2, 2, 10) ","
	}
}
END {
	print "};"
}